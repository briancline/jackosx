/*  

    PLua -  Panda Lua (with this you can write your panda modules directly in Lua, even AUDIO MODULES!!!) module.
    written by Giovanni Petrantoni.
    Copyright (c) 2003, Giovanni Petrantoni.
    http://xpanda.sf.net/
    
    main.cpp - main file.
    
    under the terms of the Artistic License :
    
    The Artistic License
    
    Preamble

    The intent of this document is to state the conditions under which a Package may be copied, 
    such that the Copyright Holder maintains some semblance of artistic control over the development of the package, 
    while giving the users of the package the right to use and distribute the Package in a more-or-less customary fashion,
    plus the right to make reasonable modifications.

    Definitions:

    "Package" refers to the collection of files distributed by the Copyright Holder, 
    and derivatives of that collection of files created through textual modification.

    "Standard Version" refers to such a Package if it has not been modified, 
    or has been modified in accordance with the wishes of the Copyright Holder.

    "Copyright Holder" is whoever is named in the copyright or copyrights for the package.

    "You" is you, if you're thinking about copying or distributing this Package.

    "Reasonable copying fee" is whatever you can justify on the basis of media cost, duplication charges, 
    time of people involved, and so on. (You will not be required to justify it to the Copyright Holder, 
    but only to the computing community at large as a market that must bear the fee.)

    "Freely Available" means that no fee is charged for the item itself, 
    though there may be fees involved in handling the item. 
    It also means that recipients of the item may redistribute it under the same conditions they received it.

    1. You may make and give away verbatim copies of the source form of the Standard Version of this Package without restriction, 
    provided that you duplicate all of the original copyright notices and associated disclaimers.

    2. You may apply bug fixes, portability fixes and other modifications derived from the Public Domain or from the Copyright Holder. 
    A Package modified in such a way shall still be considered the Standard Version.

    3. You may otherwise modify your copy of this Package in any way, 
    provided that you insert a prominent notice in each changed file stating how and when you changed that file, 
    and provided that you do at least ONE of the following:

        a) place your modifications in the Public Domain or otherwise make them Freely Available, 
        such as by posting said modifications to Usenet or an equivalent medium, 
        or placing the modifications on a major archive site such as ftp.uu.net, 
        or by allowing the Copyright Holder to include your modifications in the Standard Version of the Package.

        b) use the modified Package only within your corporation or organization.

        c) rename any non-standard executables so the names do not conflict with standard executables, 
        which must also be provided, and provide a separate manual page for each non-standard executable 
        that clearly documents how it differs from the Standard Version.

        d) make other distribution arrangements with the Copyright Holder.

    4. You may distribute the programs of this Package in object code or executable form, 
    provided that you do at least ONE of the following:

    a) distribute a Standard Version of the executables and library files, 
    together with instructions (in the manual page or equivalent) on where to get the Standard Version.

    b) accompany the distribution with the machine-readable source of the Package with your modifications.

    c) accompany any non-standard executables with their corresponding Standard Version executables, 
    giving the non-standard executables non-standard names, and clearly documenting the differences in manual pages (or equivalent), 
    together with instructions on where to get the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

    5. You may charge a reasonable copying fee for any distribution of this Package. 
    You may charge any fee you choose for support of this Package. You may not charge a fee for this Package itself. 
    However, you may distribute this Package in aggregate with other (possibly commercial) programs as part of 
    a larger (possibly commercial) software distribution provided that you do not advertise this Package as a product of your own.

    6. The scripts and library files supplied as input to or produced as output from the programs of this Package do not automatically 
    fall under the copyright of this Package, but belong to whomever generated them, and may be sold commercially, and may be aggregated with this Package.

    7. C or perl subroutines supplied by you and linked into this Package shall not be considered part of this Package.

    8. The name of the Copyright Holder may not be used to endorse or promote products derived from this software without specific prior written permission.

    9. THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.

    The End.

*/


