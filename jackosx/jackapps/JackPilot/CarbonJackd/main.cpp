/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#include <Carbon/Carbon.h>
#include <Jackd/jackd.h>
#include "CAPThread.h"
#include "AboutWindow.h"

typedef struct _args {
    int argc;
    char **argv;
    int err;
}Args;

const EventTypeSpec kCmdEvents[] = 
{
	{ kEventClassCommand, kEventCommandProcess }
};


static OSStatus		CommandHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );
void routine(void *para);
void error(int err);

int main(int argc, char* argv[])
{
    IBNibRef 		nibRef;
    OSStatus		err;
    CAPThread *theThread;

    
    Args args;
    args.argc = argc;
    args.argv = argv;
    args.err = 0;
    
        
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr( err, CantGetNibRef );
    
    
    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    require_noerr( err, CantSetMenuBar );
    
    InstallApplicationEventHandler( NewEventHandlerUPP( CommandHandler ),GetEventTypeCount( kCmdEvents ),kCmdEvents, 0, NULL );

    DisposeNibReference(nibRef);
    
    DrawMenuBar();
    
    theThread = new CAPThread(routine,&args,CAPThread::kDefaultThreadPriority);
    theThread->Start();

    sleep(4);
    
    if(args.err!=0) { error(args.err); goto exit; }

    
    RunApplicationEventLoop();
	        
    delete theThread;

exit:
CantSetMenuBar:
CantGetNibRef:
	return err;
}


static pascal OSStatus
CommandHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
        OSStatus		result = eventNotHandledErr;
	HICommand		command;
	
	GetEventParameter( inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof( HICommand ), NULL, &command );
        	
        switch (command.commandID)
        {
            case 'abou':
                AboutWindow::Present(CFSTR("About"));
                result = noErr;
                break;
            case kHICommandQuit:
                AboutWindow::Present(CFSTR("About1"));
                result = noErr;
                break;
            default:
                return noErr;
        }
        return noErr;
}

void routine(void *para) {
    Args *arg = (Args*)para;
    arg->err = jackdmain(arg->argc,arg->argv);
}

void error(int err) {
    switch(err) {
        case -1:
            AboutWindow::Present(CFSTR("err-1"));
            break;
        case -2:
            AboutWindow::Present(CFSTR("err-2"));
            break;
        case -3:
            AboutWindow::Present(CFSTR("err-3"));
            break;
    }
}



