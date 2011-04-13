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
#include <jack/jack.h>
#include <Carbon/Carbon.h>

#ifdef __cplusplus
extern "C"
{
#endif


int getDefInput(void);
int getDefOutput(void);
int getSysOut(void);
int getVerboseLevel(void);
int getHogMode(void);
int getClockMode(void);
int getMonitorMode(void);
int getMIDIMode(void);

int openJack(const char *stringa);
int closeJack(void);
void closeJack1(void);
void closeJack2(void);
int my_system(const char*command);
int my_system2(const char*command);
int checkJack(void);
bool openJackClient(void);
const char** getAllPorts(void);
void portaPerNumero(int n, char *nomeOut, unsigned long *tipo);
int connessionePerNumero2(int n, char **nomeOut, int len);
int getStatus(void);
int numeroPorte();
int connectPorts(char* da, char* a);
int disconnectPorts(char* da, char* a);
float cpuLoad(void);
unsigned long getTipoByName(const char* name);
void writeStatus(int n);
int getSR(void);
int getBUF(void);
int getDRIVER(void);
int getCHAN(void);
int jackALStore(int inCH, int outCH, int AUTOC, int DEFinput, int DEFoutput, int DEFsystem, int LOGSLevel, char* driverIn, char* driverOut, int HOG, int CLOCK, int MONITOR, int MIDI);
int jackALLoad(void);
int getInCH(void);
int getOutCH(void);
int getAutoC(void);
int getConnections(void);
int getInterface(void) ;
int ottieniNomeClienti(char **nomi,int *quanti);
int nameOfClient(int n,char *nome);
int quantiClienti(void);
void writeHomePath(char *path);
int getFlagOfJack(void);
jack_client_t* getClient(void);
void getCurrentAudioDevice(char *outName);
void setCurrentAudioDevice(char *inName);
void JPLog(const char *fmt,...);

// Globals
extern int gPortNum;

#ifdef __cplusplus
}
#endif
