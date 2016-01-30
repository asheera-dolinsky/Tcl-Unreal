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

/*
	TODO: Use in production and pinpoint what's missing, also profile for potential memory leaks.
*/

#pragma once

#define _TCL_DLL_FNAME_ "tcl86t.dll"
#define _STRUCT_OFFSET_ 2
#include <typeinfo>
#include "Api.hpp"
#include "TupleUtils.hpp"
#include "Components/ActorComponent.h"
#include "TclComponent.generated.h"


template<int> struct POPULATE;

template<int numberOfParams> struct COMPILE_ON_PARAMS {
	template<typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static bool EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {
		Tcl_Obj* objects[numberOfParams];
		for (int i=0; i<numberOfParams; i++) { objects[i] = const_cast<Tcl_Obj*>(arguments[i+1]); }
		return POPULATE<numberOfParams+_STRUCT_OFFSET_>::FROM<TupleSpecialization, ParamTypes...>(interpreter, values, objects);
	}
};
template<> struct COMPILE_ON_PARAMS<0> {
	template<typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static bool EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {
		return true;
	}
};

template<typename> struct IS_TARRAY : std::false_type { static const bool OF_UOBJECTS = false; };
template<typename T> struct IS_TARRAY<TArray<T>> : std::true_type {
	static const bool OF_UOBJECTS = std::is_base_of<UObject, remove_pointer<T>::type>::value;
};

template<typename> struct IS_TSUBCLASSOF : std::false_type { static const bool OF_UOBJECTS = false; };
template<typename T> struct IS_TSUBCLASSOF<TSubclassOf<T>> : std::true_type {
	static const bool OF_UOBJECTS = std::is_base_of<UObject, T>::value;
};

template<typename T> struct WrapperContainer {
	T* self;
	FString name;
};

template<typename ReturnType> struct COMPILE_DELEGATE_ON_PARAMS;
template<typename ReturnType, typename ReturnPropertyType> struct SPECIALIZED_DECONSTRUCTOR;
template<typename T> struct NEW_OBJ;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PHANTOMGUNSDEMO_API UTclComponent : public UActorComponent {
	GENERATED_UCLASS_BODY()
protected:
	static void* handle;
	static _Tcl_CreateInterpProto _Tcl_CreateInterp;
	static _Tcl_DeleteInterpProto _Tcl_DeleteInterp;
	static _Tcl_EvalProto _Tcl_Eval;
	static _Tcl_CreateObjCommandProto _Tcl_CreateObjCommand;
	static _Tcl_SetObjResultProto _Tcl_SetObjResult;
	static _Tcl_GetObjResultProto _Tcl_GetObjResult;
	static _Tcl_NewObjProto _Tcl_NewObj;
	static _Tcl_NewBooleanObjProto _Tcl_NewBooleanObj;
	static _Tcl_NewLongObjProto _Tcl_NewLongObj;
	static _Tcl_NewDoubleObjProto _Tcl_NewDoubleObj;
	static _Tcl_NewStringObjProto _Tcl_NewStringObj;
	static _Tcl_NewListObjProto _Tcl_NewListObj;
	static _Tcl_SetVar2ExProto _Tcl_SetVar2Ex;
	static _Tcl_GetBooleanFromObjProto _Tcl_GetBooleanFromObj;
	static _Tcl_GetLongFromObjProto _Tcl_GetLongFromObj;
	static _Tcl_GetDoubleFromObjProto _Tcl_GetDoubleFromObj;
	static _Tcl_GetStringFromObjProto _Tcl_GetStringFromObj;

	Tcl_Interp* interpreter = nullptr;

	int defineNil() {
		static const Tcl_ObjType type = { "NIL", &Tcl_FreeInternalRepProc, &Tcl_DupInternalRepProc, &Tcl_UpdateStringProc, &Tcl_SetFromAnyProc };
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto val = _Tcl_NewObj();
			val->typePtr = &type;
			*val = *(_Tcl_SetVar2Ex(interpreter, "NIL", nullptr, val, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG));
			return TCL_OK;
		}
	}

	int eval(const char*);

	template<typename Last> void collect(TArray<Tcl_Obj*>* collector, Last head) {
		collector->Add(NEW_OBJ<Last>::MAKE(interpreter, head));
	}
	template<typename First, typename... Rest> void collect(TArray<Tcl_Obj*>* collector, First head, Rest... tail) {
		collector->Add(NEW_OBJ<First>::MAKE(interpreter, head));
		collect<Rest...>(collector, tail...);
	}

	template<typename P> Tcl_Obj* tarrayToList(TArray<P> arr) {
		const auto len = arr.Num();
		auto objs = new Tcl_Obj*[len];
		for(int i = 0; i < len; i++) {
			objs[i] = NEW_OBJ<P>::MAKE(interpreter, arr[i]);
		}
		auto result = _Tcl_NewListObj(len, static_cast<ClientData>(objs));
		delete[] objs;
		return result;
	}

	template<typename ReturnType, typename ReturnPropertyType, typename T> int generalizedDeconstructor(FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto wrapper = [](ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) -> int {
				static const int numberOfParams = 2;
				numberOfArgs--;  // proc is counted too
				auto data = static_cast<WrapperContainer<UObject>*>(clientData);
				if (numberOfArgs != numberOfParams) {
					UE_LOG(LogClass, Log, TEXT("Tcl: number of arguments to %s : number of arguments = %d isn't equal to the number of parameters = %d"), *(data->name), numberOfArgs, numberOfParams)
					return TCL_ERROR;
				}
				tuple<UObject*, Tcl_Interp*, FString, T, FString> values;
				get<0>(values) = data->self;
				get<1>(values) = interpreter;
				get<2>(values) = data->name;
				auto ok = COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<UObject*, Tcl_Interp*, FString, T, FString>, T, FString>(interpreter, arguments, values);
				if(!ok) { return TCL_ERROR; }
				ok = apply(&SPECIALIZED_DECONSTRUCTOR<ReturnType, ReturnPropertyType>::ENGAGE<T>, values);
				return ok? TCL_OK : TCL_ERROR;
			};
			const char* fname = TCHAR_TO_ANSI(*name);
			auto data = new WrapperContainer<UObject>({ nullptr, name });
			_Tcl_CreateObjCommand(interpreter, fname, wrapper, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<UObject>);
			data = nullptr;
			return TCL_OK;
		}
	}
