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
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "cpuid.h"
#include "display.h"
#include "main.h"
#include "msr.h"


// --------


// Options given at command line.
options_t options;

// --------


/*
 * Cleans up at program exit.
 */
void cleanup(void) {
	close(options.fd);
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
 * Breaks the main loop when a signal is caught.
 */
void sighandler(int sig) {
	options.stop = sig;
}


// --------


/*
 * Print usage and exit.
 */
static void usage(void) {
	printf("Usage: powermon [-d device] [-f family] [-m model] [-t type] [-v vendor]\n\n");

	printf("Options:\n");
	printf(" -d: cpuctl(4) device.\n");
	printf(" -f: CPU family.\n");
	printf(" -m: CPU model.\n");
	printf(" -t: CPU type.\n");
	printf(" -v: CPU vendor.\n");

	exit(1);
}


/*
 * Parses the command line options and sets defaults.
 */
static void parse_cmdoption(int argc, char *argv[]) {
	int32_t ch;

	while ((ch = getopt(argc, argv, "d:f:hm:t:v:")) != -1) {
		switch (ch) {
			case 'd':
				options.device = optarg;
				break;

			case 'f':
				options.cpufamily = optarg;
				break;

			case 'm':
				strlcpy(options.cpumodel, optarg, sizeof(options.cpumodel));
				break;

			case 't':
				if (!strcmp(optarg, "client")) {
					options.cputype = CLIENT;
				} else if (!strcmp(optarg, "server")) {
					options.cputype = SERVER;
				} else {
					options.cputype = UNKNOWN;
				}
				break;

			case 'v':
				strlcpy(options.cpuvendor, optarg, sizeof(options.cpuvendor));
				break;

			case '?':
			case 'h':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (!options.device) {
		options.device = "/dev/cpuctl0";
	}

	if ((options.fd = open(options.device, O_RDWR)) == -1) {
		exit_error(1, "ERROR: Couldn't open %s: %s\n", 
				options.device, strerror(errno));
	}

	if (!options.cpufamily) {
		options.cpufamily = getcpufamily();
	}

	if (!options.cputype) {
		options.cputype = getcputype();

		// Try to determine based on MSRs.
		if (options.cputype == UNKNOWN) {
			if (checkmsr(PP1_STATUS)){
				options.cputype = CLIENT;
			} else if (checkmsr(DRAM_STATUS)) {
				options.cputype = SERVER;
			}
		}
	}

	if (!strlen(options.cpuvendor)) {
		getcpuvendor(options.cpuvendor, sizeof(options.cpuvendor));
	}

	if (!strlen(options.cpumodel)) {
		getcpumodel(options.cpumodel, sizeof(options.cpumodel));
	}
}


/*
 * Checks if the CPU is supported. If not an error string is
 * printed and the program is aborted.
 */
static void checkcpu(void) {
	if (strcmp(options.cpuvendor, "GenuineIntel")) {
		exit_error(1, "%s\n", "Only Intel CPUs are supported, sorry.");
	}

	if (options.cputype == UNKNOWN) {
		exit_error(1, "%s\n", "CPU type is unknown, specify with -t.");
	}

	if (options.cputype == UNSUPPORTED) {
		exit_error(1, "%s\n", "CPU is unsupported");
	}
}


// --------


/*
 * powermon is a top-like tool to show realtime power statistics.
 * The data is retrieved from the RAPL interface, exposed through
 * MSR. Only Intel CPUs starting with Sandy Bridge support the
 * interface, other models and vendors are not supported. Client
 * aka desktop CPUs expose the GPU power consumption, server CPUs
 * and it's derivates (for example Socket 2011 desktop models)
 * the DRAM power consumtion instead.
 *
 * All the user needs to do is to start the program with 'powermon'.
 * The command line options are only necessary if the tools in not
 * able determine parameters automaticaly, maybe because the CPU
 * is unknown to it.
 */
int main(int argc, char *argv[]) {
	// Register handlers.
	atexit(cleanup);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);


	// If cpuctl(4) isn't loaded there's nothing we could do.
	struct stat sb;

	if (stat("/dev/cpuctl0", &sb) != 0) {
		exit_error(1, "%s\n", "ERROR: cpuctl(4) isn't available. Sorry.");
	}


	// Parse options.
	parse_cmdoption(argc, argv);


	// Check if CPU is supported.
	checkcpu();


	// Setup curses an start the main loop.
	display();


	// Regular exit.
	return 0;
}

