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
/* latency.c */

#define	KERNEL_PRIVATE

#include "latency.h"

#define DEBUG_SAMPLER		0

int quit (char *s)
{
    void set_enable();
    void set_rtcdec();
    void set_remove();
    
    if (trace_enabled) set_enable(0);
    // This flag is turned off when calling quit() due to a set_remove() failure.
    if (set_remove_flag) set_remove();
    if (decrementer_val) set_rtcdec(0);
    
    printf("latency: ");
        if (s) printf("%s", s);
    exit(1);
}

void set_enable(int val) 
{
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDENABLE;		/* protocol */
    mib[3] = val;
    mib[4] = 0;
    mib[5] = 0;		        /* no flags */
    
    if (sysctl(mib, 4, NULL, &needed, NULL, 0) < 0)
        quit("trace facility failure, KERN_KDENABLE\n");
}

void set_numbufs (int nbufs) 
{
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDSETBUF;
    mib[3] = nbufs;
    mib[4] = 0;
    mib[5] = 0;		        /* no flags */
    if (sysctl(mib, 4, NULL, &needed, NULL, 0) < 0)
        quit("trace facility failure, KERN_KDSETBUF\n");
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDSETUP;		
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;		        /* no flags */
    if (sysctl(mib, 3, NULL, &needed, NULL, 0) < 0)
        quit("trace facility failure, KERN_KDSETUP\n");
}

void set_pidexclude (int pid, int on_off) 
{
    kd_regtype kr;
    
    kr.type = KDBG_TYPENONE;
    kr.value1 = pid;
    kr.value2 = on_off;
    needed = sizeof(kd_regtype);
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDPIDEX;
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;
    
    sysctl(mib, 3, &kr, &needed, NULL, 0);
}

void set_rtcdec (int decval)
{
    kd_regtype kr;
    int ret;
    extern int errno;
    
    kr.type = KDBG_TYPENONE;
    kr.value1 = decval;
    needed = sizeof(kd_regtype);
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDSETRTCDEC;		/* protocol */
    mib[3] = 0;				/* wildcard address family */
    mib[4] = 0;
    mib[5] = 0;				/* no flags */

    errno = 0;

    if ((ret=sysctl(mib, 3, &kr, &needed, NULL, 0)) < 0) {
        decrementer_val = 0;
        quit("trace facility failure, KERN_KDSETRTCDEC\n");
    }
}


void get_bufinfo(kbufinfo_t *val)
{
    needed = sizeof (*val);
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDGETBUF;		
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;		/* no flags */
    
    if (sysctl(mib, 3, val, &needed, 0, 0) < 0)
        quit("trace facility failure, KERN_KDGETBUF\n");
}

void set_remove() 
{
    extern int errno;
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDREMOVE;		/* protocol */
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;				/* no flags */
    
    errno = 0;
    
    if (sysctl(mib, 3, NULL, &needed, NULL, 0) < 0) {
        set_remove_flag = 0;
        if(errno == EBUSY) {
            quit("the trace facility is currently in use...\n         fs_usage, sc_usage, and latency use this feature.\n\n");
        } else {
            quit("trace facility failure, KERN_KDREMOVE\n");
        }
    }
}

void sigwinch()
{
    gotSIGWINCH = 1;
}

void sigintr()
{
    set_enable(0);
    set_pidexclude(getpid(), 0);
    set_rtcdec(0);
    set_remove();
    
    exit(1);
}

void sigquit()
{
    set_enable(0);
    set_pidexclude(getpid(), 0);
    set_rtcdec(0);
    set_remove();
    
    exit(1);
}

void sigterm()
{
    set_enable(0);
    set_pidexclude(getpid(), 0);
    set_rtcdec(0);
    set_remove();
    
    exit(1);
}

