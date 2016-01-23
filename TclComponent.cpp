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

#include "PhantomGunsDemo.h"
#include "TclComponent.h"


void* UTclComponent::handle = nullptr;
_Tcl_CreateInterpProto UTclComponent::_Tcl_CreateInterp = nullptr;
_Tcl_EvalProto UTclComponent::_Tcl_Eval = nullptr;
_Tcl_CreateObjCommandProto UTclComponent::_Tcl_CreateObjCommand = nullptr;
_Tcl_SetObjResultProto UTclComponent::_Tcl_SetObjResult;
_Tcl_NewObjProto UTclComponent::_Tcl_NewObj = nullptr;
_Tcl_NewBooleanObjProto UTclComponent::_Tcl_NewBooleanObj = nullptr;
_Tcl_NewLongObjProto UTclComponent::_Tcl_NewLongObj = nullptr;
_Tcl_NewDoubleObjProto UTclComponent::_Tcl_NewDoubleObj = nullptr;
_Tcl_NewStringObjProto UTclComponent::_Tcl_NewStringObj = nullptr;
_Tcl_SetVar2ExProto UTclComponent::_Tcl_SetVar2Ex = nullptr;
_Tcl_GetBooleanFromObjProto UTclComponent::_Tcl_GetBooleanFromObj = nullptr;
_Tcl_GetLongFromObjProto UTclComponent::_Tcl_GetLongFromObj = nullptr;
_Tcl_GetDoubleFromObjProto UTclComponent::_Tcl_GetDoubleFromObj = nullptr;
_Tcl_GetStringFromObjProto UTclComponent::_Tcl_GetStringFromObj = nullptr;

// Sets default values for this component's properties
UTclComponent::UTclComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

}

// Called when the game starts
void UTclComponent::BeginPlay() {
	Super::BeginPlay();
	if (handle == nullptr) {
		auto dllPath = FPaths::Combine(*FPaths::GameDir(), TEXT("ThirdParty/"), TEXT(_TCL_DLL_FNAME_));
		if (FPaths::FileExists(dllPath)) {
			handle = FPlatformProcess::GetDllHandle(*dllPath);
			if (handle == nullptr) { UE_LOG(LogClass, Log, TEXT("Tcl bootstrapping failed")) }
			else {
				FString procName = "";
				procName = "Tcl_CreateInterp";
				_Tcl_CreateInterp = static_cast<_Tcl_CreateInterpProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_Eval";
				_Tcl_Eval = static_cast<_Tcl_EvalProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_CreateObjCommand";
				_Tcl_CreateObjCommand = static_cast<_Tcl_CreateObjCommandProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_SetObjResult";
				_Tcl_SetObjResult = static_cast<_Tcl_SetObjResultProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_NewObj";
				_Tcl_NewObj = static_cast<_Tcl_NewObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_NewBooleanObj";
				_Tcl_NewBooleanObj = static_cast<_Tcl_NewBooleanObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_NewLongObj";
				_Tcl_NewLongObj = static_cast<_Tcl_NewLongObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_NewDoubleObj";
				_Tcl_NewDoubleObj = static_cast<_Tcl_NewDoubleObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_NewStringObj";
				_Tcl_NewStringObj = static_cast<_Tcl_NewStringObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_SetVar2Ex";
				_Tcl_SetVar2Ex = static_cast<_Tcl_SetVar2ExProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_GetBooleanFromObj";
				_Tcl_GetBooleanFromObj = static_cast<_Tcl_GetBooleanFromObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_GetLongFromObj";
				_Tcl_GetLongFromObj = static_cast<_Tcl_GetLongFromObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_GetDoubleFromObj";
				_Tcl_GetDoubleFromObj = static_cast<_Tcl_GetDoubleFromObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				procName = "Tcl_GetStringFromObj";
				_Tcl_GetStringFromObj = static_cast<_Tcl_GetStringFromObjProto>(FPlatformProcess::GetDllExport(handle, *procName));
				if (_Tcl_CreateInterp == nullptr ||
					_Tcl_Eval == nullptr ||
					_Tcl_CreateObjCommand == nullptr ||
					_Tcl_SetObjResult == nullptr ||
					_Tcl_NewObj == nullptr ||
					_Tcl_NewBooleanObj == nullptr ||
					_Tcl_NewLongObj == nullptr ||
					_Tcl_NewDoubleObj == nullptr ||
					_Tcl_NewStringObj == nullptr ||
					_Tcl_SetVar2Ex == nullptr ||
					_Tcl_GetBooleanFromObj == nullptr ||
					_Tcl_GetLongFromObj == nullptr ||
					_Tcl_GetDoubleFromObj == nullptr ||
					_Tcl_GetStringFromObj == nullptr) {
					handle = nullptr;
					UE_LOG(LogClass, Log, TEXT("Bootstrapping one or more functions for Tcl failed!"))
				}
				else {
					interpreter = _Tcl_CreateInterp();
					define<UObject>("NIL", nullptr);
					UE_LOG(LogClass, Log, TEXT("Bootstrapping Tcl and its functions succeeded!"))
				}
			}
		}
		else { UE_LOG(LogClass, Log, TEXT("Cannot find %s for Tcl!"), _TCL_DLL_FNAME_) }
	} else {
		interpreter = _Tcl_CreateInterp();
		define<UObject>("NIL", nullptr);
	}
	
}

void UTclComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UTclComponent::Tcl_FreeInternalRepProc(Tcl_Obj* obj) { }
void UTclComponent::Tcl_DupInternalRepProc(Tcl_Obj* srcPtr, Tcl_Obj* dupPtr) { }
void UTclComponent::Tcl_UpdateStringProc(Tcl_Obj* obj) { }
int UTclComponent::Tcl_SetFromAnyProc(Tcl_Interp* interp, Tcl_Obj* obj) { return 0; }

bool UTclComponent::handleIsMissing() { return handle == nullptr; }

_Tcl_CreateObjCommandProto UTclComponent::get_Tcl_CreateObjCommand() { return _Tcl_CreateObjCommand; }
_Tcl_SetObjResultProto UTclComponent::get_Tcl_SetObjResult() { return _Tcl_SetObjResult; }
_Tcl_SetVar2ExProto UTclComponent::get_Tcl_SetVar2Ex() { return _Tcl_SetVar2Ex; }
_Tcl_NewObjProto UTclComponent::get_Tcl_NewObj() { return _Tcl_NewObj; }
_Tcl_NewBooleanObjProto UTclComponent::get_Tcl_NewBooleanObj() { return _Tcl_NewBooleanObj; }
_Tcl_NewLongObjProto UTclComponent::get_Tcl_NewLongObj() { return _Tcl_NewLongObj; };
_Tcl_NewDoubleObjProto UTclComponent::get_Tcl_NewDoubleObj() { return _Tcl_NewDoubleObj; }
_Tcl_NewStringObjProto UTclComponent::get_Tcl_NewStringObj() { return _Tcl_NewStringObj; }
_Tcl_GetBooleanFromObjProto UTclComponent::get_Tcl_GetBooleanFromObj() { return _Tcl_GetBooleanFromObj; }
_Tcl_GetLongFromObjProto UTclComponent::get_Tcl_GetLongFromObj() { return _Tcl_GetLongFromObj; }
_Tcl_GetDoubleFromObjProto UTclComponent::get_Tcl_GetDoubleFromObj() { return _Tcl_GetDoubleFromObj; }
_Tcl_GetStringFromObjProto UTclComponent::get_Tcl_GetStringFromObj() { return _Tcl_GetStringFromObj; }

int UTclComponent::eval(const char* code) {
	if (handle == nullptr || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
	else { return _Tcl_Eval(interpreter, code); }

}

int32 UTclComponent::Eval(FString Filename, FString Code) {
	FString fname = "";
	if(!Filename.IsEmpty()) {
		fname = FPaths::GameContentDir() + "Scripts/" + Filename;
		if(FPaths::FileExists(fname)) {
			UE_LOG(LogClass, Log, TEXT("Tcl loaded a script file at path: %s"), *fname)
			FFileHelper::LoadFileToString(Code, *fname);
		} else {
			UE_LOG(LogClass, Error, TEXT("File at path: %s doesn't exist! Executing the Code field"), *fname)
		}
	}
	auto status = eval(TCHAR_TO_ANSI(*Code));
	if (status != TCL_OK) { UE_LOG(LogClass, Log, TEXT("Tcl script error for! filepath: '%s' or code: '%s'!"), *fname, *Code) }
	return status;

}