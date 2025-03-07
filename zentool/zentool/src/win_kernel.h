#ifndef WIN_KERNEL_H
#define WIN_KERNEL_H

#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

DWORD_PTR set_cpu_affinity(int cpu);
void reset_cpu_affinity(DWORD_PTR previous_affinity_mask);
HANDLE open_driver(void);
void close_driver(HANDLE driver_handle);
void submit_ioctl(HANDLE driver_handle, DWORD ioctl_code, void* input, DWORD input_size, void* output, DWORD output_size);
void allocate_contiguous_memory(HANDLE driver_handle, uint64_t size, uint64_t* out_physical_address, uint64_t* out_virtual_address);
void read_physical(HANDLE driver_handle, uint64_t physical_address, void* buffer, size_t size);
void write_physical(HANDLE driver_handle, uint64_t physical_address, void* buffer, size_t size);
uint64_t rdmsr_on_cpu(HANDLE driver_handle, uint32_t reg, int cpu);
bool wrmsr_on_cpu(HANDLE driver_handle, uint32_t reg, int cpu, uint64_t val);
void* mmap_contig(HANDLE driver_handle, size_t size, uint64_t* physical_address);
void munmap_contig(HANDLE driver_handle, void* data);

#endif