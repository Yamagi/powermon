/*
 * Copyright (c) 2017 Y. Burmeister
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */ 

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cpuctl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include "cpuid.h"


// --------


/*
 * Gives the CPU model.
 *
 *  - model: Pointer to a char array with minimum length 49.
 */
void getcpumodel(char *model) {
	cpuctl_cpuid_count_args_t cpuid;


	// Check if supported.
	cpuid.level = 0x80000000;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	if (cpuid.data[0] < 0x80000004) {
		strcpy(model, "Unknown CPU Model");
		return;
	}

	// Block 0
	cpuid.level = 0x80000002;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	memcpy(model, cpuid.data, sizeof(uint32_t));
	memcpy(model + 4, cpuid.data + 1, sizeof(uint32_t));
	memcpy(model + 8, cpuid.data + 2, sizeof(uint32_t));
	memcpy(model + 12, cpuid.data + 3, sizeof(uint32_t));


	// Block 1
	cpuid.level = 0x80000003;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	memcpy(model + 16 , cpuid.data, sizeof(uint32_t));
	memcpy(model + 20, cpuid.data + 1, sizeof(uint32_t));
	memcpy(model + 24, cpuid.data + 2, sizeof(uint32_t));
	memcpy(model + 28, cpuid.data + 3, sizeof(uint32_t));


	// Block 2
	cpuid.level = 0x80000004;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	memcpy(model + 32 , cpuid.data, sizeof(uint32_t));
	memcpy(model + 36, cpuid.data + 1, sizeof(uint32_t));
	memcpy(model + 40, cpuid.data + 2, sizeof(uint32_t));
	memcpy(model + 44, cpuid.data + 3, sizeof(uint32_t));

}

/*
 * Gives the CPU vendor.
 *
 *  - vendor: Pointer to a char array with minimum length 13.
 */
void getcpuvendor(char *vendor) {
	cpuctl_cpuid_count_args_t cpuid;

	cpuid.level = 0x0;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	// Yes, this is ugly.
	memcpy(vendor, cpuid.data + 1, sizeof(uint32_t));
	memcpy(vendor + 4, cpuid.data + 3, sizeof(uint32_t));
	memcpy(vendor + 8, cpuid.data + 2, sizeof(uint32_t));
	vendor[12] = '\0';
}


/*
 * Returns the CPU family.
 */
const char *getcpufamily(void) {
	cpuctl_cpuid_count_args_t cpuid;

	cpuid.level = 0x1;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	// CPU identifiers are taken from  Intel® 64 and IA-32
    // Architectures Software Developer Manual: Vol 3, Table 35-1
	switch (cpuid.data[0] & 0xfffffff0) {
		// Silvermont
		case 0x506d0:
			return "Silvermont";

		// Airmont
		case 0x406c0:
			return "Airmont";

		// Goldmont
		case 0x506c0:
		case 0x506f0:
			return "Goldmont";

		// Sandy Bridge
		case 0x206a0:
		case 0x206d0:
			return "Sandy Bridge";

		// Ivy Bridge
		case 0x306a0:
		case 0x306e0:
			return "Ivy Bridge";

		// Haswell
		case 0x40660:
		case 0x40650:
		case 0x306c0:
		case 0x306f0:
			return "Haswell";

		// Broadwell
		case 0x306d0:
		case 0x40670:
		case 0x406f0:
		case 0x50660:
			return "Broadwell";

		// Skylake
		case 0x406e0:
		case 0x506e0:
		case 0x50650:
			return "Skylake";

		// Kaby Lake
		case 0x806e0:
		case 0x60600:
			return "Kaby Lake";

		default:
			return "Unknown";
			break;
	}
}


/*
 * Returns the CPU type.
 */
cputype_e getcputype(void) {
	cpuctl_cpuid_count_args_t cpuid;

	cpuid.level = 0x1;
	cpuid.level_type = 0;

	if (ioctl(cmdopts.fd, CPUCTL_CPUID_COUNT, &cpuid) == -1) {
		exit_error(1, "ERROR: ioctl CPUCTL_CPUID_COUNT failed: %s\n", errno);
	}

	// CPU identifiers are taken from  Intel® 64 and IA-32
    // Architectures Software Developer Manual: Vol 3, Table 35-1
	switch (cpuid.data[0] & 0xfffffff0) {
		// Pentium
		case 0x00510:
		case 0x00520:
		case 0x00540:
			return UNSUPPORTED;

		// P6
		case 0x00610:
		case 0x00630:
		case 0x00650:
		case 0x70600:
		case 0x00680:
		case 0x006a0:
		case 0x006b0:
		case 0x00690:
		case 0x006d0:
			return UNSUPPORTED;

		// Netburst
		case 0x00f00:
		case 0x0f010:
		case 0x00f20:
		case 0x00f30:
		case 0x00f40:
		case 0x00f60:
			return UNSUPPORTED;

		// Atom
		case 0x106c0:
		case 0x20660:
		case 0x20670:
		case 0x30650:
		case 0x30660:
		case 0x40600:
		case 0x30670:
		case 0x406a0:
		case 0x506a0:
			return UNSUPPORTED;

		// Silvermont
		case 0x506d0:
			return CLIENT;

		// Airmont
		case 0x406c0:
			return CLIENT;

		// Goldmont
		case 0x506c0:
		case 0x506f0:
			return CLIENT;

		// Core / Core2
		case 0x006f0:
		case 0x10670:
		case 0x106d0:
			return UNSUPPORTED;

		// Nehalem / Westmere
		case 0x106e0:
		case 0x106f0:
		case 0x20650:
		case 0x206c0:
		case 0x206e0:
		case 0x206f0:
			return UNSUPPORTED;

		// Sandy Bridge client
		case 0x206a0:
			return CLIENT;

		// Sandy Bridge server
		case 0x206d0:
			return SERVER;

		// Ivy Bridge client
		case 0x306a0:
			return CLIENT;

		// Ivy Bridge server
		case 0x306e0:
			return SERVER;

		// Haswell client
		case 0x40660:
		case 0x40650:
		case 0x306c0:
			return CLIENT;

		// Haswell server
		case 0x306f0:
			return SERVER;

		// Broadwell client
		case 0x306d0:
		case 0x40670:
			return CLIENT;

		// Broadwell server
		case 0x406f0:
		case 0x50660:
			return SERVER;

		// Skylake client
		case 0x406e0:
		case 0x506e0:
			return CLIENT;

		// Skylake server
		case 0x50650:
			return SERVER;

		// Kaby Lake
		case 0x806e0:
		case 0x60600:
			return CLIENT;

		// Unknown
		default:
			return UNKNOWN;
	}
}


// --------