public:	
	UTclComponent();
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	static bool handleIsMissing();
	static _Tcl_CreateObjCommandProto get_Tcl_CreateObjCommand();
	static _Tcl_SetObjResultProto get_Tcl_SetObjResult();
	static _Tcl_SetVar2ExProto get_Tcl_SetVar2Ex();
	static _Tcl_NewObjProto get_Tcl_NewObj();
	static _Tcl_NewBooleanObjProto get_Tcl_NewBooleanObj();
	static _Tcl_NewLongObjProto get_Tcl_NewLongObj();
	static _Tcl_NewDoubleObjProto get_Tcl_NewDoubleObj();
	static _Tcl_NewStringObjProto get_Tcl_NewStringObj();
	static _Tcl_GetBooleanFromObjProto get_Tcl_GetBooleanFromObj();
	static _Tcl_GetLongFromObjProto get_Tcl_GetLongFromObj();
	static _Tcl_GetDoubleFromObjProto get_Tcl_GetDoubleFromObj();
	static _Tcl_GetStringFromObjProto get_Tcl_GetStringFromObj();

	static void Tcl_FreeInternalRepProc(Tcl_Obj*);
	static void Tcl_DupInternalRepProc(Tcl_Obj*, Tcl_Obj*);
	static void Tcl_UpdateStringProc(Tcl_Obj *obj);
	static int Tcl_SetFromAnyProc(Tcl_Interp*, Tcl_Obj*);

	template<typename Cls> static void freeWrapperContainer(ClientData clientData) {
		auto data = static_cast<WrapperContainer<Cls>*>(clientData);
		delete data;
		data = nullptr; 
	}
	
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bind(Cls* self, FString name) {
		return COMPILE_DELEGATE_ON_PARAMS<ReturnType>::RUN<Cls, ParamTypes...>(self, name, interpreter);
	}

	template<typename ...ParamTypes> int pack(ParamTypes... args) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			TArray<Tcl_Obj*> collector;
			collect<ParamTypes...>(&collector, args...);
			const auto len = sizeof...(ParamTypes);
			const Tcl_Obj* arguments[len];
			for(int i = 0; i < len; i++) { arguments[i] = collector[i]; }
			_Tcl_SetObjResult(interpreter, _Tcl_NewListObj(len, static_cast<ClientData>(arguments)));
			return TCL_OK;
		}
	}

	template<typename P> int convert(TArray<P> arr) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			_Tcl_SetObjResult(interpreter, tarrayToList(arr));
			return TCL_OK;
		}
	}

	Tcl_Obj* getresult();
	int setresult(Tcl_Obj* result);

	template<typename T, typename P> int addStructDeconstructor(FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			generalizedDeconstructor<P, UStructProperty, T>(name);
			return TCL_OK;
		}
	}
	template<typename T, typename P> int addObjectDeconstructor(FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			generalizedDeconstructor<P, UObjectPropertyBase, T>(name);
			return TCL_OK;
		}
	}
	template<typename T> int addBooleanDeconstructor(FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			generalizedDeconstructor<bool, UBoolProperty, T>(name);
			return TCL_OK;
		}
	}
	template<typename T, typename P> int addNumericDeconstructor(FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			generalizedDeconstructor<P, UNumericProperty, T>(name);
			return TCL_OK;
		}
	}

	template<typename T> int define(FString Location, T* ptr, FString Key = "", int flags = TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) {
		static const auto tname = std::is_base_of<UObject, T>::value? "UObject" : IS_TARRAY<T>::OF_UOBJECTS? "TArray of UObjects" : IS_TSUBCLASSOF<T>::OF_UOBJECTS? "TSubclassOf of UObjects" : typeid(T).name();
		static const FString tnameconv = tname;
		static const Tcl_ObjType type = { tname, &Tcl_FreeInternalRepProc, &Tcl_DupInternalRepProc, &Tcl_UpdateStringProc, &Tcl_SetFromAnyProc };
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto val = _Tcl_NewObj();
			val->internalRep.otherValuePtr = static_cast<ClientData>(ptr);
			val->typePtr = &type;
			if (Key.IsEmpty()) {
				*val = *(_Tcl_SetVar2Ex(interpreter, TCHAR_TO_ANSI(*Location), nullptr, val, flags));
				UE_LOG(LogClass, Log, TEXT("Object of type %s defined in %s for Tcl"), *tnameconv, *Location)
			} else {
				*val = *(_Tcl_SetVar2Ex(interpreter, TCHAR_TO_ANSI(*Location), TCHAR_TO_ANSI(*Key), val, flags));
				UE_LOG(LogClass, Log, TEXT("Object of type %s defined in %s(%s) for Tcl"), *tnameconv, *Location, *Key)
			}
			return TCL_OK;
		}
	}

	UFUNCTION(BlueprintCallable, Category = Tcl)
	int32 Eval(FString Filename, FString Code);
	
};

