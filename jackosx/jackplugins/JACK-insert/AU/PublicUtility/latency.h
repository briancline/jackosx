/*	Copyright: 	© Copyright 2002 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* latency.h */

/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/* 
   cc -I. -DKERNEL_PRIVATE -O -o latency latency.c
*/

#include <mach/mach.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <nlist.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>

#include <libc.h>
#include <termios.h>
#include <bsd/curses.h>
#include <sys/ioctl.h>

#ifndef KERNEL_PRIVATE
#define KERNEL_PRIVATE
#include "kdebug.h"
#undef KERNEL_PRIVATE
#else
#include "kdebug.h"
#endif /*KERNEL_PRIVATE*/

#include <sys/sysctl.h>
#include <errno.h>
#include <err.h>

#include <mach/host_info.h>
#include <mach/mach_error.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mach_syscalls.h>
#include <mach/clock_types.h>
#include <mach/mach_time.h>

#include <libkern/OSTypes.h>

extern mach_port_t clock_port;

#define KERN_KDPIDEX    14


int      s_too_slow;
int      s_max_latency;
int      s_min_latency = 0;
long long s_total_latency = 0;
int      s_total_samples;
int      s_exceeded_threshold = 0;

int      i_too_slow;
int      i_max_latency;
int      i_min_latency = 0;
long long i_total_latency = 0;
int      i_total_samples;
int      i_exceeded_threshold = 0;

long     start_time;
long     curr_time;
long     refresh_time;

char     *policy_name;
int      my_pri = -1;

char *kernelpath = (char *)0;
char *code_file = (char *)0;

typedef struct {
  u_long  k_sym_addr;       /* kernel symbol address from nm */
  u_int   k_sym_len;        /* length of kernel symbol string */
  char   *k_sym_name;       /* kernel symbol string from nm */
} kern_sym_t;

kern_sym_t *kern_sym_tbl;      /* pointer to the nm table       */
int        kern_sym_count;    /* number of entries in nm table */
char       pcstring[128];

#define UNKNOWN "Can't find symbol name"


double   divisor;
int      gotSIGWINCH = 0;
int      trace_enabled = 0;

#define SAMPLE_SIZE 300000

int mib[6];
size_t needed;
char  *my_buffer;

kbufinfo_t bufinfo = {0, 0, 0};

FILE *log_fp = (FILE *)0;
int num_of_codes = 0;
int need_new_map = 0;
int total_threads = 0;
kd_threadmap *mapptr = 0;

#define MAX_ENTRIES 1024
struct ct {
        int type;
        char name[32];
} codes_tab[MAX_ENTRIES];

/* If NUMPARMS changes from the kernel, then PATHLENGTH will also reflect the change */
#define NUMPARMS 23
#define PATHLENGTH (NUMPARMS*sizeof(long))

struct th_info {
        int  thread;
        int  type;
        int  child_thread;
        int  arg1;
        double stime;
        long *pathptr;
        char pathname[PATHLENGTH + 1];
};

#define MAX_THREADS 512
struct th_info th_state[MAX_THREADS];

int  cur_max = 0;

#define TRACE_DATA_NEWTHREAD   0x07000004
#define TRACE_STRING_NEWTHREAD 0x07010004
#define TRACE_STRING_EXEC      0x07010008

#define INTERRUPT         0x01050000
#define DECR_TRAP         0x01090000
#define DECR_SET          0x01090004
#define MACH_vmfault      0x01300000
#define MACH_sched        0x01400000
#define MACH_stkhandoff   0x01400008
#define VFS_LOOKUP        0x03010090
#define BSC_exit          0x040C0004
#define IES_action        0x050b0018
#define IES_filter        0x050b001c
#define TES_action        0x050c0010
#define CQ_action         0x050d0018


#define DBG_FUNC_ALL	(DBG_FUNC_START | DBG_FUNC_END)
#define DBG_FUNC_MASK	0xfffffffc

#define DBG_ZERO_FILL_FAULT   1
#define DBG_PAGEIN_FAULT      2
#define DBG_COW_FAULT         3
#define DBG_CACHE_HIT_FAULT   4

char *fault_name[5] = {
        "",
	"ZeroFill",
	"PageIn",
	"COW",
	"CacheHit",
};

char *pc_to_string();

int decrementer_val = 0;     /* Value used to reset decrementer */
int set_remove_flag = 1;     /* By default, remove trace buffer */

Boolean						_isTraceingCPU = FALSE;

/* 
*/
