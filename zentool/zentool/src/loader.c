/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "options.h"
#include "util.h"
#include "win.h"
#include "win_getopt.h"
#include "win_kernel.h"

#define MSR_AMD64_PATCH_LEVEL 0x0000008b
#define MSR_AMD64_PATCH_LOADER 0xc0010020

#define fatal(...) err(EXIT_FAILURE, __VA_ARGS__)
#define fatalx(...) errx(EXIT_FAILURE, __VA_ARGS__)

#define PAGE_ROUND_UP(x) ((((uint64_t)(x)) + (PAGE_SIZE-1))  & (~(PAGE_SIZE-1)))

static int retry;

static const struct option kLongOpts[] = {
    {          "help", false, NULL, 'h' },
    {          "core",  true, NULL, 'c' },
    {           "cpu",  true, NULL, 'c' },
    {         "retry", false, &retry, true },
    {0},
};

static const char *kOptHelp[] = {
    "print this message",
    "select cpu core number",
    "alias for --core",
    "keep trying on failure",
};

static void print_usage()
{
    logmsg("attempt to apply a cpu microcode update");

    print_usage_generic("load", "FILE", kLongOpts, kOptHelp);

    logmsg("This command requires admin privileges and driver loaded.");
}

#define MAX_CONTIG_ATTEMPT 1024

int cmd_load_main(int argc, char *argv[])
{
    int c;
    int longopt;
    int cpu = -1;

    reset_getopt();

    while ((c = getopt_long(argc, argv, "hp:c:", kLongOpts, &longopt)) != -1) {
    switch (c) {
            case 'h': print_usage();
                      return 0;
            case 'c': cpu = strtoul(optarg, NULL, 0);
                      break;
            case '?': print_usage();
                      errx(EXIT_FAILURE, "invalid options");
        }
    }

    if (argc == optind) {
        print_usage();
        errx(EXIT_FAILURE, "you must specify an update file to load");
    }

    int ucode_fd = open(argv[optind], O_RDONLY | O_BINARY);
    if (ucode_fd < 0) {
        fatal("failed to open ucode");
    }

    if (cpu < 0) {
        errx(EXIT_FAILURE, "you need to specify a core number, e.g. --cpu=2");
    }

    struct stat statbuf;

    if (fstat(ucode_fd, &statbuf) < 0) {
        fatal("stat ucode");
    }

    dbgmsg("reading %ld bytes", statbuf.st_size);

    HANDLE driver_handle = open_driver();

    logmsg("old ucode patch on cpu %d: %#llx",
            cpu,
            rdmsr_on_cpu(driver_handle, MSR_AMD64_PATCH_LEVEL, cpu));

    while (true) 
    {
        uint64_t physical_address = 0x0;
        uint8_t* kernel_data = mmap_contig(driver_handle, statbuf.st_size, &physical_address);
        uint8_t* local_data = (uint8_t*)malloc(statbuf.st_size);

        if (!kernel_data)
        {
            fatalx("mmap memory");
        }

        _lseeki64(ucode_fd, 0, SEEK_SET);

        if (_read(ucode_fd, local_data, statbuf.st_size) != statbuf.st_size)
        {
            fatal("read ucode");
        }

        write_physical(driver_handle, physical_address, local_data, statbuf.st_size);

        uint64_t patch_physaddr = physical_address;
        uint64_t patch_kva = kernel_data;

        dbgmsg("patch at %#llx in physmem", patch_physaddr);
        dbgmsg("patch at %#llx in virtmem", patch_kva);

        if (wrmsr_on_cpu(driver_handle, MSR_AMD64_PATCH_LOADER, cpu, patch_kva) != true) {
            logerr("wrmsr failed, the CPU did not accept the update.");

            // Data no longer needed.
            munmap_contig(driver_handle, kernel_data);

            // Optionally repeat forever (used for testing).
            if (retry) continue;

            logerr("    - Check the signature, `zentool verify`                         \n"
                   "    - Check the cpuid matches, `zentool print`                      \n"
                   "    - Is the update revision >= current revision? Try `rdmsr 0x8b`  \n"
                   "    - This could be transient, try again?                             ");
            fatalx("wrmsr failed, the CPU did not accept the update.");
        }

        munmap_contig(driver_handle, kernel_data);
        break;
    }

    logmsg("new ucode patch on cpu %d: %#llx",
            cpu,
            rdmsr_on_cpu(driver_handle, MSR_AMD64_PATCH_LEVEL, cpu));
    return 0;
}