template<typename T> struct IMPL_CONVERT {  // UStruct, TArray<T>, TSubclassOf<T>, TSubclassOf<T> always has the same class, so it's okay to leave it to the base case
	template<typename> FORCEINLINE static int ON_TARRAY_OF_UOBJECTS(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val, FString parentType, FString gottenType) {
		return _TCL_SKIP_;
	}
	template<typename P> FORCEINLINE static int ON_TARRAY_OF_UOBJECTS(Tcl_Interp* interpreter, Tcl_Obj* obj, TArray<P>* val, FString parentType, FString gottenType) {
		static const FString genericType = "TArray of UObjects";
		// here be dragons!
		// TODO: REWORK THIS PART, as the typechecking is naive
		if(gottenType == genericType) {
			auto ptr = static_cast<TArray<UObject*>*>(obj->internalRep.otherValuePtr);
			typedef remove_pointer<P>::type O;
			for(auto item : *ptr) {
				if(!(item == nullptr || item->IsA(O::StaticClass()))) {
					auto cls = item->GetClass();
					gottenType = (cls == nullptr)? "nullptr" : cls->GetDescription();
					UE_LOG(LogClass, Error, TEXT("Tcl error! Received a TArray containing wrong type of UObjects: '%s'. The TArray should be of the type or subtype: '%s'."), *gottenType, *parentType)
					return TCL_ERROR;
				}
			}
			auto deref = *(static_cast<T*>(obj->internalRep.otherValuePtr));
			*val = deref;
			return TCL_OK;
		}
		UE_LOG(LogClass, Error, TEXT("Tcl error! Received a TArray containing wrong type of UObjects: '%s'. The TArray should be of the type or subtype: '%s'."), *gottenType, *parentType)
		return TCL_ERROR;
	}
	template<bool> FORCEINLINE static int TARRAY_COND(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val, FString parentType, FString gottenType) {
		return _TCL_SKIP_;
	}
	template<> FORCEINLINE static int TARRAY_COND<true>(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val, FString parentType, FString gottenType) {
		return ON_TARRAY_OF_UOBJECTS(interpreter, obj, val, parentType, gottenType);
	}
	template<typename> FORCEINLINE static int ON_TSUBCLASS_OF_UOBJECTS(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val, FString parentType, FString gottenType) {
		return _TCL_SKIP_;
	}
	template<typename P> FORCEINLINE static int ON_TSUBCLASS_OF_UOBJECTS(Tcl_Interp* interpreter, Tcl_Obj* obj, TSubclassOf<P>* val, FString parentType, FString gottenType) {
		static const FString nilName = "NIL";
		static const FString genericType = "TSubclassOf of UObjects";
		if(gottenType == nilName || obj->internalRep.otherValuePtr == nullptr) { return TCL_OK; }
		if(gottenType == genericType) {
			FString name = typeid(P).name();
			auto ptr = static_cast<TSubclassOf<UObject>*>(obj->internalRep.otherValuePtr);
			auto repr = *ptr;
			TSubclassOf<class UObject> cls = *repr;
			TSubclassOf<class UObject> parentcls =  P::StaticClass();
			auto clsvalid = *cls != nullptr;
			auto pass = ((*parentcls != nullptr) && clsvalid)? (*cls)->IsChildOf(*parentcls) : false;
			if(pass) {
				*val = *repr;
				return TCL_OK;
			} else { gottenType = clsvalid? repr->GetDescription() : "nullptr"; }
		}
		UE_LOG(LogClass, Error, TEXT("Tcl error! Received a TSubclassOf containing wrong type of UObjects: '%s'. The TSubclassOf should be of the type or subtype: '%s'."), *gottenType, *parentType)
		return TCL_ERROR;
	}
	template<bool> FORCEINLINE static int TSUBCLASS_COND(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val, FString parentType, FString gottenType) {
		return _TCL_SKIP_;
	}
	template<> FORCEINLINE static int TSUBCLASS_COND<true>(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val, FString parentType, FString gottenType) {
		return ON_TSUBCLASS_OF_UOBJECTS(interpreter, obj, val, parentType, gottenType);
	}
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val) {
		static const FString desiredType = typeid(T).name();
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		if (obj == nullptr || obj->typePtr == nullptr) {
			UE_LOG(LogClass, Error, TEXT("Tcl error! Tcl_Obj* or its typePtr is a nullptr. It should be of type or subtype: '%s'."), *desiredType)
			return TCL_ERROR;
		}
		FString gottenType = obj->typePtr->name;
		auto status = TARRAY_COND<IS_TARRAY<T>::OF_UOBJECTS>(interpreter, obj, val, desiredType, gottenType);
		if(status != _TCL_SKIP_) { return status; }
		status = TSUBCLASS_COND<IS_TSUBCLASSOF<T>::OF_UOBJECTS>(interpreter, obj, val, desiredType, gottenType);
		if(status != _TCL_SKIP_) { return status; }
		if(gottenType == desiredType) {
			if(obj == nullptr || obj->internalRep.otherValuePtr == nullptr) {
				UE_LOG(LogClass, Error, TEXT("Tcl error! Received a nullptr instead of a flat type: '%s'."), *desiredType)
				return TCL_ERROR;
			}  // flat values should never return a nullptr
			auto deref = *(static_cast<T*>(obj->internalRep.otherValuePtr));
			*val = deref;
			return TCL_OK;
		} else {
			UE_LOG(LogClass, Error, TEXT("Tcl error! Received an object of wrong type: '%s'. It should be of type: '%s'."), *gottenType, *desiredType)
			return TCL_ERROR;
		}
	}
};
template<typename T> struct IMPL_CONVERT<T*> {  // Pointers
	template<bool> FORCEINLINE static int ON_UOBJECT(Tcl_Interp* interpreter, Tcl_Obj* obj, T** val, FString parentType, FString gottenType) {
		return _TCL_SKIP_;
	}
	template<> FORCEINLINE static int ON_UOBJECT<true>(Tcl_Interp* interpreter, Tcl_Obj* obj, T** val, FString parentType, FString gottenType) {
		static const FString genericType = "UObject";
		if(gottenType == genericType) {
			auto ptr = static_cast<UObject*>(obj->internalRep.otherValuePtr);
			if(ptr->IsA(T::StaticClass())) {
				*val = static_cast<T*>(obj->internalRep.otherValuePtr);
				return TCL_OK;
			}
		}
		UE_LOG(LogClass, Error, TEXT("Tcl error! Received an object of wrong type: '%s'. It should be of type or subtype: '%s'."), *gottenType, *parentType)
		return TCL_ERROR;
	}
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, T** val) {
		static const FString nilName = "NIL";
		static const FString desiredType = typeid(T).name();
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		if (obj == nullptr || obj->typePtr == nullptr) {
			UE_LOG(LogClass, Error, TEXT("Tcl error! Tcl_Obj* or its typePtr is a nullptr. It should be of type or subtype: '%s'."), *desiredType)
			return TCL_ERROR;
		}
		FString gottenType = obj->typePtr->name;
		if(gottenType == nilName || obj->internalRep.otherValuePtr == nullptr) { return TCL_OK; }
		auto status = ON_UOBJECT<std::is_base_of<UObject, T>::value>(interpreter, obj, val, desiredType, gottenType);
		if(status != _TCL_SKIP_) { return status; }
		if(gottenType == desiredType) {
			*val = static_cast<T*>(obj->internalRep.otherValuePtr);
			return TCL_OK;
		}
		UE_LOG(LogClass, Error, TEXT("Tcl error! Received an object of wrong type: '%s'. It should be of type or subtype: '%s'."), *gottenType, *desiredType)
		return TCL_ERROR;
	}
};
template<> struct IMPL_CONVERT<bool> {  // bool
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, bool* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		int in = 0;
		auto result = UTclComponent::get_Tcl_GetBooleanFromObj()(interpreter, obj, &in);
		*val = !!in;
		return result;
	}
};
template<> struct IMPL_CONVERT<int32> {  // int32
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int32* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int32>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<int64> {  // int64
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int32* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int64>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<float> {  // float
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, float* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		double in = 0.0;
		auto result = UTclComponent::get_Tcl_GetDoubleFromObj()(interpreter, obj, &in);
		*val = static_cast<float>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<FString> {  // FString
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, FString* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		auto result = UTclComponent::get_Tcl_GetStringFromObj()(obj, nullptr);
		*val = result;
		return TCL_OK;
	}
};

template<int idx> struct POPULATE {
	template <typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static bool FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {
		auto result = IMPL_CONVERT<std::tuple_element<idx, TupleSpecialization>::type>::CALL(interpreter, objects[idx-_STRUCT_OFFSET_-1], &(get<idx>(values)));
		return !(result == _TCL_BOOTSTRAP_FAIL_ || result == TCL_ERROR) && POPULATE<idx-1>::FROM<TupleSpecialization, ParamTypes...>(interpreter, values, objects);
	}
};
template<> struct POPULATE<_STRUCT_OFFSET_> {
	template <typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static bool FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {
		return true;
	}
};

template<typename T> struct NEW_OBJ {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, T val) {
		static const auto cleanUpFunc = [](Tcl_Obj* obj) -> void {
				auto ptr = static_cast<T*>(obj->internalRep.otherValuePtr);
				FString tname = obj->typePtr->name;
				delete ptr;
				UE_LOG(LogClass, Log, TEXT("Tcl has garbage collected an object of type: %s"), *tname)
		};
		static const Tcl_ObjType type = { IS_TARRAY<T>::OF_UOBJECTS? "TArray of UObjects" : IS_TSUBCLASSOF<T>::OF_UOBJECTS? "TSubclassOf of UObjects" : typeid(T).name(), cleanUpFunc, &UTclComponent::Tcl_DupInternalRepProc, &UTclComponent::Tcl_UpdateStringProc, &UTclComponent::Tcl_SetFromAnyProc };
		auto obj = UTclComponent::get_Tcl_NewObj()();
		obj->internalRep.otherValuePtr = static_cast<ClientData>(new T(val));
		obj->typePtr = &type;
		FString tname = obj->typePtr->name;
		return obj;
	}
};
template<typename T> struct NEW_OBJ<T*> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, T* val) {
		static const Tcl_ObjType type = { std::is_base_of<UObject, T>::value? "UObject" : typeid(T).name(), &UTclComponent::Tcl_FreeInternalRepProc, &UTclComponent::Tcl_DupInternalRepProc, &UTclComponent::Tcl_UpdateStringProc, &UTclComponent::Tcl_SetFromAnyProc };
		auto obj = UTclComponent::get_Tcl_NewObj()();
		obj->internalRep.otherValuePtr = static_cast<ClientData>(val);
		obj->typePtr = &type;
		return obj;
	}
};
template<> struct NEW_OBJ<bool> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, bool val) {
		return UTclComponent::get_Tcl_NewBooleanObj()(val);
	}
};
template<> struct NEW_OBJ<int32> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, int32 val) {
		return UTclComponent::get_Tcl_NewLongObj()(val);
	}
};
template<> struct NEW_OBJ<uint32> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, uint32 val) {
		return UTclComponent::get_Tcl_NewLongObj()(val);
	}
};
template<> struct NEW_OBJ<int64> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, int64 val) {
		return UTclComponent::get_Tcl_NewLongObj()(val);
	}
};
template<> struct NEW_OBJ<float> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, float val) {
		return UTclComponent::get_Tcl_NewDoubleObj()(val);
	}
};
template<> struct NEW_OBJ<FString> {
	FORCEINLINE static Tcl_Obj* MAKE(Tcl_Interp* interpreter, FString val) {
		return UTclComponent::get_Tcl_NewStringObj()(TCHAR_TO_ANSI(*val), -1);
	}
};

