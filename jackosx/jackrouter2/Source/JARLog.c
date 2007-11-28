/*
Copyright © Grame 2007

This library is free software; you can redistribute it and modify it under
the terms of the GNU Library General Public License as published by the
Free Software Foundation version 2 of the License, or any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License
for more details.

You should have received a copy of the GNU Library General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
grame@rd.grame.fr

*/

#include "JARLog.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int JAR_fDebug = 0;

/*
void JARLog(char *fmt, ...)
{
    if (JAR_fDebug) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "JAR: ");
        vfprintf(stderr, fmt, ap);
		fflush(stderr);
        va_end(ap);
    }
}
*/

void JARLog(char *fmt, ...)
{
    if (JAR_fDebug) {
        va_list ap;
        va_start(ap, fmt);
        printf("JAR: ");
        vprintf(fmt, ap);
		fflush(stderr);
        va_end(ap);
    }
}

/*
void Print4CharCode(char* msg, long c)
{
    if (JAR_fDebug) {
        unsigned int  __4CC_number = (c);
        char __4CC_string[5];
        memcpy(__4CC_string, &__4CC_number, 4);
        __4CC_string[4] = 0;
        JARLog("%s'%s'\n", (msg), __4CC_string);
    }
}
*/
