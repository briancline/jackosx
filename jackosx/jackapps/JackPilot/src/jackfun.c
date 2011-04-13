/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#include "jackfun.h"

#define SHELL "/bin/sh"

int firsttime = 0;
int status;
jack_client_t* client = NULL;
int sr, buf, driver, ch;
int inch, outch, autoc;
int interface;
char* homePath = NULL;
int flag;
char coreAudioDevice[256];
int defInput,defOutput,defSystem;
int verboseLevel = 0;
int hogmode = 0;
int clockmode = 0;
int monitormode = 0;
int MIDImode = 0;

#define JACK_MAX_PORTS  2048

int gPortNum = 0;

/*
static void JackPortRegistration(jack_port_id_t port, int a, void *arg) 
{
	gPortNum = numeroPorte_aux();
}
*/

int openJack(const char *stringa) 
{
    JPLog("%s\n", stringa);
    int a;
    if (firsttime == 0) {
        int test;
        test = checkJack();
        if (test != 0) { 
			return 1; 
		}
    }
    a = my_system(stringa);
    if (a == 1) { 
       //nConnect = 0;  // steph 
        a = checkJack();
    }
    return a;
}

void closeJack1(void) 
{
	if (client != NULL) {
		jack_client_close(client);
		client = NULL;
	}
}

void closeJack2(void) 
{
    firsttime = 0;
}

int closeJack(void) 
{
	if (client != NULL) {
		jack_client_close(client);
		client = NULL;
	}
	
#if 1
    JPLog("closeJack : kill\n");
    int pid = checkJack();
	if (pid != 0) { 
		kill(pid, SIGQUIT); //SIGUSR2 is the one that should work (look at jackd.c or engine.c) but doesn't work...
  	} 
#else
	my_system2("killall jackd"); //this works but to re-start jackd you must close and re open JP (look console logs)
	my_system2("killall jackd");
	my_system2("killall jackd");
#endif
	
	return 1;
}

int my_system(const char* command)
{
	firsttime = 1;
	int status;
	pid_t pid = fork();
  
	if (pid == 0) {
		execl(SHELL, SHELL, "-c", command, NULL);
		_exit(EXIT_FAILURE);
	} else if (pid < 0) {
		status = -1;
	} else {
		status = 1;
		sleep(4);
	}
	return status;
}

int my_system2(const char* command)
{
	firsttime = 1;
	int status;
	pid_t pid;

	pid = fork();
	if (pid == 0) {
      execl(SHELL, SHELL, "-c", command, NULL);
      _exit(EXIT_FAILURE);
	} else if (pid < 0) {
		status = -1;
	} else {
		status = 1;
	}
	return status;
}

/*
int my_system(const char* command)
{
    firsttime = 1;
    
    switch (fork()) {
        case 0:					// child process 
            switch (fork()) {
                case 0:			// grandchild process 
                    execl(SHELL, SHELL, "-c", command, NULL);
                    _exit(99);
                case - 1:
                    _exit(98);
                default:
                    _exit(0);
            }
        case - 1:			
            return -1;		
            
        default:
            sleep(4);
            return 1;
    }
}

int my_system2(const char* command)
{
    firsttime = 1;
    
    switch (fork()) {
        case 0:					// child process 
            switch (fork()) {
                case 0:			// grandchild process 
                    execl(SHELL, SHELL, "-c", command, NULL);
                    _exit(99);
                case - 1:
                    _exit(98);
                default:
                    _exit(0);
            }
        case - 1:			
            return -1;		
            
        default:
            return 1;
    }
}
 */

int checkJack(void) 
{
    int quanti;
    quanti = quantiProc();
    int i;
    for (i = 0; i < quanti; i++) {
        int test;
	   //test = strcmp("jackd", ottieniNome(i));
		test = strcmp("jackdmp", ottieniNome(i));
		if (test == 0) { 
			flag = ottieniFlag(i); 
			return ottieniPid(i); 
		}
    }
    
    return 0;
}

bool openJackClient(void) 
{
    if (client != NULL) {
        JPLog("openJackClient : first close old client \n");
        jack_client_close(client);
		client = NULL;
    }
    client = jack_client_open("JackPilot", JackNullOption, NULL);
    return (client != NULL);
}

const char** getAllPorts(void)
{
	if (client) {
		return jack_get_ports(client, NULL, NULL, 0);
	} else {
		return 0;
	}
}