template<typename T> struct PROCESS_RETURN {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, T val) {
		auto obj = NEW_OBJ<T>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<typename T> struct PROCESS_RETURN<T*> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, T* val) {
		auto obj = NEW_OBJ<T*>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<bool> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, bool val) {
		auto obj = NEW_OBJ<bool>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<int32> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int32 val) {
		auto obj = NEW_OBJ<int32>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<uint32> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, uint32 val) {
		auto obj = NEW_OBJ<uint32>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<int64> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int64 val) {
		auto obj = NEW_OBJ<int64>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<float> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, float val) {
		auto obj = NEW_OBJ<float>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<FString> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, FString val) {
		auto obj = NEW_OBJ<FString>::MAKE(interpreter, val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};

template<typename ReturnType> struct COMPILE_DELEGATE_ON_PARAMS {
	template<typename Cls, typename ...ParamTypes> FORCEINLINE static int RUN(Cls* self, FString name, Tcl_Interp* interpreter) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto wrapper = [](ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) -> int {
				const int numberOfParams = sizeof...(ParamTypes);
				numberOfArgs--;  // proc is counted too
				auto data = static_cast<WrapperContainer<Cls>*>(clientData);
				if (numberOfArgs != numberOfParams) {
					UE_LOG(LogClass, Log, TEXT("Tcl: number of arguments to %s : number of arguments = %d isn't equal to the number of parameters = %d"), *(data->name), numberOfArgs, numberOfParams)
					return TCL_ERROR;
				}
				tuple<Cls*, Tcl_Interp*, FString, ParamTypes...> values;
				get<0>(values) = data->self;
				get<1>(values) = interpreter;
				get<2>(values) = data->name;
				auto ok = COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<Cls*, Tcl_Interp*, FString, ParamTypes...>, ParamTypes...>(interpreter, arguments, values);
				if(!ok) { return TCL_ERROR; }
				auto delegateWrapper = [](Cls* self, Tcl_Interp* interpreter, FString name, ParamTypes... args) -> bool {
					TBaseDelegate<ReturnType, ParamTypes...> del;
					del.BindUFunction(self, TCHAR_TO_ANSI(*name));
					if(del.IsBound()) { 
						auto ret = del.Execute(args...);
						PROCESS_RETURN<ReturnType>::USE(interpreter, ret);
					}
					return del.IsBound();
				};
				typedef bool(*DelegateWrapperFptr)(Cls* self, Tcl_Interp*, FString name, ParamTypes...);
				ok = apply(static_cast<DelegateWrapperFptr>(delegateWrapper), values);
				return ok? TCL_OK : TCL_ERROR;
			};
			const char* fname = TCHAR_TO_ANSI(*name);
			auto data = new WrapperContainer<Cls>({ self, name });
			UTclComponent::get_Tcl_CreateObjCommand()(interpreter, fname, wrapper, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<Cls>);
			data = nullptr;
			return TCL_OK;
		}
	}
};
template<> struct COMPILE_DELEGATE_ON_PARAMS<void> {
	template<typename Cls, typename ...ParamTypes> FORCEINLINE static int RUN(Cls* self, FString name, Tcl_Interp* interpreter) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto wrapper = [](ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) -> int {
				const int numberOfParams = sizeof...(ParamTypes);
				numberOfArgs--;  // proc is counted too
				auto data = static_cast<WrapperContainer<Cls>*>(clientData);
				if (numberOfArgs != numberOfParams) {
					UE_LOG(LogClass, Log, TEXT("Tcl: number of arguments to %s : number of arguments = %d isn't equal to the number of parameters = %d"), *(data->name), numberOfArgs, numberOfParams)
					return TCL_ERROR;
				}
				tuple<Cls*, Tcl_Interp*, FString, ParamTypes...> values;
				get<0>(values) = data->self;
				get<1>(values) = interpreter;
				get<2>(values) = data->name;
				auto ok = COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<Cls*, Tcl_Interp*, FString, ParamTypes...>, ParamTypes...>(interpreter, arguments, values);
				if(!ok) { return TCL_ERROR; }
				auto delegateWrapper = [](Cls* self, Tcl_Interp* interpreter, FString name, ParamTypes... args) -> bool {
					TBaseDelegate<void, ParamTypes...> del;
					del.BindUFunction(self, TCHAR_TO_ANSI(*name));
					return del.ExecuteIfBound(args...);
				};
				typedef bool(*DelegateWrapperFptr)(Cls* self, Tcl_Interp*, FString name, ParamTypes...);
				ok = apply(static_cast<DelegateWrapperFptr>(delegateWrapper), values);
				return ok? TCL_OK : TCL_ERROR;
			};
			const char* fname = TCHAR_TO_ANSI(*name);
			auto data = new WrapperContainer<Cls>({ self, name });
			UTclComponent::get_Tcl_CreateObjCommand()(interpreter, fname, wrapper, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<Cls>);
			data = nullptr;
			return TCL_OK;
		}
	}
};

