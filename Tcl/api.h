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


#include "tcl.h"

typedef Tcl_Interp*(*_Tcl_CreateInterpProto)(void);
typedef void(*_Tcl_DeleteInterpProto)(Tcl_Interp*);
typedef int(*_Tcl_EvalProto)(Tcl_Interp*, const char*);
typedef void(*_Tcl_CreateObjCommandProto)(Tcl_Interp*, const char*, Tcl_ObjCmdProc*, ClientData, Tcl_CmdDeleteProc*);
typedef void(*_Tcl_SetObjResultProto)(Tcl_Interp*, Tcl_Obj*);
typedef Tcl_Obj*(*_Tcl_NewObjProto)(void);
typedef void(*_Tcl_IncrRefCountProto)(Tcl_Obj*, const char*, int);
typedef void(*_Tcl_DecrRefCountProto)(Tcl_Obj*, const char*, int);
typedef Tcl_Obj*(*_Tcl_NewBooleanObjProto)(int);
typedef Tcl_Obj*(*_Tcl_NewLongObjProto)(long);
typedef Tcl_Obj*(*_Tcl_NewDoubleObjProto)(double);
typedef Tcl_Obj*(*_Tcl_NewStringObjProto)(const char*, int);
typedef Tcl_Obj*(*_Tcl_NewListObjProto)(int, ClientData);  // a hack to bypass const checks, ClientData is actually Tcl_Obj* const[]
typedef Tcl_Obj*(*_Tcl_SetVar2ExProto)(Tcl_Interp*, char*, char*, Tcl_Obj*, int);
typedef Tcl_Obj*(*_Tcl_GetVar2ExProto)(Tcl_Interp*, char*, char*, int);
typedef int(*_Tcl_GetBooleanFromObjProto)(Tcl_Interp*, Tcl_Obj*, int*);
typedef int(*_Tcl_GetLongFromObjProto)(Tcl_Interp*, Tcl_Obj*, long*);
typedef int(*_Tcl_GetDoubleFromObjProto)(Tcl_Interp*, Tcl_Obj*, double*);
typedef char*(*_Tcl_GetStringFromObjProto)(Tcl_Obj*, int*);

#define _TCL_BOOTSTRAP_FAIL_ -1
#define _TCL_SKIP_ -2