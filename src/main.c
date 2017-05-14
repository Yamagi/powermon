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
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "main.h"
#include "msr.h"


// --------


// Options given at command line.
cmdopts_t cmdopts;

// --------


/*
 * Prints a message to stderr and exits with error code.
 *
 *  - msg: Message to print.
 *  - code: Exit code,
 */
void exit_error(const char *msg, int32_t code) {
	fprintf(stderr, "ERROR: %s\n", msg);
	exit(code);
}


/*
 * Print usage and exit.
 */
void usage(void) {
	printf("Usage: raplctl [-d /dev/cpuctl0]\n\n");

	printf("Options:\n");
	printf(" -d: cpuctl(4) device to operate on\n");

	exit(1);
}


/*
 * Parses the command line options and sets defaults.
 */
void parse_cmdoption(int argc, char *argv[]) {
	int32_t ch;

	while ((ch = getopt(argc, argv, "d:h")) != -1) {
		switch (ch) {
			case 'd':
				cmdopts.device = optarg;
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
	}
}


// --------


/*
 * TODO: Program description.
 */
int main(int argc, char *argv[]) {
	// If cpuctl(4) isn't loaded there's nothing we could do.
	struct stat sb;

	if (stat("/dev/cpuctl0", &sb) != 0) {
		exit_error("cpuctl(4) isn't available. Sorry.", 1);
	}


	// Parse options.
	parse_cmdoption(argc, argv);


	// Regular exit
	return 0;
}

