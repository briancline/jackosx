/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */


#include <stdio.h>
#include <Carbon/Carbon.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/sysctl.h>

typedef struct kinfo_proc kinfo_proc;

kinfo_proc* GetBSDProcessList(size_t *procCount, kinfo_proc infop);

kinfo_proc* test(int *quanti);