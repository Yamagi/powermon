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

#include <math.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "msr.h"


// --------


/*
 * Current state of energy counters (in joule).
 */
typedef struct energy_t {
	double dram;
	double pkg;
	double pp0;
	double pp1;
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
 * Powerlimits of the package.
 */
typedef struct powerlimits_t {
	uint64_t maximum_power;
	uint64_t minimum_power;
	uint64_t thermal_spec_power;
} powerlimits_t;

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
	// Package.
	uint64_t msr = getmsr(PKG_STATUS);
	status_msr_t status = *(status_msr_t *)&msr;
	energy->pkg = multi->energy * status.total_energy_consumed;

	// PP0.
	msr = getmsr(PP0_STATUS);
	status = *(status_msr_t *)&msr;
	energy->pp0 = multi->energy * status.total_energy_consumed;
     
	// PP1.
	if (options.cputype == CLIENT) {
		msr = getmsr(PP1_STATUS);
		status = *(status_msr_t *)&msr;
		energy->pp1 = multi->energy * status.total_energy_consumed;

		energy->dram = 0;
	}

	//DRAM.
	if (options.cputype == SERVER) {
		msr = getmsr(DRAM_STATUS);
		status = *(status_msr_t *)&msr;
		energy->dram = multi->energy * status.total_energy_consumed;

		energy->pp1 = 0;
	}
}


/*
 * Fills the given multipliers_t struct.
 *
 *  - *multipliers: Struct to fill.
 */
static void getmultipliers(multipliers_t *multipliers) {
	uint64_t msr = getmsr(UNIT_MULTIPLIER);
	unit_msr_t units = *(unit_msr_t *)&msr;

	multipliers->energy = 1.0 / (double)B2POW(units.energy);
	multipliers->power = 1.0 / (double)B2POW(units.power);
	multipliers->time = 1.0 / (double)B2POW(units.time);
}


/*
 * Fills the given powerlimits_t structs.
 * 
 *  - *limits: Struct to fill.
 */
static void getpowerlimits(powerlimits_t *limits) {
	uint64_t msr = getmsr(PKG_INFO);
	info_msr_t values = *(info_msr_t *)&msr;

	limits->maximum_power = values.maximum_power / 10;
	limits->minimum_power = values.minimum_power / 10;
	limits->thermal_spec_power = values.thermal_spec_power / 10;
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
	wrap->throttle = (double)(multi->time * 4294967295); // 2^32-1
}


// --------


/*
 * Prints a nice status monitor until the user interrupts us.
 */
