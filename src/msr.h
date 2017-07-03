/*
 * Copyright (c) 2012, Intel Corporation
 * Copyright (c) 2017, Y. Burmeister
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MSR_H_
#define MSR_H_

/*
 * Intel CPUs tarting with Sandy Bridge write their internal energy
 * management counters into several MSR. What counters are available
 * and their meaning is dependent on the CPU model and platform.
 *
 *
 *  Client platform:
 *    * PKG: Power consumption of the whole package / socket.
 *    * PP0: x86 cores.
 *    * PP1: Integrated GPU.
 *
 *  It can be observed that PP0 + PP1 < PKG. The difference between
 *  PP0 and PP1 is the uncore (L3 cache, memory controller, ring bus,
 *  etc.) power consumption. This has been confirmed by Intel.
 *
 *
 *  Server platform:
 *    * PKG: Power consumption of the whole package / socket.
 *    * PP0: x86 cores.
 *    * DRAM: Power consumption of the DIMM sockets.
 *
 *  The DRAM values are highly dependent on the OEM platform. The
 *  measurements may be very accurate or just garbage. The uncores
 *  power consumption can be calculated by the difference between
 *  PKG and PP0.
 *
 *
 * Further informations can be found in this paper:
 * https://www2.eecs.berkeley.edu/Pubs/TechRpts/2012/EECS-2012-168.pdf 
 *
 * Intels documentation can be found here:
 * https://software.intel.com/en-us/articles/intel-power-governor
 */


#include <stdbool.h>
#include <stdint.h>


// --------


// The unit multiplier. It is used to calculate the factor to convert
// raw values into actual Joules. Additionally the MSR wraparound is 
// calculated from it.
#define UNIT_MULTIPLIER 0x606


// --------


// Package / socket power limit to be enforced.
#define PKG_LIMIT    0x610

// Power consumed since reboot or last register wrap around.
#define PKG_STATUS   0x611

// Time the CPU was throttled to enforce the power limit.
#define PKG_THROTTLE 0x613

// Informations about min and max power limit.
#define PKG_INFO     0x614


// --------


// x86 cores power limit to be enforced.
#define PP0_LIMIT  0x638

// Power consumption of x86 cores since reboot or register wrap around.
#define PP0_STATUS 0x639

// Gets or set the priority how the power management units distributes
// the power between PP0 and PP1. The priority is between 0 (low) and
// 31 (high).
#define PP0_POLICY 0x63a

// Time the x86 cores were throttled to enforce the power limit.
#define PP0_TIME   0x63b


// --------


// GPU power limit to be enforced.
#define PP1_LIMIT  0x640

// GPU power consumption since reboot or register wrap around.
#define PP1_STATUS 0x641

// Gets or set the priority how the power management units distributes
// the power between PP0 and PP1. The priority is between 0 (low) and
// 31 (high).
#define PP1_POLICY 0x642


// --------

// DRAM power limit to be enforced.
#define DRAM_LIMIT    0x618

// DRAM power consumption since reboot or register wrap around.
#define DRAM_STATUS   0x619

// Time DRAM bandwidth was throttled to enforce the power limit.
#define DRAM_THROTTLE 0x61b

// Informations about min and max power limit.
#define DRAM_INFO     0x61c


// --------


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

// PKG_LIMIT MSR structure.
typedef struct pkg_limit_msr_t {
	uint64_t power_limit_1         : 15;
	uint64_t limit_enabled_1       : 1;
	uint64_t clamp_enabled_1       : 1;
	uint64_t limit_time_window_y_1 : 5;
	uint64_t limit_time_window_f_1 : 2;
	uint64_t                       : 8;
	uint64_t power_limit_2         : 15;
	uint64_t limit_enabled_2       : 1;
	uint64_t clamp_enabled_2       : 1;
	uint64_t limit_time_window_y_2 : 5;
	uint64_t limit_time_window_f_2 : 2;
	uint64_t                       : 7;
	uint64_t lock_enabled          : 1;
} pkg_limit_msr_t;

// *_STATUS MSR structure.
typedef struct status_msr_t {
	uint64_t total_energy_consumed : 32;
	uint64_t                       : 32;
} status_msr_t;

// *_THROTTLE MSR structure.
typedef struct throttle_msr_t {
	uint64_t accumulated_throttled_time : 32;
	uint64_t                            : 32;
} throttle_msr_t;

// *_INFO MSR structure.
typedef struct info_msr_t {
	uint64_t thermal_spec_power        : 15;
	uint64_t                           : 1;
	uint64_t minimum_power             : 15;
	uint64_t                           : 1;     
	uint64_t maximum_power             : 15;
	uint64_t                           : 1;
	uint64_t maximum_limit_time_window : 6;
	uint64_t                           : 10;
} info_msr_t;

// *_POLICY MSR structure.
typedef struct policy_msr_t {
	uint64_t priority_level : 5;
	uint64_t                : 32;
	uint64_t                : 27;
} policy_msr_t;

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


// --------


// Replacement for pow() with a base of 2.
#define B2POW(e) (((e) == 0) ? 1 : (2 << ((e) - 1)))

/*
 * Checks if the given MSR exists.
 *
 * - msr: MSR to be checked.
 */
bool checkmsr(int32_t msr);

/*
 * Reads the given MSR and returns it's data.
 *
 *  - msr: MSR to read.
 */
uint64_t getmsr(int32_t msr);


// --------

#endif // MSR_H_

