/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#include <jackfun.h>

#define SHELL "/bin/sh"

int firsttime = 0;
int status;
jack_client_t* client = NULL;
const char** jplist;
const char **connec;
int sr,buf,driver,ch;
int inch,outch,autoc;
int nConnect;
int interface;
char *homePath = NULL;
int flag;
char coreAudioDevice[256];
int defInput,defOutput,defSystem;
int verboseLevel = 0;

int openJack(const char *stringa) 
{
    JPLog("%s\n",stringa);
    int a;
    if (firsttime == 0) {
        int test;
        test = checkJack();
        if(test!=0) { return 1; }
    }
    a = my_system(stringa);
    if (a == 1) { 
        nConnect = 0;
        a = checkJack();
    }
    return a;
}

int closeJack(void) 
{
	if (client != NULL) {
		jack_client_close(client);
		client = NULL;
	}
	
#if 1
    int pid = checkJack();
	if (pid!=0) { kill(pid,SIGQUIT); } //SIGUSR2 is the one that should work (look at jackd.c or engine.c) but doesn't work...
#else
	my_system2("killall jackd"); //this works but to re-start jackd you must close and re open JP (look console logs)
	my_system2("killall jackd");
	my_system2("killall jackd");
#endif
	
	return 1;
}

int 
my_system (const char *command)
{
  firsttime=1;
  int status;
  pid_t pid;

  pid = fork();
  if (pid == 0)
    {
      execl(SHELL, SHELL, "-c", command, NULL);
      _exit(EXIT_FAILURE);
    }
  else if (pid < 0)
    status = -1;
  else {
      status = 1;
      sleep(4);
  }
  return status;
}

int 
my_system2 (const char *command)
{
  firsttime = 1;
  int status;
  pid_t pid;

  pid = fork ();
  if (pid == 0)
    {
      execl (SHELL, SHELL, "-c", command, NULL);
      _exit (EXIT_FAILURE);
    }
  else if (pid < 0)
    status = -1;
  else {
      status = 1;
      }
  return status;
}

int checkJack(void) 
{
    int quanti;
    quanti = quantiProc();
    int i;
    for (i = 0; i < quanti; i++) {
		
        int test;
        test = strcmp("jackd",ottieniNome(i));
		if (test == 0) { 
			flag = ottieniFlag(i); 
			return ottieniPid(i); 
		}
    }
    
    return 0;
}

int openJackClient(void) 
{
    client = jack_client_new("JackPilot");
	return 1;
}

int ottieniPorte(void)  //why not void instead of int return!!??
{
    if(client!=NULL) {
        if (jplist != NULL) {
            free(jplist);
            jplist = NULL;
        }
        jplist = jack_get_ports(client, NULL, NULL, 0);
		return 1;
    }
	jplist = NULL;
    return 1;
}

int portaPerNumero(int n, char *nomeOut, unsigned long *tipo) 
{
    ottieniPorte();
	if(!jplist) return 0;
    if (jplist[n] != NULL) {
        strcpy(nomeOut,jplist[n]);
        *tipo = getTipoByName(jplist[n]);        
        return 1;
    }
    return 0;
}

int numeroPorte(void) 
{
    if (jplist != NULL) { 
        int i = 0;
        while(*jplist != NULL) {
            *jplist++;
            i++;
        }
        int a;
        for(a=0; a< i;a++) {
            *jplist--;
        }
        return i;
    }
    return 0;
}

int getConnections(void) 
{
    nConnect=0;
    ottieniPorte();
    int a = numeroPorte();
    int i;
    for (i=0; i<a ;i++) {
        const char **aa;
        jack_port_t *jp;
        jp = jack_port_by_name(client,jplist[i]);
        if (jp!=NULL) if((aa = jack_port_get_all_connections(client,jp))!=NULL) {
            int g = 0;
            while(aa[g]!=NULL) {
                g++;
            }
            nConnect += g;
        }
    }
    return nConnect;
}

