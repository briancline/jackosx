/*
  Copyright ©  Johnny Petrantoni 2003
 
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

*/

// bequite.c

#include <stdio.h>
#include "procinfo.h"
#include <signal.h>
#include "bequite.h"

typedef struct processo {
    char nome[30];
    int pid;
} InfoProc;

int quantiProc() {
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    return quanti;
}

InfoProc ottieniInfo(int n) {
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    InfoProc result;
    strcpy(result.nome,pInfo[n].kp_proc.p_comm);
    result.pid = pInfo[n].kp_proc.p_pid;
    return result;
}

char* ottieniNome(int n) {
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    char *result;
    result = pInfo[n].kp_proc.p_comm;
    if(ottieniFlag(n)==1) strcat(result," #stopped#");
    return result;
}

int ottieniPid(int n) {
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    return pInfo[n].kp_proc.p_pid;
}

void stop(int n) {
    kill(ottieniPid(n),SIGSTOP);
}

void reStart(int n) {
    kill(ottieniPid(n),SIGCONT);
}

int ottieniFlag(int n) {
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    int stat;
    stat = pInfo[n].kp_proc.p_stat;
    int result;
    if (stat!=4) result = 0;
    if (stat==4) result = 1;
    return result;
}

char * ottieniNomeFromPid(int pid) {
    int quanti = quantiProc();
    int i;
    for (i = 0;i<quanti;i++) {
        if(pid==ottieniPid(i)) {
            if(strcmp("LaunchCFMApp",ottieniNome(i))==0) { //Look for the name using CARBON (for CFM applications)
                OSErr err;
                ProcessSerialNumber process;
                CFStringRef nomeStr;
                err = GetCurrentProcess(&process);
                if(err==noErr) {
                    err = CopyProcessName(&process,&nomeStr);
                }
                if(err!=noErr) return ottieniNome(i);
                else {
                    char *resu;
                    resu = CFStringGetCStringPtr(nomeStr,NULL);
                    return resu;
                }

            }
            return ottieniNome(i);
        }
    }
    return NULL;
}


