#include "win_kernel.h"
#include "util.h"
#include "win.h"

DWORD_PTR set_cpu_affinity(int cpu)
{
    HANDLE thread = GetCurrentThread();
    DWORD_PTR previous_affinity_mask = SetThreadAffinityMask(thread, 1 << cpu);
    if (previous_affinity_mask == 0)
    {
        errx(EXIT_FAILURE, "failed to set CPU affinity, error %d", GetLastError());
    }

    dbgmsg("set CPU affinity to core %d", cpu);
    return previous_affinity_mask;
}

void reset_cpu_affinity(DWORD_PTR previous_affinity_mask)
{
    HANDLE thread = GetCurrentThread();
    if (SetThreadAffinityMask(thread, previous_affinity_mask) == 0)
    {
        errx(EXIT_FAILURE, "failed to reset CPU affinity, error %d", GetLastError());
    }

    dbgmsg("reset CPU affinity to previous state");
}

HANDLE open_driver(void)
{
    HANDLE driver_handle = CreateFileW(L"\\\\.\\AsrOmgDrv", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (!driver_handle || driver_handle == INVALID_HANDLE_VALUE)
    {
        errx(EXIT_FAILURE, "failed to open \\\\.\\AsrOmgDrv, error %d", GetLastError());
        return NULL;
    }

    dbgmsg("opened \\\\.\\AsrOmgDrv with handle %p", driver_handle);

    return driver_handle;
}

void close_driver(HANDLE driver_handle)
{
    if (!CloseHandle(driver_handle))
    {
        errx(EXIT_FAILURE, "failed to close driver handle, error %d", GetLastError());
    }

    dbgmsg("closed driver handle %p", driver_handle);
}

void submit_ioctl(HANDLE driver_handle, DWORD ioctl_code, void* input, DWORD input_size, void* output, DWORD output_size)
{
    DWORD bytes_returned;
    if (!DeviceIoControl(driver_handle, ioctl_code, input, input_size, output, output_size, &bytes_returned, NULL))
    {
        errx(EXIT_FAILURE, "DeviceIoControl failed, error %d", GetLastError());
    }
}

void allocate_contiguous_memory(HANDLE driver_handle, uint64_t size, uint64_t* out_physical_address, uint64_t* out_virtual_address)
{
    struct request_data
    {
        uint32_t allocation_size;
        uint32_t physical_address;
        uint64_t virtual_address;
    };

    struct request_data request = { 0 };
    request.allocation_size = size;

    submit_ioctl(driver_handle, 0x222880, &request, sizeof(request), &request, sizeof(request));

    *out_physical_address = request.physical_address;
    *out_virtual_address = request.virtual_address;

    dbgmsg("allocated %lld bytes at physical address %p and virtual address %p", size, *out_physical_address, *out_virtual_address);
}

void read_physical(HANDLE driver_handle, uint64_t physical_address, void* buffer, size_t size)
{
    struct request_data
    {
        uint64_t physical_address;
        uint32_t size;
        uint32_t value_size;
        void* data;
    };

    struct request_data request = { 0 };
    request.physical_address = physical_address;
    request.size = size;
    request.value_size = 1;
    request.data = buffer;

    submit_ioctl(driver_handle, 0x222808, &request, sizeof(request), &request, sizeof(request));

    dbgmsg("read %lld bytes from physical address %p", size, physical_address);
}

void write_physical(HANDLE driver_handle, uint64_t physical_address, void* buffer, size_t size)
{
    struct request_data
    {
        uint64_t physical_address;
        uint32_t size;
        uint32_t value_size;
        void* data;
    };

    struct request_data request = { 0 };
    request.physical_address = physical_address;
    request.size = size;
    request.value_size = 1;
    request.data = buffer;

    submit_ioctl(driver_handle, 0x22280C, &request, sizeof(request), &request, sizeof(request));

    dbgmsg("wrote %lld bytes to physical address %p", size, physical_address);
}

uint64_t rdmsr_on_cpu(HANDLE driver_handle, uint32_t reg, int cpu)
{
    DWORD_PTR original_affinity = set_cpu_affinity(cpu);

    struct request_data
    {
        uint32_t return_value_low;
        uint32_t padding;
        uint32_t msr_reg;
        uint32_t return_value_high;
    };

    struct request_data request = { 0 };
    request.msr_reg = reg;
    request.return_value_low = 0xdead;

    submit_ioctl(driver_handle, 0x222848, &request, sizeof(request), &request, sizeof(request));

    dbgmsg("read msr %x on cpu %d, value %llx", reg, cpu, request.return_value_low | ((uint64_t)request.return_value_high << 32));

    reset_cpu_affinity(original_affinity);

    return request.return_value_low | ((uint64_t)request.return_value_high << 32);
}

bool wrmsr_on_cpu(HANDLE driver_handle, uint32_t reg, int cpu, uint64_t val)
{
    DWORD_PTR original_affinity = set_cpu_affinity(cpu);

    struct request_data
    {
        uint32_t return_value_low;
        uint32_t padding;
        uint32_t msr_reg;
        uint32_t return_value_high;
    };

    struct request_data request = { 0 };
    request.msr_reg = reg;
    request.return_value_low = val & 0xFFFFFFFF;
    request.return_value_high = val >> 32;

    submit_ioctl(driver_handle, 0x22284C, &request, sizeof(request), &request, sizeof(request));

    dbgmsg("wrote msr %x on cpu %d, value %llx", reg, cpu, val);

    reset_cpu_affinity(original_affinity);

    return true;
}

void* mmap_contig(HANDLE driver_handle, size_t size, uint64_t* physical_address)
{
    uint64_t virtual_address;
    allocate_contiguous_memory(driver_handle, size, physical_address, &virtual_address);
    return (void*)virtual_address;
}

void munmap_contig(HANDLE driver_handle, void* data)
{
    /**/
}