void initialize()
{
    int      		loop_cnt, sample_sc_now;
    int      		decrementer_usec = 0;
    kd_regtype		kr;
    void     getdivisor();
    void     init_code_file();
    void     do_kernel_nm();
    void     open_logfile();
    
    policy_name = "TIMESHARE";
    kernelpath = "/mach_kernel";
    code_file = "/usr/share/misc/trace.codes";
    
    do_kernel_nm();
    
    sample_sc_now = 25;
    
    getdivisor();
    decrementer_val = decrementer_usec * divisor;
    
    init_code_file();
    /* 
        When the decrementer isn't set in the options,
        decval will be zero and this call will reset
        the system default ...
    */
    set_rtcdec(decrementer_val);
    
    signal(SIGWINCH, sigwinch);
    signal(SIGINT, sigintr);
    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigterm);
    
    if ((my_buffer = malloc(SAMPLE_SIZE * sizeof(kd_buf))) == (char *)0)
        quit("can't allocate memory for tracing info\n");
    set_remove();
    set_numbufs(SAMPLE_SIZE);
    set_enable(0);
    
    // setup Kernel for trace logging
    kr.type = KDBG_RANGETYPE;
    kr.value1 = 0;	
    kr.value2 = -1;
    needed = sizeof(kd_regtype);
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDSETREG;		
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;		/* no flags */
    if (sysctl(mib, 3, &kr, &needed, NULL, 0) < 0)
        quit("trace facility failure, KERN_KDSETREG\n");
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDSETUP;		
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;		/* no flags */
    
    if (sysctl(mib, 3, NULL, &needed, NULL, 0) < 0)
        quit("trace facility failure, KERN_KDSETUP\n");

    // set_pidexclude(getpid(), 1);	// COMMENTED OUT 7 Feb, @ 10:20AM... NOT SURE IF WE WANT TO INCLUDE THIS PROCESS IN THE LOG OR NOT...
    set_enable(1);
    trace_enabled = 1;
    need_new_map = 1;
    
    loop_cnt = 0;
    start_time = time((long *)0);
    refresh_time = start_time;
}

double getdivisor()
{
    mach_timebase_info_data_t info;
    mach_timebase_info (&info);

    divisor = ( (double)info.denom / (double)info.numer) * 1000;
	return divisor;
}
												  
void read_command_map()
{
    size_t size;
    int mib[6];
  
    if (mapptr) {
	free(mapptr);
	mapptr = 0;
    }
    total_threads = bufinfo.nkdthreads;
    size = bufinfo.nkdthreads * sizeof(kd_threadmap);
    if (size) {
        if ((mapptr = (kd_threadmap *) malloc(size))) {
	     bzero (mapptr, size);
	} else {
	    printf("Thread map is not initialized -- this is not fatal\n");
	    return;
	}
    }
    
    // Now read the threadmap
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDTHRMAP;
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;		// no flags
    if (sysctl(mib, 3, mapptr, &size, NULL, 0) < 0) {
        // This is not fatal -- just means we can't map command strings
        printf("Can't read the thread map -- this is not fatal\n");
	free(mapptr);
	mapptr = 0;
	return;
    }
    return;
}

void create_map_entry(int thread, char *command)
{
    int			i, n;
    kd_threadmap	*map;
    
    if (!mapptr) return;
    
    for (i = 0, map = 0; !map && i < total_threads; i++) {
        if (mapptr[i].thread == thread )	
	    map = &mapptr[i];   // Reuse this entry, the thread has been reassigned
    }
    
    if (!map) {
        // look for invalid entries that I can reuse
        for (i = 0, map = 0; !map && i < total_threads; i++) {
	    if (mapptr[i].valid == 0 )	
	        map = &mapptr[i];   // Reuse this invalid entry
	}
    }
    
    if (!map) {
        /* If reach here, then this is a new thread and 
	 * there are no invalid entries to reuse
	 * Double the size of the thread map table.
	 */
        
        n = total_threads * 2;
	mapptr = (kd_threadmap *) realloc(mapptr, n * sizeof(kd_threadmap));
	bzero(&mapptr[total_threads], total_threads*sizeof(kd_threadmap));
	map = &mapptr[total_threads];
	total_threads = n;
    }

    map->valid = 1;
    map->thread = thread;
    (void)strncpy (map->command, command, sizeof(map->command));
    map->command[sizeof(map->command)-1] = '\0';
}

kd_threadmap *find_thread_map(int thread)
{
    int i;
    kd_threadmap *map;
    
    if (!mapptr)
        return((kd_threadmap *)0);
    
    for (i = 0; i < total_threads; i++) {
        map = &mapptr[i];
	if (map->valid && (map->thread == thread))
	    return(map);
    }
    return ((kd_threadmap *)0);
}

void kill_thread_map(int thread)
{
    kd_threadmap *map;
    
    if ((map = find_thread_map(thread))) {
        map->valid = 0;
	map->thread = 0;
	map->command[0] = '\0';
    }
}