template<typename ReturnType> struct SPECIALIZED_DECONSTRUCTOR<ReturnType, UStructProperty> {
	template<typename T> FORCEINLINE static bool ENGAGE(UObject* self, Tcl_Interp* interpreter, FString name, T retStruct, FString retName) {
		TBaseDelegate<ReturnType, T, FString> del;
		auto wrapper = [](T retStruct, FString name) -> ReturnType {
			ReturnType Value;
			for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
				auto Property = *PropIt;
				if(Property->GetNameCPP() == name) {
					auto CastProperty = Cast<UStructProperty>(Property);
					if (CastProperty != nullptr) {
						auto ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
						Value = *(static_cast<ReturnType*>(ValuePtr));
						break;
					}
				}
			}
			return Value;
		};
		del.BindLambda(wrapper);
		if(del.IsBound()) { 
			auto ret = del.Execute(retStruct, retName);
			PROCESS_RETURN<ReturnType>::USE(interpreter, ret);
		}
		return del.IsBound();
	}
};
template<typename ReturnType> struct SPECIALIZED_DECONSTRUCTOR<ReturnType, UObjectPropertyBase> {
	template<typename T> FORCEINLINE static bool ENGAGE(UObject* self, Tcl_Interp* interpreter, FString name, T retStruct, FString retName) {
		TBaseDelegate<ReturnType, T, FString> del;
		auto wrapper = [](T retStruct, FString name) -> ReturnType {
			ReturnType Value = nullptr;
			for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
				auto Property = *PropIt;
				if(Property->GetNameCPP() == name) {
					auto CastProperty = Cast<UObjectPropertyBase>(Property);
					if (CastProperty != nullptr) {
						auto ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
						Value = static_cast<ReturnType>(ValuePtr);
						break;
					}
				}
			}
			return Value;
		};
		del.BindLambda(wrapper);
		if(del.IsBound()) { 
			auto ret = del.Execute(retStruct, retName);
			PROCESS_RETURN<ReturnType>::USE(interpreter, ret);
		}
		return del.IsBound();
	}
};
template<> struct SPECIALIZED_DECONSTRUCTOR<bool, UBoolProperty> {
	template<typename T> FORCEINLINE static bool ENGAGE(UObject* self, Tcl_Interp* interpreter, FString name, T retStruct, FString retName) {
		TBaseDelegate<bool, T, FString> del;
		auto wrapper = [](T retStruct, FString name) -> bool {
			for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
				auto Property = *PropIt;
				if(Property->GetNameCPP() == name) {
					auto BoolProperty = Cast<UBoolProperty>(Property);
					if (BoolProperty != nullptr) {
						auto ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
						auto BoolValue = BoolProperty->GetPropertyValue(ValuePtr);
						return BoolValue;
					} else { return false; }
				}
			}
			return false;
		};
		del.BindLambda(wrapper);
		if(del.IsBound()) { 
			auto ret = del.Execute(retStruct, retName);
			PROCESS_RETURN<bool>::USE(interpreter, ret);
		}
		return del.IsBound();
	}
};
template<> struct SPECIALIZED_DECONSTRUCTOR<int32, UNumericProperty> {
	template<typename T> FORCEINLINE static bool ENGAGE(UObject* self, Tcl_Interp* interpreter, FString name, T retStruct, FString retName) {
		TBaseDelegate<int32, T, FString> del;
		auto wrapper = [](T retStruct, FString name) -> int32 {
			for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
				auto Property = *PropIt;
				if(Property->GetNameCPP() == name) {
					auto NumericProperty = Cast<UNumericProperty>(Property);
					if (NumericProperty != nullptr && NumericProperty->IsInteger()) {
						auto ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
						auto NumericValue = NumericProperty->GetSignedIntPropertyValue(ValuePtr);
						return NumericValue;
					} else { return 0; }
				}
			}
			return 0;
		};
		del.BindLambda(wrapper);
		if(del.IsBound()) { 
			auto ret = del.Execute(retStruct, retName);
			PROCESS_RETURN<int32>::USE(interpreter, ret);
		}
		return del.IsBound();
	}
};
template<> struct SPECIALIZED_DECONSTRUCTOR<float, UNumericProperty> {
	template<typename T> FORCEINLINE static bool ENGAGE(UObject* self, Tcl_Interp* interpreter, FString name, T retStruct, FString retName) {
		TBaseDelegate<float, T, FString> del;
		auto wrapper = [](T retStruct, FString name) -> float {
			for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
				auto Property = *PropIt;
				if(Property->GetNameCPP() == name) {
					auto NumericProperty = Cast<UNumericProperty>(Property);
					if (NumericProperty != nullptr && NumericProperty->IsFloatingPoint()) {
						auto ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
						auto NumericValue = NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
						return NumericValue;
					} else { return 0.f; }
				}
			}
			return 0.f;
		};
		del.BindLambda(wrapper);
		if(del.IsBound()) { 
			auto ret = del.Execute(retStruct, retName);
			PROCESS_RETURN<float>::USE(interpreter, ret);
		}
		return del.IsBound();
	}
};