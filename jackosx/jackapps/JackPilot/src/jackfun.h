/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#include "bequite.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <Jack/jack.h>
#include <Carbon/Carbon.h>


int getDefInput(void);
int getDefOutput(void);
int getSysOut(void);

int openJack(const char *stringa);
int closeJack(void);
int my_system(const char*command);
int my_system2(const char*command);
int checkJack(void);
int openJackClient(void);
int ottieniPorte(void);
int portaPerNumero(int n,char *nomeOut,unsigned long *tipo);
int connessionePerNumero(int n,char *nomeOut);
int connessionePerNumero2(int n,char **nomeOut);
int getStatus(void);
int numeroPorte(void);
int connectPorts( char*da, char*a);
int disconnectPorts( char*da, char*a);
float cpuLoad(void);
unsigned long getTipoByName(const char* name);
void writeStatus(int n);
int loadPrefStat(void);
int savePrefStat(int DRIVER,int INTERFACE,int SR,int BUF,int CH);
int getSR(void);
int getBUF(void);
int getDRIVER(void);
int getCHAN(void);
int jackALStore(int inCH,int outCH,int AUTOC,int DEFinput,int DEFoutput,int DEFsystem);
int jackALLoad(void);
int getInCH(void);
int getOutCH(void);
int getAutoC(void);
int getConnections(void);
int getInterface(void) ;
int ottieniNomeClienti(char **nomi,int *quanti);
int nomeCliente(int n,char *nome);
int quantiClienti(void);
void writeHomePath(char *path);
int getFlagOfJack(void);
jack_client_t* getClient(void);
void getCurrentAudioDevice(char *outName);
void setCurrentAudioDevice(char *inName);
void JPLog(char *fmt,...);
///other Kill utilities