struct th_info *find_thread(int thread, int type1, int type2) {
    struct th_info *ti;
    
    for (ti = th_state; ti < &th_state[cur_max]; ti++) {
        if (ti->thread == thread) {
            if (type1 == 0)
                return(ti);
            if (type1 == ti->type)
                return(ti);
            if (type2 == ti->type)
                return(ti);
        }
    }
    return ((struct th_info *)0);
}

char *find_code(type)
{
    int i;
    
    for (i = 0; i < num_of_codes; i++) {
        if (codes_tab[i].type == type) 
            return(codes_tab[i].name);
    }
    
    return ((char *)0);
}

void sample_sc(uint64_t start, uint64_t stop, char *binPointer)
{
    kd_buf   *kd, *last_mach_sched, *last_decrementer_kd, *end_of_sample;
    uint64_t now;
    int count;
    int first_entry = 1;
    char   command[32];
    double timestamp, last_timestamp = 0, delta = 0, start_bias = 0;
    void read_command_map();
    
    _isTraceingCPU = TRUE;
    curr_time = time((long *)0);
    
    // Get kernel buffer information
    get_bufinfo(&bufinfo);
    
    if (need_new_map) {
        read_command_map();
        need_new_map = 0;
    }
    needed = bufinfo.nkdbufs * sizeof(kd_buf);
    mib[0] = CTL_KERN;
    mib[1] = KERN_KDEBUG;
    mib[2] = KERN_KDREADTR;		
    mib[3] = 0;
    mib[4] = 0;
    mib[5] = 0;		// no flags
    
    if (sysctl(mib, 3, my_buffer, &needed, NULL, 0) < 0)
        quit("trace facility failure, KERN_KDREADTR\n");
    
    count = needed;
    
    // execute this block if our trace buffer wrapped
    if (bufinfo.flags & KDBG_WRAPPED) {
        int i;
        
        for (i = 0; i < cur_max; i++) {
            th_state[i].thread = 0;
            th_state[i].type = -1;
            th_state[i].pathptr = (long *)0;
            th_state[i].pathname[0] = 0;
        }
        cur_max = 0;
        need_new_map = 1;
        
        set_enable(0);
        set_enable(1);
        if (binPointer) {
            double latency = (double)(stop - start) / divisor;
            binPointer += sprintf(binPointer, "%-19.19s   scheduling latency = %.1fus  num_of_traces = %d <<<<<<< trace buffer wrapped >>>>>>>\n\n",
                            &(ctime(&curr_time)[0]), latency, count);
        }
    }
    
    end_of_sample = &((kd_buf *)my_buffer)[count];
    last_decrementer_kd = (kd_buf *)my_buffer;
    last_mach_sched = (kd_buf *)0;
    
    // parse the obtained trace
    for (kd = (kd_buf *)my_buffer; kd < end_of_sample; kd++) {
        int debugid, thread, cpunum;
        int type, clen, mode;
        int len;
        char *p;
        long *sargptr;
        double i_latency = 0;
        struct th_info *ti;
        char   command1[32];
        char   sched_info[64];
        kd_threadmap *map;
        kd_threadmap *find_thread_map();
        double handle_decrementer();
        kd_buf *log_decrementer();
        int check_for_thread_update();
        char * enter_syscall();
        char * exit_syscall();
        char * print_entry();
        
        thread  = kd->arg5 & KDBG_THREAD_MASK;
        cpunum =  (kd->arg5 & KDBG_CPU_MASK) ? 1: 0;
        debugid = kd->debugid;
        type    = kd->debugid & DBG_FUNC_MASK;
        
        if (check_for_thread_update(thread, type, kd))
            continue;
        
        if (type == DECR_TRAP)
            i_latency = handle_decrementer(kd);
        
        now = (((uint64_t)kd->timestamp.tv_sec) << 32) |
            (uint64_t)((unsigned int)(kd->timestamp.tv_nsec));
        
        timestamp = ((double)now) / divisor;
        
        if (now < start || now > stop) {
            if (debugid & DBG_FUNC_START)
                binPointer = enter_syscall(binPointer, kd, thread, type, command, timestamp, delta, start_bias, 0);
            else if (debugid & DBG_FUNC_END)
                binPointer = exit_syscall(binPointer, kd, thread, type, command, timestamp, delta, start_bias, 0);
            continue;
        }
        
        if (first_entry) {
            double latency;
            char buf1[132];
            char buf2[132];
            
            latency = (double)(stop - start) / divisor;
			
            sprintf(buf1, "%-19.19s             scheduling latency = %.1fus", &(ctime(&curr_time)[0]), latency);
            clen = 131;
            memset(buf2, '-', clen);
            buf2[clen] = 0;
            
            if (binPointer) {
                binPointer += sprintf(binPointer, "%s\n", buf2);
                binPointer += sprintf(binPointer, "%s\n", buf1);
                binPointer += sprintf(binPointer, "%s\n", buf2);
                binPointer += sprintf(binPointer, "RelTime(Us)  Delta              debugid                      arg1       arg2       arg3      arg4       thread   CPU command\n");
                binPointer += sprintf(binPointer, "%s\n", buf2);
            }
            start_bias = ((double)start) / divisor;
            last_timestamp = timestamp;
            first_entry = 0;
        }
        
        delta = timestamp - last_timestamp;

        if ((map = find_thread_map(thread)))
            strcpy(command, map->command);
        else
            command[0] = 0;
        
        switch (type) {
            case CQ_action:
                if (binPointer) {
                    binPointer += sprintf(binPointer, "%9.1f %8.1f\t\t\t\tCQ_action @ %-59.59s %-8x  %d  %s\n",
                                            timestamp - start_bias, delta, pc_to_string(kd->arg1, 59, 1) , thread, cpunum, command);
                }
                last_timestamp = timestamp;
                break;
            case TES_action:
                if (binPointer) {
                    binPointer+= sprintf(binPointer, "%9.1f %8.1f\t\t\t\tTES_action @ %-58.58s %-8x  %d  %s\n",
                                            timestamp - start_bias, delta, pc_to_string(kd->arg1, 58, 1) , thread, cpunum, command);
                }
                last_timestamp = timestamp;
                break;
            case IES_action:
                if (binPointer) {
                    binPointer += sprintf(binPointer, "%9.1f %8.1f\t\t\t\tIES_action @ %-58.58s %-8x  %d  %s\n",
                                            timestamp - start_bias, delta, pc_to_string(kd->arg1, 58, 1) , thread, cpunum, command);
                }
                last_timestamp = timestamp;
                break;
            case IES_filter:
                if (binPointer) {
                    binPointer += sprintf(binPointer, "%9.1f %8.1f\t\t\t\tIES_filter @ %-58.58s %-8x  %d  %s\n",
                                            timestamp - start_bias, delta, pc_to_string(kd->arg1, 58, 1) , thread, cpunum, command);
                }
                last_timestamp = timestamp;
                break;
            case DECR_TRAP:
                last_decrementer_kd = kd;
                p = " ";
                mode = 1;
                if ((ti = find_thread((kd->arg5 & KDBG_THREAD_MASK), 0, 0))) {
                    if (ti->type == -1 && strcmp(command, "kernel_task"))
                        mode = 0;
                }
            
                if (binPointer) {
                    binPointer += sprintf(binPointer, "%9.1f %8.1f[%.1f]%s\t\tDECR_TRAP @ %-59.59s %-8x  %d  %s\n",
                                            timestamp - start_bias, delta, i_latency, p, pc_to_string(kd->arg2, 59, mode) , thread, cpunum, command);
                }
                last_timestamp = timestamp;
                break;
            case DECR_SET:
                if (binPointer) {
                    if ((double)kd->arg1/divisor >= 1000.0) {
						binPointer += sprintf (binPointer, "%9.1f %8.1f[%.1f]  \t%-28.28s                                            %-8x  %d  %s\n",
												timestamp - start_bias, delta, (double)kd->arg1/divisor, "DECR_SET", thread, cpunum, command);
					} else {
						binPointer += sprintf (binPointer, "%9.1f %8.1f[%.1f]  \t%-28.28s                                             %-8x  %d  %s\n",
												timestamp - start_bias, delta, (double)kd->arg1/divisor, "\tDECR_SET", thread, cpunum, command);
					}
                }
                last_timestamp = timestamp;
                break;
                
                case MACH_sched:
                case MACH_stkhandoff:
		    last_mach_sched = kd;
		    
                    if ((map = find_thread_map(kd->arg2)))
                        strcpy(command1, map->command);
		    else
                        sprintf(command1, "%-8x", kd->arg2);

		    if ((ti = find_thread(kd->arg2, 0, 0))) {
                        if (ti->type == -1 && strcmp(command1, "kernel_task"))
                            p = "U";
                        else
                            p = "K";
		    } else {
                        p = "*";
                    }
		    memset(sched_info, ' ', sizeof(sched_info));
                    
		    sprintf(sched_info, "%14.14s", command);
		    clen = strlen(sched_info);
		    sched_info[clen] = ' ';
                    
		    sprintf(&sched_info[14],  " @ pri %3d  -->  %14.14s", kd->arg3, command1);
		    clen = strlen(sched_info);
		    sched_info[clen] = ' ';
                    
		    sprintf(&sched_info[45], " @ pri %3d%s", kd->arg4, p);
                    
		    if (binPointer) {
                        binPointer += sprintf(binPointer, "%9.1f %8.1f\t\t\t\t%-10.10s  %s    %-8x  %d  **SCHEDULED**\n",
                                                    timestamp - start_bias, delta, "MACH_SCHED", sched_info, thread, cpunum);
		    }
		    last_timestamp = timestamp;
		    break;
		case VFS_LOOKUP:
		    if ((ti = find_thread(thread, 0, 0)) == (struct th_info *)0) {
                        if (cur_max >= MAX_THREADS)
                            continue;
                        ti = &th_state[cur_max++];
                        
                        ti->thread = thread;
                        ti->type   = -1;
                        ti->pathptr = (long *)0;
                        ti->child_thread = 0;
		    }
		    if (!ti->pathptr) {
                        ti->arg1 = kd->arg1;
                        memset(&ti->pathname[0], 0, (PATHLENGTH + 1));
                        sargptr = (long *)&ti->pathname[0];
				
                        *sargptr++ = kd->arg2;
                        *sargptr++ = kd->arg3;
                        *sargptr++ = kd->arg4;
                        ti->pathptr = sargptr;
		    } else {
                        sargptr = ti->pathptr;
                        /*  We don't want to overrun our pathname buffer if the
                            kernel sends us more VFS_LOOKUP entries than we can
                            handle.
                        */
                        if ((long *)sargptr < (long *)&ti->pathname[PATHLENGTH]) {
                            *sargptr++ = kd->arg1;
                            *sargptr++ = kd->arg2;
                            *sargptr++ = kd->arg3;
                            *sargptr++ = kd->arg4;
                            ti->pathptr = sargptr;
                            // print the tail end of the pathname
                            len = strlen(ti->pathname);
                            if (len > 28)
                                len -= 28;
                            else
                                len = 0;
			    
                            if (binPointer) {
                                binPointer += sprintf(binPointer, "%9.1f %8.1f\t\t\t\t%-28.28s %-28s    %-8x   %-8x  %d  %s\n",
                                                            timestamp - start_bias, delta, "VFS_LOOKUP", 
                                                            &ti->pathname[len], ti->arg1, thread, cpunum, command);
                            }
                        }
                    }
		    last_timestamp = timestamp;
		    break;
		default:
		    if (debugid & DBG_FUNC_START) {
                        binPointer = enter_syscall(binPointer, kd, thread, type, command, timestamp, delta, start_bias, 1);
                    } else if (debugid & DBG_FUNC_END) {
                        binPointer = exit_syscall(binPointer, kd, thread, type, command, timestamp, delta, start_bias, 1);
                    } else {
                        binPointer = print_entry(binPointer, kd, thread, type, command, timestamp, delta, start_bias);
                    }
                    
		    last_timestamp = timestamp;
		    break;
		}
	}
        if (last_mach_sched && binPointer)
	        binPointer += sprintf(binPointer, "\nblocked by %s @ priority %d\n", command, last_mach_sched->arg3);
        
        _isTraceingCPU = FALSE;
}