void portaPerNumero(int n, char* nomeOut, unsigned long* tipo) 
{
	const char** ports = getAllPorts();
	if (!ports) 
		return;
    if (ports[n] != NULL) {
        strcpy(nomeOut, ports[n]);
        *tipo = getTipoByName(ports[n]);        
	}
	free(ports);
}

int numeroPorte() 
{
 	const char** ports = getAllPorts();
	int i = 0;
	if (ports) {
		for (i = 0; i < JACK_MAX_PORTS; i++) {
			if (ports[i] == NULL) {
				free(ports);
				return i;
			}
		}
	}
	free(ports);
	return i;
}

/*
int numeroPorte_aux() 
{
	const char** ports = getAllPorts();
	int i = 0;
	if (ports) {
		for (i = 0; i < JACK_MAX_PORTS; i++) {
			if (ports[i] == NULL) {
				free(ports);
				return i;
			}
		}
	}
	free(ports);
	return i;
}
*/

int getConnections(void) 
{
    int nConnect = 0;
 	const char** ports = getAllPorts();
    int a = numeroPorte(ports);
    int i;
    for (i = 0; i < a; i++) {
        const char** aa;
        jack_port_t* jp;
        jp = jack_port_by_name(client, ports[i]);
        if (jp != NULL) {
			if ((aa = jack_port_get_all_connections(client, jp)) != NULL) {
				int g = 0;
				while (aa[g] != NULL) {
					g++;
				}
				nConnect += g;
				free(aa);  
			}
		}
    }
	if (ports)
		free(ports);
    return nConnect;
}

int numeroConn(const char** connec) 
{
	int i = 0;
	if (connec) {
		for (i = 0; i < 1024; i++) {
			if (connec[i] == NULL)
				return i;
		}
	}
	return i;
}

void writeStatus(int n) 
{
    status = n;
}

int getStatus(void) 
{
    return status;
} 

int connectPorts(char* da, char* a) 
{
    return jack_connect(client, da, a);
}

int disconnectPorts(char* da, char* a) 
{
    return jack_disconnect(client, da, a);
}

int connessionePerNumero2(int n, char** nomeOut, int len) 
{
    int many = 0;
    char porta[256];
    jack_port_t* jp;
    unsigned long tipo;
    portaPerNumero(n, porta, &tipo);
    jp = jack_port_by_name(client, porta);
    if (jp == NULL) 
		return 0;
    const char** connec = jack_port_get_all_connections(client, jp);
	if (connec != NULL) {
		int nu = numeroConn(connec);
		int i;
		if (nomeOut != NULL) {
			for (i = 0; i < nu && i < len; i++){
				if (connec[i] != NULL && nomeOut[i] != NULL) 
					strcpy(nomeOut[i], connec[i]);
			}
		}
		many = nu;
		free(connec); 
	}
    return many;
}

float cpuLoad(void)
{
    if (client != NULL) {
        float load = (float)jack_cpu_load(client);
        return load;
    }
    return 0.0f;
}

unsigned long getTipoByName(const char* name)
{
    jack_port_t* jp;
    jp = jack_port_by_name(client, name);
    if (jp != NULL) 
		return jack_port_flags(jp);
    return 0;
}

int getSR(void) 
{
    return sr;
}
int getBUF(void) 
{
    return buf;
}
int getDRIVER(void) 
{
    return driver;
}
int getCHAN(void) 
{
    return ch;
}

int getInterface(void) 
{
    return interface;
}

int jackALStore(int inCH, int outCH, int AUTOC, int DEFinput, int DEFoutput, int DEFsystem, int LOGSLevel, char* driverIn, char* driverOut, int HOG, int CLOCK, int MONITOR, int MIDI) 
{
    FILE *prefFile;
    char *path;
    path = (char*)alloca(256*sizeof(char));
    sprintf(path,"%s/Library/Preferences/JAS.jpil",homePath);
    if ((prefFile = fopen(path, "wt")) == NULL) {
        return 0;
    } else {	
		verboseLevel = LOGSLevel;
        hogmode = HOG;
        clockmode = CLOCK;
        monitormode = MONITOR;
        MIDImode = MIDI;
		fprintf(prefFile,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d"
				,inCH, -1, outCH, -1, AUTOC, -1, DEFinput, -1, DEFoutput, -1, DEFsystem, -1, verboseLevel, -1, driverIn, -1, driverOut, -1, HOG, -1, CLOCK, -1, MONITOR, -1, MIDI
				); 
		fclose(prefFile);
    }
    return 1;
}

