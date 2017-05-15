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
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "msr.h"


// --------

/*
 * Current state of energy counters (in joule).
 */
typedef struct energy_t {
	double pkg;
	double pp0;
	double pp1;
	double ram;
} energy_t;

/*
 * Multipliers used to calculate actual values from the raw data.
 */
typedef struct multipliers_t {
	double energy;
	double power;
	double time;
} multipliers_t;

/*
 * Maximum value of *_STATUS und *_THROTTLE MSR.
 */
typedef struct wraparound_t {
	double status;
	double throttle;
} wraparound_t;


// --------


/*
 * Fills the given energy_t struct with the current state
 * of the energy counter. The raw values are converted to
 * joule.
 *
 *  - *energy: Struct to fill.
 *  - *multi: Struct to get correction multipliers from.
 */
static void getenergy(multipliers_t *multi, energy_t *energy) {
	status_msr_t status;
	uint64_t msr;

	// Package
	msr = read_msr(PKG_STATUS);
	status = *(status_msr_t *)&msr;
	energy->pkg = multi->energy * status.total_energy_consumed;

	// PP0
	msr = read_msr(PP0_STATUS);
	status = *(status_msr_t *)&msr;
	energy->pp0 = multi->energy * status.total_energy_consumed;
     
	// PP1
	if (cmdopts.cputype == CLIENT) {
		msr = read_msr(PP1_STATUS);
		status = *(status_msr_t *)&msr;
		energy->pp1 = multi->energy * status.total_energy_consumed;

		energy->ram = 0;
	}

	//DRAM
	if (cmdopts.cputype == SERVER) {
		msr = read_msr(DRAM_STATUS);
		status = *(status_msr_t *)&msr;
		energy->ram = multi->energy * status.total_energy_consumed;

		energy->pp1 = 0;
	}
}


/*
 * Fills the given multipliers_t struct.
 *
 *  - *multipliers: Struct to fill.
 */
static void getmultipliers(multipliers_t *multipliers) {
	uint64_t msr;
	unit_msr_t units;

	msr = read_msr(UNIT_MULTIPLIER);
	units = *(unit_msr_t *)&msr;

	multipliers->energy = 1.0 / (double)B2POW(units.energy);
	multipliers->power = 1.0 / (double)B2POW(units.power);
	multipliers->time = 1.0 / (double)B2POW(units.time);
}


/*
 * Fills the given wraparound_t struct based upon the values
 * in the given multipliers_t.
 *
 *  - *multi: multipliers_t struct to read values from.
 *  - *wrap: Struct to fill.
 */
static void getwraparounds(multipliers_t *multi, wraparound_t *wrap) {
	wrap->status = (double)(multi->energy * 4294967295); // 2^32-1
	wrap->throttle= (double)(multi->time* 4294967295); // 2^32-1
}


// --------


/*
 * Prints a nice status monitor until the user interrupts us.
 */
void monitor(void) {
	// Initialize multipliers
	multipliers_t multipliers;
    getmultipliers(&multipliers);


	// Intialize wrap arounds
	wraparound_t wraparound;
	getwraparounds(&multipliers, &wraparound);


	// Counters
	energy_t cur_energy;
	energy_t last_energy;
	energy_t total_energy = {0};
	energy_t delta_energy = {0};
	uint32_t count = 0;

	getenergy(&multipliers, &last_energy);

	while (1) {
		getenergy(&multipliers, &cur_energy);

		// PKG
		if (cur_energy.pkg < last_energy.pkg) {
			delta_energy.pkg += wraparound.status - last_energy.pkg;
			delta_energy.pkg += cur_energy.pkg;

			total_energy.pkg += wraparound.status - last_energy.pkg;
			total_energy.pkg += cur_energy.pkg;
		} else {
			delta_energy.pkg += cur_energy.pkg - last_energy.pkg;
			total_energy.pkg += cur_energy.pkg - last_energy.pkg;
		}

		// PP0
		if (cur_energy.pp0 < last_energy.pp0) {
			delta_energy.pp0 += wraparound.status - last_energy.pp0;
			delta_energy.pp0 += cur_energy.pp0;

			total_energy.pp0 += wraparound.status - last_energy.pp0;
			total_energy.pp0 += cur_energy.pp0;
		} else {
			delta_energy.pp0 += cur_energy.pp0 - last_energy.pp0;
			total_energy.pp0 += cur_energy.pp0 - last_energy.pp0;
		}

		// PP1
		if (cur_energy.pp1 < last_energy.pp1) {
			delta_energy.pp1 += wraparound.status - last_energy.pp1;
			delta_energy.pp1 += cur_energy.pp1;

			total_energy.pp1 += wraparound.status - last_energy.pp1;
			total_energy.pp1 += cur_energy.pp1;
		} else {
			delta_energy.pp1 += cur_energy.pp1 - last_energy.pp1;
			total_energy.pp1 += cur_energy.pp1 - last_energy.pp1;
		}

		// DRAM
		if (cur_energy.ram < last_energy.ram) {
			delta_energy.ram += wraparound.status - last_energy.ram;
			delta_energy.ram += cur_energy.ram;

			total_energy.ram += wraparound.status - last_energy.ram;
			total_energy.ram += cur_energy.ram;
		} else {
			delta_energy.ram += cur_energy.ram - last_energy.ram;
			total_energy.ram += cur_energy.ram - last_energy.ram;
		}

		last_energy = cur_energy;
		count++;

		if (count == 20) {
			printf("Package   | Total: %fJ, Current: %fJ\n", 
					total_energy.pkg, delta_energy.pkg);
			printf("x86 Cores | Total: %fJ, Current: %fJ\n", 
					total_energy.pp0, delta_energy.pp0);
			printf("GPU       | Total: %fJ, Current: %fJ\n", 
					total_energy.pp1, delta_energy.pp1);
			printf("DRAM      | Total: %fJ, Current: %fJ\n", 
					total_energy.ram, delta_energy.ram);
			printf("Uncore    | Total: %fJ, Current: %fJ\n\n", 
					total_energy.pkg - (total_energy.pp0 + total_energy.pp1),
					delta_energy.pkg - (delta_energy.pp0 + delta_energy.pp1));

			delta_energy.pkg = 0;
			delta_energy.pp0 = 0;
			delta_energy.pp1 = 0;
			delta_energy.ram = 0;
			count = 0;
		}

		// One sample every 50 milliseconds.
		usleep(50 * 1000);
	}
}


// --------