char *enter_syscall (char *fp, kd_buf *kd, int thread, int type, char *command, double timestamp, double delta, double bias, int print_info)
{
    struct th_info *ti;
    int    i;
    int    cpunum;
    char  *p;
    
    cpunum =  (kd->arg5 & KDBG_CPU_MASK) ? 1: 0;
    
    if (print_info && fp) {
        if ((p = find_code(type))) {
            if (type == INTERRUPT) {
                int mode = 1;
                if ((ti = find_thread((kd->arg5 & KDBG_THREAD_MASK), 0, 0))) {
                    if (ti->type == -1 && strcmp(command, "kernel_task")) mode = 0;
                }

                fp += sprintf(fp, "%9.1f %8.1f\t\t\t\tINTERRUPT @ %-59.59s %-8x  %d  %s\n",
                                    timestamp - bias, delta, pc_to_string(kd->arg2, 59, mode), thread, cpunum, command);
            } else if (type == MACH_vmfault) {
                fp += sprintf(fp, "%9.1f %8.1f\t\t\t\t%-28.28s                                            %-8x  %d  %s\n",
                                    timestamp - bias, delta, p, thread, cpunum, command);
            } else {
                fp += sprintf(fp, "%9.1f %8.1f\t\t\t\t%-28.28s %-8x   %-8x   %-8x  %-8x   %-8x  %d  %s\n",
                                    timestamp - bias, delta, p, kd->arg1, kd->arg2, kd->arg3, kd->arg4, 
                                    thread, cpunum, command);
            }
        } else {
            fp += sprintf(fp, "%9.1f %8.1f\t\t\t\t%-8x                     %-8x   %-8x   %-8x  %-8x   %-8x  %d  %s\n",
                                timestamp - bias, delta, type, kd->arg1, kd->arg2, kd->arg3, kd->arg4, 
                                thread, cpunum, command);
        }
    }
    
    if ((ti = find_thread(thread, -1, type)) == (struct th_info *)0) {
        if (cur_max >= MAX_THREADS) {
            static int do_this_once = 1;
            
            if (do_this_once) {
                for (i = 0; i < cur_max; i++) {
                    if (!fp) break;
                    fp += sprintf(fp, "thread = %x, type = %x\n", th_state[i].thread, th_state[i].type);
                }
                do_this_once = 0;
            }
            return fp;
        }
    
        ti = &th_state[cur_max++];
        ti->thread = thread;
        ti->child_thread = 0;
    }
    
    if (type != BSC_exit) {
        ti->type = type;
    } else {
        ti->type = -1;
    }
    ti->stime  = timestamp;
    ti->pathptr = (long *)0;
    
    return fp;
}