int jackALLoad(void) 
{
    FILE *prefFile;
    char *path;
    char driverIn[128];
    char driverOut[128];
    path = (char*)alloca(256*sizeof(char));
    sprintf(path,"%s/Library/Preferences/JAS.jpil",homePath);
    if ((prefFile = fopen(path, "rt")) == NULL) {
        inch = 2;
        outch = 2;
        autoc = 1;
        defInput = FALSE;
        defOutput = FALSE;
        defSystem = FALSE;
		verboseLevel = 0;
        hogmode = 0;
        clockmode = 0;
        monitormode = 0;
        MIDImode = 0;
        return 1;
    } else {
		int nullo;
		fscanf(prefFile,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
            &inch,&nullo,&outch,&nullo,&autoc,&nullo,&defInput,&nullo,&defOutput,&nullo,&defSystem,&nullo,&verboseLevel,&nullo,driverIn, &nullo, driverOut, &nullo, &hogmode, &nullo, &clockmode, &nullo, &monitormode, &nullo, &MIDImode); 
 		fclose(prefFile);
    }
    return 1;
}

int getDefInput(void) 
{
    return defInput;
}

int getDefOutput(void) 
{
    return defOutput;
}

int getSysOut(void) 
{
    return defSystem;
}

int getInCH(void) 
{
    return inch;
}

int getOutCH(void) 
{
    return outch;
}

int getAutoC(void) 
{
    return autoc;
}

int getVerboseLevel(void) 
{
	return verboseLevel;
}

int getHogMode(void) 
{
	return hogmode;
}

int getClockMode(void) 
{
	return clockmode;
}

int getMonitorMode(void) 
{
	return monitormode;
}

int getMIDIMode(void) 
{
	return MIDImode;
}

int nameOfClient(int n, char* name) 
{ 
	//!!Fix me, why lots of malloc for only one name!!
    char** array;
	const char** ports = getAllPorts();
    int nporte = numeroPorte(ports);
    array = (char**)malloc(nporte * sizeof(char*));
    int i;
    for (i = 0; i < nporte; i++) {
        array[i] = (char*)malloc(256 * sizeof(char));
		memset(array[i], 0x0, sizeof(char) * 256);
    }
    int quanti;
    ottieniNomeClienti(array, &quanti);
    strcpy(name, array[n]);
    for (i = 0; i < nporte; i++) {
        free(array[i]);
    }
    free(array);
	if (ports)
		free(ports);
    return 1;
}


int quantiClienti(void) 
{
    int quanti;
    ottieniNomeClienti(NULL, &quanti);
    return quanti;
}

int ottieniNomeClienti(char** name, int* quanti) 
{
	const char** ports = getAllPorts();
    int nPorts = numeroPorte(ports);
    int i;
    char idport[256], oldIdport[256];
    int progr = 0;
    for (i = 0; i < nPorts; i++) {
        int a;
        for (a = 0; a < strlen(ports[i]); a++) { 
            idport[a] = ports[i][a];
            if (ports[i][a] == ':') 
				break;
        }
        if (strcmp(&idport[0], &oldIdport[0]) == 0) { 
			goto end; 
		}
        if (name) {
			if (name[progr]) 
				strcpy(name[progr], ports[i]);
		}
        progr++;
    end:
        strcpy(&oldIdport[0], &idport[0]);
    }
    *quanti = progr;
	if (ports) 
		free(ports);
    return 0;
}

void writeHomePath(char *path) 
{
    if (homePath != NULL) 
		free(homePath);
    homePath = (char*)malloc(sizeof(char) * strlen(path) + 1);
	memset(homePath, 0x0, sizeof(char)*strlen(path) + 1);
    strcpy(homePath, path);
}

int getFlagOfJack(void) 
{
    return flag;
}

jack_client_t* getClient(void) 
{
    return client;
}

void getCurrentAudioDevice(char* outName) 
{
    if (outName != NULL) 
		strcpy(outName, &coreAudioDevice[0]);
}

void setCurrentAudioDevice(char* inName) 
{
    if (inName != NULL) 
		strcpy(&coreAudioDevice[0], inName);
}

void JPLog(const char *fmt,...) 
{
	if (verboseLevel != 0) {
		va_list ap;
		va_start(ap, fmt);
		fprintf(stderr,"JP: ");
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}
