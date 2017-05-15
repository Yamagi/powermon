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

#ifndef MAIN_H_
#define MAIN_H_


// --------


// CPU types.
typedef enum cputype_e {
	CLIENT = 0,
	SERVER,
	UNKNOWN,
	UNSUPPORTED
} cputype_e;


// Options given at command line.
typedef struct options_t {
	// cpuctl device to operate on.
	const char *device;

	// FD to cpuctl device.
	uint32_t fd;

	// CPU family string.
	const char *cpufamily;

	// CPU type.
	cputype_e cputype;

	// CPU vendor
	char cpuvendor[13];

	// CPU model
	char cpumodel[49];

	// If set the mainloop is broken.
	uint32_t stop;
} options_t;

extern options_t options;


// --------


/*
 * Prints a message to stderr and exits with error code.
 *
 *  - code: Exit code.
 *  - fmt: Format of message.
 *  - ...: Message list.
 */
void exit_error(int32_t code, const char *fmt, ...);


// --------

#endif // MAIN_H_