char *exit_syscall (char *fp, kd_buf *kd, int thread, int type, char *command, double timestamp, double delta, double bias, int print_info)
{
    struct th_info *ti;
    int    cpunum;
    char   *p;
    
    cpunum =  (kd->arg5 & KDBG_CPU_MASK) ? 1: 0;
    ti = find_thread(thread, type, type);
    
    if (print_info && fp) {
        if (ti) {
			if ((timestamp - ti->stime) >= 10000.0) {
				fp += sprintf(fp, "%9.1f %8.1f(%.1f) \t", timestamp - bias, delta, timestamp - ti->stime);
			} else {
				fp += sprintf(fp, "%9.1f %8.1f(%.1f) \t\t", timestamp - bias, delta, timestamp - ti->stime);
			}
			
        } else {
            fp += sprintf(fp, "%9.1f %8.1f()      \t\t\t", timestamp - bias, delta);
        }
		
        if ((p = find_code(type))) {
            if (type == INTERRUPT) {
                fp += sprintf(fp, "INTERRUPT                                                               %-8x  %d  %s\n", thread, cpunum, command);
            } else if (type == MACH_vmfault && kd->arg2 <= DBG_CACHE_HIT_FAULT) {
                fp += sprintf(fp, "%-28.28s %-8.8s   %-8x                        %-8x  %d  %s\n",
                                p, fault_name[kd->arg2], kd->arg1, thread, cpunum, command);
            } else {
                fp += sprintf(fp, "%-28.28s %-8x   %-8x                        %-8x  %d  %s\n",
                                p, kd->arg1, kd->arg2, thread, cpunum, command);
            }
        } else {
            fp += sprintf(fp, "%-8x                     %-8x   %-8x                        %-8x  %d  %s\n",
                            type, kd->arg1, kd->arg2, thread, cpunum, command);
        }
    }
    
    if (ti == (struct th_info *)0) {
        if ((ti = find_thread(thread, -1, -1)) == (struct th_info *)0) {
            if (cur_max >= MAX_THREADS) return fp;
            
            ti = &th_state[cur_max++];
            
            ti->thread = thread;
            ti->child_thread = 0;
            ti->pathptr = (long *)0;
        }
    }
    
    ti->type = -1;
    return fp;
}

