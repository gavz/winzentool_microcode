#include <stdio.h>
#include "win_kernel.h"
#include "win_test.h"
#include "util.h"

#define MSR_AMD64_PATCH_LEVEL 0x0000008b

extern uint64_t call_fpatan(uint64_t arg);

uint64_t test_fpatan_in_cpu(int cpu)
{
    DWORD_PTR original = set_cpu_affinity(cpu);
    uint64_t output = call_fpatan(0);
    reset_cpu_affinity(original);
    return output;
}

int cmd_wintest_main(int argc, char** argv)
{
    HANDLE driver_handle = open_driver();

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int number_of_cpus = info.dwNumberOfProcessors;

    for (int i = 0; i < number_of_cpus; i++)
    {
        logmsg("CPU %d: %llx, fpatan => %llx", i, rdmsr_on_cpu(driver_handle, MSR_AMD64_PATCH_LEVEL, i), test_fpatan_in_cpu(i));
    }

    return 0;
}