void monitor(void) {
	// Initialize curses.
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	nodelay(stdscr, TRUE);
	curs_set(0);

	// Initialize multipliers.
	multipliers_t multipliers;
    getmultipliers(&multipliers);


	// Initiale package limits.
	powerlimits_t powerlimits;
	getpowerlimits(&powerlimits);
	uint64_t powerlimit = powerlimits.thermal_spec_power < powerlimits.maximum_power
		? powerlimits.maximum_power : powerlimits.thermal_spec_power;


	// Intialize wrap arounds.
	wraparound_t wraparound;
	getwraparounds(&multipliers, &wraparound);


	// Counters.
	energy_t cur_energy;
	energy_t last_energy;
	energy_t total_energy = {0, 0, 0, 0};
	energy_t delta_energy = {0, 0, 0, 0};
	uint32_t count = 0;

	getenergy(&multipliers, &last_energy);

	while (1) {
		getenergy(&multipliers, &cur_energy);

		// PKG.
		if (cur_energy.pkg < last_energy.pkg) {
			delta_energy.pkg += wraparound.status - last_energy.pkg;
			delta_energy.pkg += cur_energy.pkg;

			total_energy.pkg += wraparound.status - last_energy.pkg;
			total_energy.pkg += cur_energy.pkg;
		} else {
			delta_energy.pkg += cur_energy.pkg - last_energy.pkg;
			total_energy.pkg += cur_energy.pkg - last_energy.pkg;
		}

		// PP0.
		if (cur_energy.pp0 < last_energy.pp0) {
			delta_energy.pp0 += wraparound.status - last_energy.pp0;
			delta_energy.pp0 += cur_energy.pp0;

			total_energy.pp0 += wraparound.status - last_energy.pp0;
			total_energy.pp0 += cur_energy.pp0;
		} else {
			delta_energy.pp0 += cur_energy.pp0 - last_energy.pp0;
			total_energy.pp0 += cur_energy.pp0 - last_energy.pp0;
		}

		// PP1.
		if (cur_energy.pp1 < last_energy.pp1) {
			delta_energy.pp1 += wraparound.status - last_energy.pp1;
			delta_energy.pp1 += cur_energy.pp1;

			total_energy.pp1 += wraparound.status - last_energy.pp1;
			total_energy.pp1 += cur_energy.pp1;
		} else {
			delta_energy.pp1 += cur_energy.pp1 - last_energy.pp1;
			total_energy.pp1 += cur_energy.pp1 - last_energy.pp1;
		}

		// DRAM.
		if (cur_energy.dram < last_energy.dram) {
			delta_energy.dram += wraparound.status - last_energy.dram;
			delta_energy.dram += cur_energy.dram;

			total_energy.dram += wraparound.status - last_energy.dram;
			total_energy.dram += cur_energy.dram;
		} else {
			delta_energy.dram += cur_energy.dram - last_energy.dram;
			total_energy.dram += cur_energy.dram - last_energy.dram;
		}

		last_energy = cur_energy;
		count++;

		if (count == 20) {
			/* The standard terminal is 79 characters wide. We omit one
			   character at each end, the usable space is 77 characters.
			   We loose 2 characters to the bars end markers, and 8 to
			   the current power consumtion display. So we can work with
			   67 characters. At the left we're starting at character 10
			   and at the right we end at character 76. */


			// Avoid artifacts.
			clear();

			// Header.
			char header[78];

			snprintf(header, sizeof(header), "%s", options.cpumodel);
			mvprintw(0, 38 - (strlen(header) / 2), header);

			snprintf(header, sizeof(header), "(Arch: %s, Limit: %luW)",
					options.cpufamily, powerlimit);
			mvprintw(1, 38 - (strlen(header) / 2), header);

			// Total power consumption.
			uint32_t num_load = floor((67.0 / powerlimit) * delta_energy.pkg);
			mvprintw(5, 1, "%6.2fW [", delta_energy.pkg);

			uint32_t i;

			for (i = 0; i < num_load && i <= 66; i++) {
				mvprintw(5, i + 10, "=");
			}

			mvprintw(5, i + 10, ">");
			mvprintw(5, 77, "]");

			// Package power consumption.
			attron(A_BOLD);
			mvprintw(9, 1, "Package:");
			attroff(A_BOLD);
			mvprintw(10, 1, "Current: %.2fJ", delta_energy.pkg);
			mvprintw(11, 1, "Total: %.2fJ", total_energy.pkg);

			// Uncore power consumption.
			attron(A_BOLD);
			mvprintw(9, 20, "Uncore:");
			attroff(A_BOLD);
			mvprintw(10, 20, "Current: %.2fJ", delta_energy.pkg - 
					(delta_energy.pp0 + delta_energy.pp1));
			mvprintw(11, 20, "Total: %.2fJ", total_energy.pkg -
					(total_energy.pp0 + total_energy.pp1));

			// x86 cores power consumption.
			attron(A_BOLD);
			mvprintw(9, 40, "x86 Cores:");
			attroff(A_BOLD);
			mvprintw(10, 40, "Current: %.2fJ", delta_energy.pp0);
			mvprintw(11, 40, "Total: %.2fJ", total_energy.pp0);

			if (options.cputype == CLIENT) {
				// GPU power consumption.
				attron(A_BOLD);
				mvprintw(9, 60, "GPU:");
				attroff(A_BOLD);
				mvprintw(10, 60, "Current: %.2fJ", delta_energy.pp1);
				mvprintw(11, 60, "Total: %.2fJ", total_energy.pp1);
			} else if (options.cputype == SERVER) {
				// DRAM power consumption.
				attron(A_BOLD);
				mvprintw(9, 60, "DRAM:");
				attroff(A_BOLD);
				mvprintw(10, 60, "Current: %.2fJ", delta_energy.dram);
				mvprintw(11, 60, "Total: %.2fJ", total_energy.dram);
			}

			// Print the new data
			refresh();

			// Quit?
			int32_t ch;

			while ((ch = getch()) != ERR) {
				switch (ch) {
					case 'q':
					case 'Q':
					case 27:
						options.stop = 1;
				}
			}

			if (options.stop) {
				break;
			}

			// Cleanup.
			delta_energy.pkg = 0;
			delta_energy.pp0 = 0;
			delta_energy.pp1 = 0;
			delta_energy.dram = 0;
			count = 0;
		}

		// One sample every 50 milliseconds.
		usleep(50 * 1000);
	}

	// Quit curses.
	endwin();
}


// --------