char *print_entry (char *fp, kd_buf *kd, int thread, int type, char *command, double timestamp, double delta, double bias)
{
    char  *p;
    int cpunum;
    
    if (!fp) return fp;
    
    cpunum = (kd->arg5 & KDBG_CPU_MASK) ? 1: 0;
    
    if ((p = find_code(type))) {
        fp += sprintf(fp, "%9.1f %8.1f\t\t\t\t%-28.28s %-8x   %-8x   %-8x  %-8x   %-8x  %d  %s\n",
		       timestamp - bias, delta, p, kd->arg1, kd->arg2, kd->arg3, kd->arg4, 
		       thread, cpunum, command);
    } else {
        fp += sprintf(fp, "%9.1f %8.1f\t\t\t\t%-8x                     %-8x   %-8x   %-8x  %-8x   %-8x  %d  %s\n",
		       timestamp - bias, delta, type, kd->arg1, kd->arg2, kd->arg3, kd->arg4, 
		       thread, cpunum, command);
    }
    return fp;
}

int check_for_thread_update (int thread, int type, kd_buf *kd)
{
    struct th_info *ti;
    void create_map_entry();
    
    switch (type) {
    
    case TRACE_DATA_NEWTHREAD:
        if ((ti = find_thread(thread, 0, 0)) == (struct th_info *)0) {
            if (cur_max >= MAX_THREADS) return 1;
            ti = &th_state[cur_max++];
            
            ti->thread = thread;
            ti->type   = -1;
            ti->pathptr = (long *)0;
        }
        ti->child_thread = kd->arg1;
        return 1;
    case TRACE_STRING_NEWTHREAD:
        if ((ti = find_thread(thread, 0, 0)) == (struct th_info *)0)
            return 1;
        if (ti->child_thread == 0)
            return 1;
        create_map_entry(ti->child_thread, (char *)&kd->arg1);
        
        ti->child_thread = 0;
        return 1;
    case TRACE_STRING_EXEC:
        create_map_entry(thread, (char *)&kd->arg1);
        return 1;
    }
    
    return 0;
}

