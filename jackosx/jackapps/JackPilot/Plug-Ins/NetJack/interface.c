/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */


#include <Panda/panda_modules.h>
#include <JPPlugin.h>

struct __myObjData {
    ParamList lista;
	JPPData *plugData;
};

MyData * HelloPanda(int *err,int MODULE_ID,int argc,char **argv) {
    MyData *x;
    x = (MyData*)malloc(sizeof(MyData));
    
    x->lista.moduleType = ONLY_PARAMETERS;
    
    x->lista.subType = 'coco';
    x->lista.outlets = 0;
    x->lista.inlets = 4;
    
    x->lista.moduleID = MODULE_ID;
	
	if(argc<=0) { *err = -1; return x; }
	
	if(x->plugData = JPP_alloc(argv[0])) *err = 0;
	else *err = -1;
		    
    return x;
}

void FlushModule(MyData *x) {
	JPP_dealloc(x->plugData);
	free(x->plugData);
}

void InletCallback(MyData *x,int inlet,Var *inValues) {
	switch(inlet) {
		case 0:
			JPP_open_editor(x->plugData);
			return;
		case 1:
			return;
		case 2:
			if(inValues) {
				if(inValues->var_t) {
					int newStatus = *(int*)inValues->var_t;
					JPP_jack_status_has_changed(x->plugData,newStatus);
				}
			}
			return;
		case 3:
			if(inValues) {
				inValues->var_t = JPP_get_window_id(x->plugData);
			}
			return;
		default:
			return;
	}
}


ParamList GetMethodsList(MyData *x) {
    return x->lista;
}

