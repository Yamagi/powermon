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

#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "cpuid.h"
#include "main.h"
#include "monitor.h"
#include "msr.h"


// --------


// Options given at command line.
cmdopts_t cmdopts;

// --------


/*
 * Cleans up at program exit.
 */
void cleanup(void) {
	close(cmdopts.fd);
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


// --------


/*
 * Print usage and exit.
 */
static void usage(void) {
	printf("Usage: raplctl [-d /dev/cpuctl0]\n\n");

	printf("Options:\n");
	printf(" -d: cpuctl(4) device to operate on\n");

	exit(1);
}


/*
 * Parses the command line options and sets defaults.
 */
static void parse_cmdoption(int argc, char *argv[]) {
	int32_t ch;

	while ((ch = getopt(argc, argv, "d:f:ht:")) != -1) {
		switch (ch) {
			case 'd':
				cmdopts.device = optarg;
				break;

			case 'f':
				cmdopts.cpufamily = optarg;
				break;

			case 't':
				if (!strcmp(optarg, "client")) {
					cmdopts.cputype = CLIENT;
				} else if (!strcmp(optarg, "server")) {
					cmdopts.cputype = SERVER;
				} else {
					cmdopts.cputype = UNKNOWN;
				}
				break;

			case '?':
			case 'h':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (!cmdopts.device) {
		cmdopts.device = "/dev/cpuctl0";

		if ((cmdopts.fd = open(cmdopts.device, O_RDWR)) == -1) {
			exit_error(1, "ERROR: Couldn't open %s: %s\n", 
					cmdopts.device, strerror(errno));
		}
	}

	if (!cmdopts.cpufamily) {
		cmdopts.cpufamily = getcpufamily();
	}

	if (!cmdopts.cputype) {
		cmdopts.cputype = getcputype();
	}
}


/*
 * Checks if the CPU is supported. If not an errir string is
 * printed and the program is aborted.
 */
static void checkcpu(void) {
	// CPU vendor must be Intel.
	char vendor[13];

	getcpuvendor(vendor);

	if (strcmp(vendor, "GenuineIntel")) {
		exit_error(1, "%s\n", "Only Intel CPUs are supported, sorry.");
	}


	// Is the CPU type supported?
	if (cmdopts.cputype == UNKNOWN) {
		exit_error(1, "%s\n", "CPU type is unknown, specify with -t.");
	}
}


// --------


/*
 * TODO: Program description.
 */
int main(int argc, char *argv[]) {
	// Register handlers
	atexit(cleanup);


	// If cpuctl(4) isn't loaded there's nothing we could do.
	struct stat sb;

	if (stat("/dev/cpuctl0", &sb) != 0) {
		exit_error(1, "%s\n", "ERROR: cpuctl(4) isn't available. Sorry.");
	}


	// Parse options.
	parse_cmdoption(argc, argv);


	// Check if CPU is supported.
	checkcpu();


	monitor();


	/*
	uint64_t blabb = read_msr(UNIT_MULTIPLIER);
	unit_msr_t blubb = *(unit_msr_t *)&blabb;
	printf("%lu\n", blubb.power);
	*/


	// Regular exit
	return 0;
}