int numeroConn(void) 
{
    int i = 0;
    while(*connec!=NULL) {
        *connec++;
        i++;
    }
    int a;
    for(a=0; a<i ;a++) {
        *connec--;
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
    return jack_connect(client,da,a);
}

int disconnectPorts(char* da, char* a) 
{
    return jack_disconnect(client,da,a);
}

int connessionePerNumero(int n, char* nomeOut) 
{
    char porta[256];
    jack_port_t* jp;
    strcpy(nomeOut,"");
    unsigned long tipo;
    portaPerNumero(n,&porta[0],&tipo);
    jp = jack_port_by_name(client,&porta[0]);
    if (jp==NULL) return 0;
    connec = jack_port_get_all_connections(client,jp);
    if (connec!=NULL) {
        int nu = numeroConn();
        int i;
        for (i=0; i<nu ;i++){
            if (connec[i] && nomeOut) strcat(nomeOut,connec[i]);
            return 1;
        }
    }
    return 0;
}

int connessionePerNumero2(int n, char** nomeOut) 
{
    int many = 0;
    char porta[256];
    jack_port_t* jp;
    unsigned long tipo;
    portaPerNumero(n,&porta[0],&tipo);
    jp = jack_port_by_name(client,&porta[0]);
    if (jp == NULL) return 0;
    connec = jack_port_get_all_connections(client,jp);
	if (connec!=NULL) {
		int nu = numeroConn();
		int i;
		if (nomeOut!=NULL) {
			for (i=0; i<nu; i++){
				if (connec[i]!=NULL && nomeOut[i]!=NULL) strcpy(nomeOut[i],connec[i]);
			}
		}
		many = nu;
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
    jp = jack_port_by_name(client,name);
    if (jp!=NULL) return jack_port_flags(jp);
    return 0;
}

int loadPrefStat(void) 
{
    FILE *jackpref;
    char *path;
    path = (char*)alloca(256*sizeof(char));
    sprintf(path,"%s/Library/Preferences/JackPilot.jpil",homePath);
    if ((jackpref = fopen(path, "rt")) == NULL) {
        sr=44100;
        buf=512;
        ch=2;
        driver=0;
        interface = 0;
        return 22;
    } else {
        int nullo;
        fscanf(jackpref,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",&driver, &nullo,&interface,&nullo,&sr,&nullo,&buf,&nullo,&ch); 
    }
    return 0;
}

int savePrefStat(int DRIVER, int INTERFACE, int SR, int BUF, int CH) 
{
    FILE *jackpref;
    char *path;
    path = (char*)alloca(256*sizeof(char));
    sprintf(path,"%s/Library/Preferences/JackPilot.jpil",homePath);
    if ((jackpref = fopen(path, "wt")) == NULL) {
        return 22;
    }
    
    fprintf(jackpref,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",DRIVER,-1,INTERFACE,-1,SR,-1,BUF,-1,CH);  
    fclose(jackpref);
    return 0;
}

int getSR(void) {
    return sr;
}
int getBUF(void) {
    return buf;
}
int getDRIVER(void) {
    return driver;
}
int getCHAN(void) {
    return ch;
}

int getInterface(void) {
    return interface;
}

int jackALStore(int inCH,int outCH,int AUTOC,int DEFinput,int DEFoutput,int DEFsystem,int LOGSLevel,int audioDevID) {
    FILE *prefFile;
    char *path;
    path = (char*)alloca(256*sizeof(char));
    sprintf(path,"%s/Library/Preferences/JAS.jpil",homePath);
    if ((prefFile = fopen(path, "wt")) == NULL) {
        return 22;
    } else {	
		verboseLevel = LOGSLevel;
		fprintf(		prefFile,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d"
						,inCH, -1, outCH,-1,AUTOC,-1,DEFinput,-1,DEFoutput,-1,DEFsystem,-1,verboseLevel,-1,audioDevID
				); 
		fclose(prefFile);
    }
    return 1;
}

int jackALLoad(void) 
{
    FILE *prefFile;
    char *path;
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
        return 1;
    } else {
		int nullo;
		fscanf(prefFile,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",&inch,&nullo,&outch,&nullo,&autoc,&nullo,&defInput,&nullo,&defOutput,&nullo,&defSystem,&nullo,&verboseLevel); 
    
		fclose(prefFile);
    }
    return 1;
}

int getDefInput(void) {
    return defInput;
}

int getDefOutput(void) {
    return defOutput;
}

int getSysOut(void) {
    return defSystem;
}

int getInCH(void) {
    return inch;
}

int getOutCH(void) {
    return outch;
}

int getAutoC(void) {
    return autoc;
}

int getVerboseLevel(void) {
	return verboseLevel;
}

int nomeCliente(int n, char* nome) 
{ 
	//!!Fix me, why lots of malloc for only one name!!
    char **array;
    int nporte = numeroPorte();
    array = (char**)malloc(nporte*sizeof(char*));
    int i;
    for(i=0;i<nporte;i++) {
        array[i] = (char*)malloc(256*sizeof(char));
		memset(array[i],0x0,sizeof(char)*256);
    }
    int quanti;
    ottieniNomeClienti(array,&quanti);
    strcpy(nome,array[n]);
    for(i=0;i<nporte;i++) {
        free(array[i]);
    }
    free(array);
    return 1;
}

int quantiClienti(void) 
{
    int quanti;
    ottieniNomeClienti(NULL,&quanti);
    return quanti;
}

int ottieniNomeClienti(char** nome, int* quanti) 
{
    ottieniPorte();
    int nPorts = numeroPorte();
    int i;
    char idport[256],oldIdport[256];
    int progr = 0;
    for (i=0;i<nPorts;i++) {
        int a;
        for (a = 0; a < strlen(jplist[i]); a++) { 
            idport[a] = jplist[i][a];
            if (jplist[i][a] == ':') break;
        }
        if(strcmp(&idport[0],&oldIdport[0])==0) { goto end; }
        if(nome) { if(nome[progr]) strcpy(nome[progr],jplist[i]); }
        progr++;
    end:
        strcpy(&oldIdport[0],&idport[0]);
    }
    *quanti = progr;
    return 0;
}

void writeHomePath(char *path) 
{
    if(homePath!=NULL) free(homePath);
    homePath = (char*)malloc(sizeof(char)*strlen(path)+1);
	memset(homePath,0x0,sizeof(char)*strlen(path)+1);
    strcpy(homePath,path);
}

int getFlagOfJack(void) {
    return flag;
}

jack_client_t* getClient(void) {
    return client;
}

void getCurrentAudioDevice(char* outName) {
    if(outName!=NULL) strcpy(outName,&coreAudioDevice[0]);
}
void setCurrentAudioDevice(char* inName) {
    if(inName!=NULL) strcpy(&coreAudioDevice[0],inName);
}

void JPLog(char *fmt,...) {
	if(verboseLevel!=0) {
		va_list ap;
		va_start(ap, fmt);
		fprintf(stderr,"JP: ");
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}
