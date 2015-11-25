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

#include "EmbedTcl.h"
#include "TclWrapper.hpp"

TclWrapper::TclWrapper(bool bootstrapSuccess = true) {
	interpreter = true? _Tcl_CreateInterp() : NULL;
}

TclWrapper::~TclWrapper() {
	// free interpreter here!
}

FString TclWrapper::dllPath;
void* TclWrapper::handle;
_Tcl_CreateInterpProto TclWrapper::_Tcl_CreateInterp;

_Tcl_EvalProto TclWrapper::_Tcl_Eval;
int TclWrapper::eval(const char* code) {
	if (handle == NULL || interpreter == NULL ) { return -1; }
	else { return _Tcl_Eval(interpreter, code); }
}

_Tcl_CreateObjCommandProto TclWrapper::_Tcl_CreateObjCommand;
int TclWrapper::registerFunction(const char* fname, Tcl_ObjCmdProc* f, ClientData clientData = (ClientData) NULL, Tcl_CmdDeleteProc* deleteCallback = (Tcl_CmdDeleteProc*) NULL) {
	if (handle == NULL || interpreter == NULL ) { return -1; } else {
		_Tcl_CreateObjCommand(interpreter, fname, f, clientData, deleteCallback);
		return TCL_OK;
	}
}

_Tcl_GetObjResultProto TclWrapper::_Tcl_GetObjResult;
int TclWrapper::getResult(Tcl_Interp* interpreter, Tcl_Obj** obj) {
	if (handle == NULL || interpreter == NULL ) { return -1; }
	else {
		*obj = _Tcl_GetObjResult(interpreter);
		return TCL_OK;
	}
}

_Tcl_SetIntObjProto TclWrapper::_Tcl_SetIntObj;
int TclWrapper::setInt(Tcl_Obj* obj, int val) {
	if (handle == NULL) { return -1; } else {
		_Tcl_SetIntObj(obj, val);
		return TCL_OK;
	}
}

_Tcl_SetStringObjProto TclWrapper::_Tcl_SetStringObj;
int TclWrapper::setString(Tcl_Obj* obj, const char* str, int len) {
	if (handle == NULL) { return -1; } else {
		_Tcl_SetStringObj(obj, str, len);
		return TCL_OK;
	}
}

_Tcl_GetIntFromObjProto TclWrapper::_Tcl_GetIntFromObj;
int TclWrapper::getInt(Tcl_Interp* interpreter, Tcl_Obj* obj, int* val) {
	if (handle == NULL || interpreter == NULL ) { return -1; }
	else { return _Tcl_GetIntFromObj(interpreter, obj, val); }
}

_Tcl_GetDoubleFromObjProto TclWrapper::_Tcl_GetDoubleFromObj;
int TclWrapper::getDouble(Tcl_Interp* interpreter, Tcl_Obj* obj, double* val) {
	if (handle == NULL || interpreter == NULL ) { return -1; }
	else { return _Tcl_GetDoubleFromObj(interpreter, obj, val); }
}

TSharedRef<TclWrapper> TclWrapper::bootstrap() {
	if (handle != NULL) { return TSharedRef<TclWrapper>(new TclWrapper()); }
	auto dllPath = FPaths::Combine(*FPaths::GameDir(), TEXT("ThirdParty/"), TEXT(_TCL_DLL_FNAME_));
	if (FPaths::FileExists(dllPath)) {
		handle = FPlatformProcess::GetDllHandle(*dllPath);
		if (handle == NULL) { UE_LOG(LogClass, Log, TEXT("Tcl bootstrapping failed")) }
		else {
			FString procName = "";
			procName = "Tcl_CreateInterp";
			_Tcl_CreateInterp = (_Tcl_CreateInterpProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_Eval";
			_Tcl_Eval = (_Tcl_EvalProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_CreateObjCommand";
			_Tcl_CreateObjCommand = (_Tcl_CreateObjCommandProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_GetObjResult";
			_Tcl_GetObjResult = (_Tcl_GetObjResultProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_SetStringObj";
			_Tcl_SetStringObj = (_Tcl_SetStringObjProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_SetIntObj";
			_Tcl_SetIntObj = (_Tcl_SetIntObjProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_GetIntFromObj";
			_Tcl_GetIntFromObj = (_Tcl_GetIntFromObjProto)FPlatformProcess::GetDllExport(handle, *procName);
			procName = "Tcl_GetDoubleFromObj";
			_Tcl_GetDoubleFromObj = (_Tcl_GetDoubleFromObjProto)FPlatformProcess::GetDllExport(handle, *procName);
			if (_Tcl_CreateInterp == NULL ||
				_Tcl_Eval == NULL ||
				_Tcl_CreateObjCommand == NULL ||
				_Tcl_GetObjResult == NULL ||
				_Tcl_SetStringObj == NULL ||
				_Tcl_SetIntObj == NULL ||
				_Tcl_GetIntFromObj == NULL ||
				_Tcl_GetDoubleFromObj == NULL) {
				handle = NULL;
				UE_LOG(LogClass, Log, TEXT("Bootstrapping functions for Tcl failed!"))
			}
			else {
				UE_LOG(LogClass, Log, TEXT("Bootstrapping Tcl and its functions succeeded!"))
				return TSharedRef<TclWrapper>(new TclWrapper());
			}
		}
	}
	else { UE_LOG(LogClass, Log, TEXT("Cannot find %s!"), _TCL_DLL_FNAME_) }
	return TSharedRef<TclWrapper>(new TclWrapper(false));
}