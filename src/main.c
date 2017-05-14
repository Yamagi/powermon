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
#include <sys/stat.h>

#include "msr.h"


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


// --------


/*
 * TODO: Program description.
 */
int main(int argc, char *argv[]) {
	// If cpuctl(4) isn't loaded there's nothing we could do,
	struct stat sb;

	if (stat("/dev/cpuctl0", &sb) != 0) {
		exit_error("cpuctl(4) isn't available. Sorry.", 1);
	}

	return 0;
}

