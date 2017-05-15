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
 * Powerlimits of the package.
 */
typedef struct packagelimits_t {
	uint64_t limit1;
	uint64_t limit2;
} packagelimits_t;

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
 * Fills the given packagelimits_t structs.
 * 
 *  - *limits: Struct to fill.
 */
static void getpackagelimits(packagelimits_t *limits) {
	uint64_t msr;
	pkg_limit_msr_t values;

	msr = read_msr(PKG_INFO);
	values = *(pkg_limit_msr_t *)&msr;

	limits->limit1 = values.power_limit_1 / 10;
	limits->limit2 = values.power_limit_2 / 10;
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
	// Initilize curses
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	nodelay(stdscr, TRUE);
	curs_set(0);

	// Initialize multipliers
	multipliers_t multipliers;
    getmultipliers(&multipliers);


	// Initiale package limits
	packagelimits_t packagelimits;
	getpackagelimits(&packagelimits);


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
			char header[78];
			uint32_t i;
			uint32_t ch;
			uint32_t num_load;
			uint32_t stop = 0;


			/* The standard terminal is 79 characters wide. We omit one
			   character at each end, the usable space is 77 characters.
			   We loose 2 characters to the bars end markers, and 8 to
			   the current power consumtion display. So we can work with
			   67 characters. At the left we're starting at character 10
			   and at the right we end at character 76. */


			// Header
			snprintf(header, sizeof(header), "%s", cmdopts.cpumodel);
			mvprintw(0, 38 - (strlen(header) / 2), header);

			snprintf(header, sizeof(header), "(Arch: %s, Limit: %luW)",
					cmdopts.cpufamily, packagelimits.limit1);
			mvprintw(1, 38 - (strlen(header) / 2), header);

			// Total power consumption.
			num_load = floor((67.0 / packagelimits.limit1) * delta_energy.pkg);

			mvprintw(5, 1, "%6.2fW [", delta_energy.pkg);

			for (i = 0; i < num_load && i <= 66; i++) {
				mvprintw(5, i + 10, "=");
			}

			mvprintw(5, i + 10, ">");

			for (i++; i <= 66; i++) {
				mvprintw(5, i + 10, " ");
			}

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

			if (cmdopts.cputype == CLIENT) {
				// GPU power consumption.
				attron(A_BOLD);
				mvprintw(9, 60, "GPU:");
				attroff(A_BOLD);
				mvprintw(10, 60, "Current: %.2fJ", delta_energy.pp1);
				mvprintw(11, 60, "Total: %.2fJ", total_energy.pp1);
			} else if (cmdopts.cputype == SERVER) {
				// DRAM power consumption.
				attron(A_BOLD);
				mvprintw(9, 60, "DRAM:");
				attroff(A_BOLD);
				mvprintw(10, 60, "Current: %.2fJ", delta_energy.ram);
				mvprintw(11, 60, "Total: %.2fJ", total_energy.ram);
			}

			// Print the new data
			refresh();


			// Quit?
			if ((ch = getch()) != ERR) {
				switch (ch) {
					case 'q':
					case 'Q':
					case 27:
						stop = 1;
				}
			}

			if (stop) {
				break;
			}

			// Cleanup
			delta_energy.pkg = 0;
			delta_energy.pp0 = 0;
			delta_energy.pp1 = 0;
			delta_energy.ram = 0;
			count = 0;
		}

		// One sample every 50 milliseconds.
		usleep(50 * 1000);
	}

	// Quit curses
	endwin();
}


// --------

