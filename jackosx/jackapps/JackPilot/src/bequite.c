/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#include <stdio.h>
#include "procinfo.h"
#include <signal.h>
#include "bequite.h"

typedef struct processo {
    char nome[30];
    int pid;
} InfoProc;

int quantiProc() 
{
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    return quanti;
}

InfoProc ottieniInfo(int n) 
{
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    InfoProc result;
    strcpy(result.nome,pInfo[n].kp_proc.p_comm);
    result.pid = pInfo[n].kp_proc.p_pid;
    return result;
}

char* ottieniNome(int n) 
{
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    char *result;
    result = pInfo[n].kp_proc.p_comm;
    if (ottieniFlag(n) == 1) 
        strcat(result," #stopped#");
    return result;
}

int ottieniPid(int n) 
{
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    return pInfo[n].kp_proc.p_pid;
}

void stop(int n) 
{
    kill(ottieniPid(n),SIGSTOP);
}

void reStart(int n) 
{
    kill(ottieniPid(n),SIGCONT);
}

int ottieniFlag(int n) 
{
    kinfo_proc *pInfo;
    int quanti;
    pInfo = test(&quanti);
    int stat;
    stat = pInfo[n].kp_proc.p_stat;
    return stat;
}


