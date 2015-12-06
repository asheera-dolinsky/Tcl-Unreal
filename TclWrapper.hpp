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
#include "tupleUtils.hpp"

template <int> struct POPULATE;

template <int numberOfParams>
struct COMPILE_ON_PARAMS {
	template<typename TupleSpecialization, typename ...ParamTypes> static inline void EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {
		Tcl_Obj* objects[numberOfParams];
		for (int i=0; i<numberOfParams; i++) { objects[i] = const_cast<Tcl_Obj*>(arguments[i+1]); }
		POPULATE<numberOfParams+1>::FROM<TupleSpecialization, ParamTypes...>(interpreter, values, objects);
	}
};
template <>
struct COMPILE_ON_PARAMS<0> {
	template<typename TupleSpecialization, typename ...ParamTypes> static inline void EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {}
};

template <typename T>
struct WrapperContainer {
	T* self;
	FString name;
};

class _PROJECT_API_ID_ TclWrapper
{
protected:
	TclWrapper(bool, uint32);

	static void* handle;
	static size_t interpreterSize;
	static const char* __id__;

	Tcl_Interp* interpreter;

	int registerId(uint32);

	template <typename Cls> static void freeWrapperContainer(ClientData clientData) {
		auto data = (WrapperContainer<Cls>*)clientData;
		delete data;
		data = nullptr; 
	}

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
	static uint32 id(Tcl_Interp*);
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

	template <typename T> static int TclWrapper::convert(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val) {
		return 0;
	}
	template <> static int TclWrapper::convert<int>(Tcl_Interp* interpreter, Tcl_Obj* obj, int* val) {
		if (handle == nullptr || interpreter == nullptr ) { return _TCL_BOOTSTRAP_FAIL_; }
		else { return _Tcl_GetIntFromObj(interpreter, obj, val); }
	}
	template <> static int TclWrapper::convert<long>(Tcl_Interp* interpreter, Tcl_Obj* obj, long* val) {
		if (handle == nullptr || interpreter == nullptr ) { return _TCL_BOOTSTRAP_FAIL_; }
		else { return _Tcl_GetLongFromObj(interpreter, obj, val); }
	}
	template <> static int TclWrapper::convert<int64>(Tcl_Interp* interpreter, Tcl_Obj* obj, int64* val) {
		if (handle == nullptr || interpreter == nullptr ) { return _TCL_BOOTSTRAP_FAIL_; }
		else {
			long in = 0;
			auto result = _Tcl_GetLongFromObj(interpreter, obj, &in);
			*val = static_cast<int64>(in);
			return result;
		}
	}
	template <> static int TclWrapper::convert<float>(Tcl_Interp* interpreter, Tcl_Obj* obj, float* val) {
		if (handle == nullptr || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		else {
			double in = 0.0;
			auto result = _Tcl_GetDoubleFromObj(interpreter, obj, &in);
			*val = static_cast<float>(in);
			return result;
		}
	}
	template <> static int TclWrapper::convert<double>(Tcl_Interp* interpreter, Tcl_Obj* obj, double* val) {
		if (handle == nullptr || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		else { return _Tcl_GetDoubleFromObj(interpreter, obj, val); }
	}

	~TclWrapper();

	template<typename Cls, typename ReturnType, typename ...ParamTypes> int TclWrapper::bind(Cls* self, FString name) {
		if (handle == nullptr || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto wrapper = [](ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) -> int {
				const int numberOfParams = sizeof...(ParamTypes);

				numberOfArgs--;  // proc is counted too

				if (numberOfArgs!=numberOfParams) {
					UE_LOG(LogClass, Log, TEXT("Tcl: number of arguments -> %d isn't equal to the number of parameters %d"), numberOfArgs, numberOfParams)
					return TCL_ERROR;
				}

				auto data = (WrapperContainer<Cls>*)clientData;
				tuple<Cls*, FString, ParamTypes...> values;
				get<0>(values) = data->self;
				get<1>(values) = data->name;

				COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<Cls*, FString, ParamTypes...>, ParamTypes...>(interpreter, arguments, values);

				auto delegateWrapper = [](Cls* self, FString name, ParamTypes... args) -> bool {
					TBaseDelegate<ReturnType, ParamTypes...> del;
					del.BindUFunction(self, TCHAR_TO_ANSI(*name));
					return del.ExecuteIfBound(args...);
				};
				typedef bool(*DelegateWrapperFptr)(Cls* self, FString name, ParamTypes...);
				apply((DelegateWrapperFptr)delegateWrapper, values);
				return TCL_OK;
			};
			const char* fname = TCHAR_TO_ANSI(*name);
			auto data = new WrapperContainer<Cls>({ self, name });
			_Tcl_CreateObjCommand(interpreter, fname, wrapper, (ClientData)data, &TclWrapper::freeWrapperContainer<Cls>);
			data = nullptr;
			return TCL_OK;
		}
	}
};

template <int idx> struct POPULATE {
	template <typename TupleSpecialization, typename ...ParamTypes> static inline void FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {
		TclWrapper::convert(interpreter, objects[idx-2], &(get<idx>(values)));
		POPULATE<idx-1>::FROM<TupleSpecialization, ParamTypes...>(interpreter, values, objects);
	}
};
template <> struct POPULATE<1> {
	template <typename TupleSpecialization, typename ...ParamTypes> static inline void FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {}
};