double handle_decrementer(kd_buf *kd)
{
    double		latency;
    int			elapsed_usecs;
    
    if ((int)(kd->arg1) >= 0)
        latency = 1;
    else
        latency = (((double)(-1 - kd->arg1)) / divisor);
    
    elapsed_usecs = (int)latency;
    
    if (elapsed_usecs > i_max_latency)
        i_max_latency = elapsed_usecs;
    if (elapsed_usecs < i_min_latency || i_total_samples == 0)
        i_min_latency = elapsed_usecs;
    i_total_latency += elapsed_usecs;
    i_total_samples++;
    
    return (latency);
}

void init_code_file()
{
    FILE		*fp;
    int			i, n, cnt, code;
    char		name[128];
    
    if ((fp = fopen(code_file, "r")) == (FILE *)0) return;
    
    n = fscanf(fp, "%d\n", &cnt);
    
    if (n != 1) return;
    
    for (i = 0; i < MAX_ENTRIES; i++) {
        n = fscanf(fp, "%x%s\n", &code, name);
        
        if (n != 2) break;
        
        strncpy(codes_tab[i].name, name, 32);
        codes_tab[i].type = code;
    }
    num_of_codes = i;
    
    fclose(fp);
}

void do_kernel_nm()
{
    int i, len;
    FILE *fp = (FILE *)0;
    char tmp_nm_file[128];
    char tmpstr[1024];
    int inchr;
    
    bzero(tmp_nm_file, 128);
    bzero(tmpstr, 1024);
    
    // Build the temporary nm file path
    sprintf(tmp_nm_file, "/tmp/knm.out.%d", getpid());
    
    // Build the nm command and create a tmp file with the output
    sprintf (tmpstr, "/usr/bin/nm -f -n -s __TEXT __text %s > %s", kernelpath, tmp_nm_file);
    system (tmpstr);
    
    // Parse the output from the nm command 
    if ((fp=fopen(tmp_nm_file, "r")) == (FILE *)0)
    {
        // Hmmm, let's not treat this as fatal
        fprintf(stderr, "Failed to open nm symbol file [%s]\n", tmp_nm_file);
        return;
    }
    
    // Count the number of symbols in the nm symbol table
    kern_sym_count=0;
    while ( (inchr = getc(fp)) != -1) {
        if (inchr == '\n')
        kern_sym_count++;
    }
    
    rewind(fp);
    
    // Malloc the space for symbol table
    if (kern_sym_count > 0) {
        kern_sym_tbl = (kern_sym_t *)malloc(kern_sym_count * sizeof (kern_sym_t));
       if (!kern_sym_tbl) {
	   // Hmmm, lets not treat this as fatal
	   fprintf(stderr, "Can't allocate memory for kernel symbol table\n");
        } else {
            bzero(kern_sym_tbl, (kern_sym_count * sizeof(kern_sym_t)));
        }
    } else {
        // Hmmm, lets not treat this as fatal
        fprintf(stderr, "No kernel symbol table \n");
    }
    
    for (i = 0; i < kern_sym_count; i++) {
        bzero(tmpstr, 1024);
        if (fscanf(fp, "%x %c %s", (unsigned int*)&kern_sym_tbl[i].k_sym_addr, (char*)&inchr, tmpstr) != 3) {
            break;
        } else {
            len = strlen(tmpstr);
            kern_sym_tbl[i].k_sym_name = malloc(len + 1);
            
            if (kern_sym_tbl[i].k_sym_name == (char *)0) {
                fprintf(stderr, "Can't allocate memory for symbol name [%s]\n", tmpstr);
                kern_sym_tbl[i].k_sym_name = (char *)0;
                len = 0;
            } else {
                strcpy(kern_sym_tbl[i].k_sym_name, tmpstr);
            }
            
            kern_sym_tbl[i].k_sym_len = len;
        }
    }
    
    if (i != kern_sym_count) {
        /* Hmmm, didn't build up entire table from nm */
        /* scrap the entire thing */
        if (kern_sym_tbl) free (kern_sym_tbl);
        kern_sym_tbl = (kern_sym_t *)0;
        kern_sym_count = 0;
    }
    fclose(fp);
    
    // Remove the temporary nm file
    unlink(tmp_nm_file);
    
#if DEBUG_SAMPLER
    // Print out the WHOLE KERNEL SYMBOL TABLE
    for (i = 0; i < kern_sym_count; i++) {
        if (kern_sym_tbl[i].k_sym_name)
            printf ("[%d] 0x%x    %s\n", i, kern_sym_tbl[i].k_sym_addr, kern_sym_tbl[i].k_sym_name);
        else
            printf ("[%d] 0x%x    %s\n", i, kern_sym_tbl[i].k_sym_addr, "No symbol name");
    }
#endif
}