#include <Panda/panda.h>
#include <Panda/variables.h>
#include <PandaLua/lauxlib.h>
#include <PandaLua/lualib.h>

#define ERROR { *err = 1; return x; }

extern int tolua__open(lua_State *);
extern int tolua_variables_open(lua_State *);
extern double tolua_tofieldnumber (lua_State* L, int lo, int index, double def);


struct __myObjData {
    ParamList lista;
	lua_State *state;
};

int HelloPLua(MyData *x,int MODULE_ID,int argc,char **argv) {
	int p_argc = argc-1;
	char *p_argv[p_argc];
	int count = 1;
	int i;
	for(i=0;i<p_argc && argv[count];i++) {
		p_argv[i] = argv[count];
		count++;
	}
	
	changeModulesSearchDirectory(NULL); // turning back to normal default panda settings
	changeModulesBundleExtension(NULL); // to make possible to load panda modules from script
	
	lua_getglobal(x->state,"HelloPanda");
	lua_pushnumber(x->state,MODULE_ID);
	lua_newtable(x->state);
	for(i=0;i<p_argc;i++) {
		lua_pushnumber(x->state,i);
		lua_pushstring(x->state,p_argv[i]);
		lua_rawset(x->state, -3);
	}
	
	lua_pushliteral(x->state, "n");
	lua_pushnumber(x->state,p_argc);
	lua_rawset(x->state, -3);
		
	lua_call(x->state,2,1);
	
	int res = (int)lua_tonumber(x->state,-1);
		
	lua_settop(x->state,0);
	return res;
}


MyData * HelloPanda(int *err,int MODULE_ID,int argc,char **argv) {
    MyData *x;
    x = (MyData*)malloc(sizeof(MyData));
	
    x->lista.subType = 'plua';
    x->lista.outlets = 0;
    x->lista.inlets = 0;
    
    x->lista.audio_outlets = 0;
    x->lista.audio_inlets = 0;
	
	x->state = NULL;
	
	if(!argv[0]) *err = 0;
	else {
		if(!(x->state = lua_open())) ERROR
		if(!tolua__open(x->state)) ERROR
		if(!tolua_variables_open(x->state)) ERROR
		
		luaopen_base(x->state);
		luaopen_table(x->state);
		luaopen_io(x->state);
		luaopen_string(x->state);
		luaopen_math(x->state);
		luaopen_debug(x->state);
		luaopen_loadlib(x->state);
		lua_settop(x->state, 0);
		
		int res = lua_dofile(x->state,argv[0]);
		if(res==0) *err = HelloPLua(x,MODULE_ID,argc,argv);
		else *err = 1;
	}
		    
    x->lista.moduleID = MODULE_ID;
	x->lista.moduleType = ONLY_PARAMETERS;
        
    return x;
}

void FlushModule(MyData *x) {
	if(x->state) {
		lua_getglobal(x->state,"FlushModule");
		lua_call(x->state,0,0);
		lua_close(x->state);
	}
	x->state = NULL;
}

void ShowHelp() {
    PandaLog("PLua -- help\n");
}

void InletCallback(MyData *x,int inlet,Var *inValues) {
    if(inValues==NULL) { 
		lua_getglobal(x->state,"OpenEditor");
		lua_pushnumber(x->state,x->lista.moduleID);
		lua_pushlightuserdata(x->state,(void*)inValues);
		lua_call(x->state,2,0);
		lua_settop(x->state,0);
		return; 
	}
	switch(inlet) {
		case 2:
		{
			lua_getglobal(x->state,"JackStatusChanged");
			lua_pushnumber(x->state,x->lista.moduleID);
			lua_pushlightuserdata(x->state,(void*)inValues);
			lua_call(x->state,2,0);
			lua_settop(x->state,0);
			return; 
		}
		default:
			return;
	}
}

ParamList GetMethodsList(MyData *x) {
    return x->lista;
}

