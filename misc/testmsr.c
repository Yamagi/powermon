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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/cpuctl.h>
#include <sys/ioctl.h>


// ----


// The unit multiplier. It is used to calculate the factor to convert
// raw values into actual Joules. Additionally the MSR wraparound is 
// calculated from it.
#define UNIT_MULTIPLIER 0x606

// Power consumed since reboot or last register wrap around.
#define PKG_STATUS   0x611

// Power consumption of x86 cores since reboot or register wrap around.
#define PP0_STATUS 0x639

// Replacement for pow() with a base of 2.
#define B2POW(e) (((e) == 0) ? 1 : (2 << ((e) - 1)))


// ----


// FD for /dev/cpuctl0
uint32_t fd;

// Quit with next iteration=
bool quit;

// *_LIMIT MSR structure (PP0, PP1 and DRAM).
typedef struct limit_msr_t {
	uint64_t power_limit         : 15;
	uint64_t limit_enabled       : 1;
	uint64_t clamp_enabled       : 1;
	uint64_t limit_time_window_y : 5; 
	uint64_t limit_time_window_f : 2;
	uint64_t                     : 7;
	uint64_t lock_enabled        : 1;
	uint64_t                     : 32;
} limit_msr_t;

// *_STATUS MSR structure.
typedef struct status_msr_t {
	uint64_t total_energy_consumed : 32;
	uint64_t                       : 32;
} status_msr_t;

// UNIT_MULTIPLIER MSR structure.
typedef struct unit_msr_t {
	uint64_t power  : 4;
	uint64_t        : 4;
	uint64_t energy : 5;
	uint64_t        : 3;
	uint64_t time   : 4;
	uint64_t        : 32;         
	uint64_t        : 12;
} unit_msr_t;


// ----


/*
 * Cleans up at program exit.
 */
void cleanup(void) {
	closefrom(0);
}


/*
 * Prints a message to stderr and exits with error code.
 *
 *  - code: Exit code.
 *  - fmt: Format of message.
 *  - ...: Message list.
 */
void exit_error(int32_t code, const char *fmt, ...) {
	va_list vl;

	va_start(vl, fmt);
	vfprintf(stderr, fmt, vl);

	exit(code);
}


/*
 * Checks if the given MSR exists.
 *
 * - msr: MSR to check.
 */
bool checkmsr(int32_t msr) {
	cpuctl_msr_args_t args;

	args.msr = msr;

	if (ioctl(fd, CPUCTL_RDMSR, &args) == -1)
	{
		return false;
	}

	return true;
}


/*
 * Reads the given MSR and returns it's data.
 *
 *  - msr: MSR to read.
 */
uint64_t getmsr(int32_t msr) {
	cpuctl_msr_args_t args;

	args.msr = msr;

	if (ioctl(fd, CPUCTL_RDMSR, &args) == -1)
	{
		exit_error(1, "ERROR: ioctl CPUCTL_RDMSR failed: %i\n", errno);
	}

	return args.data;
}


/*
 * Breaks the main loop when a signal is caught.
 */
void sighandler(int sig) {
	quit = true;
}


// ----


int main(void) {
	// Register handlers
	atexit(cleanup);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	// Get FD
	if ((fd = open("/dev/cpuctl0", O_RDWR)) == -1) {
		exit_error(1, "ERROR: Couldn't open %s: %s\n", 
				"/dev/cpuctl0", strerror(errno));
	}

	// Check if CPU is supported
	if (!checkmsr(UNIT_MULTIPLIER)) {
		exit_error(1, "MSR UNIT_MULTIPLIER doesn't exist. Sorry.");
	}

	// Get and print correction values
	uint64_t msr = getmsr(UNIT_MULTIPLIER);
	unit_msr_t units = *(unit_msr_t *)&msr;
	printf("Raw correction value: 0x%xh\n", units.energy);

	double correction = 1.0f / (double)B2POW(units.energy);
	printf("Calculated correction value: %f\n\n", correction);

	// Get and print PACKAGE and PP0 power counters
	printf("--------\n");

	for (int i = 0; i < 10; i++) {
		// Package
		uint64_t msr = getmsr(PKG_STATUS);
		status_msr_t status = *(status_msr_t *)&msr;
		printf("Raw package power consumption: 0x%xh\n",
			   	status.total_energy_consumed);
		printf("Calculated package power consumtion: %f\n\n",
				correction * status.total_energy_consumed);

		// x86 cores
		msr = getmsr(PP0_STATUS);
		status = *(status_msr_t *)&msr;
		printf("Raw x86 cores power consumption: 0x%xh\n",
				status.total_energy_consumed);
		printf("Calculated x86 cores power consumtion: %f\n",
				correction * status.total_energy_consumed);

		printf("--------\n");

		if (quit) {
			break;
		}

		sleep(2);
	}

	return 0;
}

