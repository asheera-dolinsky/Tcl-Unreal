/*
*	 The MIT License (MIT)
*
*	 Copyright (c) 2015 Alisa Dolinsky
*
*	 Permission is hereby granted, free of charge, to any person obtaining a copy
*	 of this software and associated documentation files (the "Software"), to deal
*	 in the Software without restriction, including without limitation the rights
*	 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*	 copies of the Software, and to permit persons to whom the Software is
*	 furnished to do so, subject to the following conditions:
*
*	 The above copyright notice and this permission notice shall be included in all
*	 copies or substantial portions of the Software.
*
*	 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*	 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*	 SOFTWARE.
*/

#pragma once

#define _PROJECT_API_ID_ EMBEDTCL_API
#define _TCL_DLL_FNAME_ "tcl86t.dll"

#include "api.hpp"

class _PROJECT_API_ID_ TclWrapper
{
protected:
	TclWrapper(bool, uint32);

	static void* handle;
	static size_t interpreterSize;
	static const char* __id__;

	Tcl_Interp* interpreter;

	int registerId(uint32);

	static _Tcl_CreateInterpProto _Tcl_CreateInterp;
	static _Tcl_EvalProto _Tcl_Eval;
	static _Tcl_CreateObjCommandProto _Tcl_CreateObjCommand;
	
	static _Tcl_ObjSetVar2Proto _Tcl_ObjSetVar2;
	static _Tcl_ObjGetVar2Proto _Tcl_ObjGetVar2;
	static _Tcl_SetStringObjProto _Tcl_SetStringObj;
	static _Tcl_NewStringObjProto _Tcl_NewStringObj;
	static _Tcl_NewLongObjProto _Tcl_NewLongObj;
	static _Tcl_SetIntObjProto _Tcl_SetIntObj;
	static _Tcl_GetObjResultProto _Tcl_GetObjResult;
	static _Tcl_GetIntFromObjProto _Tcl_GetIntFromObj;
	static _Tcl_GetLongFromObjProto _Tcl_GetLongFromObj;
	static _Tcl_GetDoubleFromObjProto _Tcl_GetDoubleFromObj;
public:
	static TSharedRef<TclWrapper> bootstrap(uint32);
	bool bootstrapSuccess();
	int eval(const char*);
	int registerFunction(const char*, Tcl_ObjCmdProc*, ClientData, Tcl_CmdDeleteProc*);
	int define(Tcl_Obj*, Tcl_Obj*, Tcl_Obj*, int);
	int fetch(Tcl_Obj*, Tcl_Obj*, Tcl_Obj**, int);

	static int id(Tcl_Interp*, uint32*);
	int id(uint32*);
	uint32 id();

	static int newString(Tcl_Obj**, const char*);
	static int newLong(Tcl_Obj**, long);

	static int setString(Tcl_Obj*, const char*, int);
	static int setInt(Tcl_Obj*, int);
	static int getResult(Tcl_Interp*, Tcl_Obj**);
	static int toInt(Tcl_Interp*, Tcl_Obj*, int*);
	int toInt(Tcl_Obj*, int*);
	static int toLong(Tcl_Interp*, Tcl_Obj*, long*);
	int toLong(Tcl_Obj*, long*);
	static int toDouble(Tcl_Interp*, Tcl_Obj*, double*);

	~TclWrapper();
};