char *pc_to_string (unsigned int pc, int max_len, int mode)
{
    int ret, len;
    int binary_search();
    
    if (mode == 0) {
        sprintf(pcstring, "0x%-8x [usermode addr]", pc);
        return pcstring;
    }

    ret = 0;
    ret = binary_search(kern_sym_tbl, 0, kern_sym_count-1, pc);

    if (ret == -1) {
        sprintf(pcstring, "0x%x", pc);
        return pcstring;
    } else if (kern_sym_tbl[ret].k_sym_name == (char *)0) {
        sprintf(pcstring, "0x%x", pc);
        return pcstring;
    } else {
        if ((len = kern_sym_tbl[ret].k_sym_len) > (max_len - 8))
            len = max_len - 8;
    
        memcpy(pcstring, kern_sym_tbl[ret].k_sym_name, len);
        sprintf(&pcstring[len], "+0x%-5x", (unsigned int)(pc - kern_sym_tbl[ret].k_sym_addr));
        return pcstring;
    }
}

// find address in kernel symbol list
int binary_search (kern_sym_t *list, int low, int high, unsigned int addr)
{
    int mid;
    mid = (low + high) / 2;
    
    if (low > high) {
        return -1;   // failed
    }
    
    if (low + 1 == high) {
        if ( (list[low].k_sym_addr <= addr) && (addr < list[high].k_sym_addr) ) {
            return low;	// case for range match in bottom
	} else if (list[high].k_sym_addr <= addr) {
	  return high;	// case for range match in top
	} else {
            return -1;	// failed
        }
    } else if (addr < list[mid].k_sym_addr) {
        return (binary_search (list, low, mid, addr));	// address in lower-half of search space
    } else {
        return (binary_search (list, mid, high, addr));	// address in upper-half of search space
    }
}

/* functions added for MillionMonkeys' use */
Boolean isTraceingCPU () {
	return _isTraceingCPU;
}

/* 
 */
