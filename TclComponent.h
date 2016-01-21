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
	TODO:
		1) handle error codes in IMPL_CONVERT for more safety
		2) devise a way to check for nullpointers before going into UFUNCTIONs
		3) make the code as safe as humanly possible
		4) illiminate memory leaks, there is probably a bunch, the first being the interpreter itself once the owner is destroyed
*/

#pragma once

#define _TCL_DLL_FNAME_ "tcl86t.dll"
#define _STRUCT_OFFSET_ 2
#include "Api.hpp"
#include "TupleUtils.hpp"
#include "Components/ActorComponent.h"
#include "TclComponent.generated.h"


template <int> struct POPULATE;

template <int numberOfParams>
struct COMPILE_ON_PARAMS {
	template<typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static void EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {
		Tcl_Obj* objects[numberOfParams];
		for (int i=0; i<numberOfParams; i++) { objects[i] = const_cast<Tcl_Obj*>(arguments[i+1]); }
		POPULATE<numberOfParams+_STRUCT_OFFSET_>::FROM<TupleSpecialization, ParamTypes...>(interpreter, values, objects);
	}
};
template <>
struct COMPILE_ON_PARAMS<0> {
	template<typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static void EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {}
};

template <typename T>
struct WrapperContainer {
	T* self;
	FString name;
};

template <typename ReturnType> struct COMPILE_DELEGATE_ON_PARAMS;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PHANTOMGUNSDEMO_API UTclComponent : public UActorComponent {
	GENERATED_UCLASS_BODY()
protected:
	static void* handle;
	static _Tcl_CreateInterpProto _Tcl_CreateInterp;
	static _Tcl_EvalProto _Tcl_Eval;
	static _Tcl_CreateObjCommandProto _Tcl_CreateObjCommand;
	static _Tcl_SetObjResultProto _Tcl_SetObjResult;
	static _Tcl_NewObjProto _Tcl_NewObj;
	static _Tcl_NewBooleanObjProto _Tcl_NewBooleanObj;
  static _Tcl_NewLongObjProto _Tcl_NewLongObj;
  static _Tcl_NewDoubleObjProto _Tcl_NewDoubleObj;
  static _Tcl_NewStringObjProto _Tcl_NewStringObj;
	static _Tcl_SetVar2ExProto _Tcl_SetVar2Ex;
	static _Tcl_GetBooleanFromObjProto _Tcl_GetBooleanFromObj;
	static _Tcl_GetLongFromObjProto _Tcl_GetLongFromObj;
	static _Tcl_GetDoubleFromObjProto _Tcl_GetDoubleFromObj;
	static _Tcl_GetStringFromObjProto _Tcl_GetStringFromObj;

	Tcl_Interp* interpreter = nullptr;

	int define(char* location, ClientData ptr, char* scope = nullptr, int flags = TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
	int define(char* location, UObject* ptr, char* scope = nullptr, int flags = TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);

	int eval(const char*);
public:	
	// Sets default values for this component's properties
	UTclComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	static bool handleIsMissing();
	static _Tcl_CreateObjCommandProto get_Tcl_CreateObjCommand();
	static _Tcl_SetObjResultProto get_Tcl_SetObjResult();
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

	template <typename Cls> static void freeWrapperContainer(ClientData clientData) {
		auto data = static_cast<WrapperContainer<Cls>*>(clientData);
		delete data;
		data = nullptr; 
	}
	
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bind(Cls* self, FString name) {
		return COMPILE_DELEGATE_ON_PARAMS<ReturnType>::RUN<Cls, ParamTypes...>(self, name, interpreter);
	}

	template<typename ReturnType, typename T> int specializeDeconstructor(FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto wrapper = [](ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) -> int {
				const int numberOfParams = 2;

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

				COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<UObject*, Tcl_Interp*, FString, T, FString>, T, FString>(interpreter, arguments, values);

				auto delegateWrapper = [](UObject* self, Tcl_Interp* interpreter, FString name, T retStruct, FString retName) -> bool {
					TBaseDelegate<ReturnType, T, FString> del;
					//typedef std::tuple_element<0, std::tuple<ParamTypes...>>::type T;
					/*
					auto wrapper = [](T retStruct, FString name) -> bool {
						for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
							UProperty* Property = *PropIt;
							if(Property->GetNameCPP() == name) {
								if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(Property)) {
									void* ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
									auto BoolValue = BoolProperty->GetPropertyValue(ValuePtr);
									return BoolValue;
								} else { return false; }
							}
						}
						return false;
					};
					*/
					auto wrapper = [](T retStruct, FString name) -> float {
						for (TFieldIterator<UProperty> PropIt(T::StaticStruct()); PropIt; ++PropIt) {
							UProperty* Property = *PropIt;
							if(Property->GetNameCPP() == name) {
								UNumericProperty* NumericProperty = Cast<UNumericProperty>(Property);
								if (NumericProperty != nullptr && NumericProperty->IsFloatingPoint()) {
									void* ValuePtr = Property->ContainerPtrToValuePtr<void>(&retStruct);
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
						PROCESS_RETURN<ReturnType>::USE(interpreter, ret);
					}
					return del.IsBound();
				};
				typedef bool(*DelegateWrapperFptr)(UObject* self, Tcl_Interp*, FString name, T, FString);
				auto ok = apply(static_cast<DelegateWrapperFptr>(delegateWrapper), values);
				return ok? TCL_OK : TCL_ERROR;
			};
			const char* fname = TCHAR_TO_ANSI(*name);
			auto data = new WrapperContainer<UObject>({ nullptr, name });
			get_Tcl_CreateObjCommand()(interpreter, fname, wrapper, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<UObject>);
			data = nullptr;
			return TCL_OK;
		}
	}

	template<typename T> int deconstructor(FString name) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			return specializeDeconstructor<float, T>(name);
		}
	}

	UFUNCTION(BlueprintCallable, Category = Tcl)
	void Define(FString Location, FString Key, UObject* Object);

	UFUNCTION(BlueprintCallable, Category = Tcl, CustomThunk, meta = (CustomStructureParam = Struct))
	void DefineStruct(FString Location, FString Key, UProperty* Struct);
	DECLARE_FUNCTION(execDefineStruct) {
		P_GET_PROPERTY(UStrProperty, Location);
		P_GET_PROPERTY(UStrProperty, Key);
		Stack.StepCompiledIn<UStructProperty>(nullptr);
		void* structPtr = Stack.MostRecentPropertyAddress;
		P_FINISH;
		if (Location.IsEmpty()) {
			UE_LOG(LogClass, Error, TEXT("Location must be filled if Key is empty!"))
			return;
		}
		if (Key.IsEmpty()) {
			define(TCHAR_TO_ANSI(*Location), static_cast<ClientData>(structPtr));
		}
		else {
			define(TCHAR_TO_ANSI(*Location), static_cast<ClientData>(structPtr), TCHAR_TO_ANSI(*Key));
		}

	}

	UFUNCTION(BlueprintCallable, Category = Tcl, CustomThunk, meta = (ArrayParm = Objects))
	void DefineMany(FString Location, FString Key, const TArray<int32>& Objects);
	DECLARE_FUNCTION(execDefineMany) {
		P_GET_PROPERTY(UStrProperty, Location);
		P_GET_PROPERTY(UStrProperty, Key);
		Stack.StepCompiledIn<UArrayProperty>(nullptr);
		void* arrPtr = Stack.MostRecentPropertyAddress;
		P_FINISH;
		if (Location.IsEmpty()) {
			UE_LOG(LogClass, Error, TEXT("Location must be filled if Key is empty!"))
			return;
		}
		if (Key.IsEmpty()) {
			define(TCHAR_TO_ANSI(*Location), static_cast<ClientData>(arrPtr));
		}
		else {
			define(TCHAR_TO_ANSI(*Location), static_cast<ClientData>(arrPtr), TCHAR_TO_ANSI(*Key));
		}

	}

	UFUNCTION(BlueprintCallable, Category = Tcl)
	void DefineClass(FString Location, FString Key, UClass* Cls);

	UFUNCTION(BlueprintCallable, Category = Tcl)
	int32 Eval(FString Filename, FString Code);
	
};

template <typename T>  // UStruct and TArray<T>
struct IMPL_CONVERT {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, T* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		auto deref = *(static_cast<T*>(obj->internalRep.otherValuePtr));
		*val = deref;
		return TCL_OK;
	}
};
template <typename T>  // UObject* and TSubjectOf, use bind<...TSubclassOf<T>*...> instead of bind<...TSubclassOf<T>...>
struct IMPL_CONVERT<T*> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, T** val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		*val = static_cast<T*>(obj->internalRep.otherValuePtr);
		return TCL_OK;
	}
};
template <>  // bool
struct IMPL_CONVERT<bool> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, bool* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		int in = 0;
		auto result = UTclComponent::get_Tcl_GetBooleanFromObj()(interpreter, obj, &in);
		*val = !!in;
		return result;
	}
};
template <>  // int32
struct IMPL_CONVERT<int32> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int32* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int32>(in);
		return result;
	}
};
template <>  // int64
struct IMPL_CONVERT<int64> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int32* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int64>(in);
		return result;
	}
};
template <>  // float
struct IMPL_CONVERT<float> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, float* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		double in = 0.0;
		auto result = UTclComponent::get_Tcl_GetDoubleFromObj()(interpreter, obj, &in);
		*val = static_cast<float>(in);
		return result;
	}
};
template <>  // FString
struct IMPL_CONVERT<FString> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, FString* val) {
		if (UTclComponent::handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		auto result = UTclComponent::get_Tcl_GetStringFromObj()(obj, nullptr);
		*val = result;
		return TCL_OK;
	}
};

template <int idx> struct POPULATE {
	template <typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static void FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {
		IMPL_CONVERT<std::tuple_element<idx, TupleSpecialization>::type>::CALL(interpreter, objects[idx-_STRUCT_OFFSET_-1], &(get<idx>(values)));
		POPULATE<idx-1>::FROM<TupleSpecialization, ParamTypes...>(interpreter, values, objects);
	}
};
template <> struct POPULATE<_STRUCT_OFFSET_> {
	template <typename TupleSpecialization, typename ...ParamTypes> FORCEINLINE static void FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {}
};

template <typename T>
struct PROCESS_RETURN {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, T val) {
		auto cleanUpFunc = [](Tcl_Obj* obj) -> void {
				auto ptr = static_cast<T*>(obj->internalRep.otherValuePtr);
				delete ptr;
				delete obj->typePtr;
				UE_LOG(LogClass, Log, TEXT("Tcl has garbage collected an object"))
		};
		auto obj = UTclComponent::get_Tcl_NewObj()();
		obj->internalRep.otherValuePtr = static_cast<ClientData>(new T(val));
		obj->typePtr = new Tcl_ObjType({ "An object constructed within Tcl", cleanUpFunc, &UTclComponent::Tcl_DupInternalRepProc, &UTclComponent::Tcl_UpdateStringProc, &UTclComponent::Tcl_SetFromAnyProc });
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <typename T>
struct PROCESS_RETURN<T*> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, T* val) {
		static const Tcl_ObjType type = { "ClientData", &UTclComponent::Tcl_FreeInternalRepProc, &UTclComponent::Tcl_DupInternalRepProc, &UTclComponent::Tcl_UpdateStringProc, &UTclComponent::Tcl_SetFromAnyProc };
		auto obj = UTclComponent::get_Tcl_NewObj()();
		obj->internalRep.otherValuePtr = static_cast<ClientData>(val);
		obj->typePtr = &type;
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <>
struct PROCESS_RETURN<bool> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, bool val) {
		auto obj = UTclComponent::get_Tcl_NewBooleanObj()(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <>
struct PROCESS_RETURN<int32> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int32 val) {
		auto obj = UTclComponent::get_Tcl_NewLongObj()(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <>
struct PROCESS_RETURN<uint32> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, uint32 val) {
		auto obj = UTclComponent::get_Tcl_NewLongObj()(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <>
struct PROCESS_RETURN<int64> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int64 val) {
		auto obj = UTclComponent::get_Tcl_NewLongObj()(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <>
struct PROCESS_RETURN<float> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, float val) {
		auto obj = UTclComponent::get_Tcl_NewDoubleObj()(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template <>
struct PROCESS_RETURN<FString> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, FString val) {
		auto obj =  UTclComponent::get_Tcl_NewStringObj()(TCHAR_TO_ANSI(*val), -1);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};

template <typename ReturnType> struct COMPILE_DELEGATE_ON_PARAMS {
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

				COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<Cls*, Tcl_Interp*, FString, ParamTypes...>, ParamTypes...>(interpreter, arguments, values);

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
				auto ok = apply(static_cast<DelegateWrapperFptr>(delegateWrapper), values);
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

template <> struct COMPILE_DELEGATE_ON_PARAMS<void> {
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

				COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<Cls*, Tcl_Interp*, FString, ParamTypes...>, ParamTypes...>(interpreter, arguments, values);

				auto delegateWrapper = [](Cls* self, Tcl_Interp* interpreter, FString name, ParamTypes... args) -> bool {
					TBaseDelegate<void, ParamTypes...> del;
					del.BindUFunction(self, TCHAR_TO_ANSI(*name));
					return del.ExecuteIfBound(args...);
				};
				typedef bool(*DelegateWrapperFptr)(Cls* self, Tcl_Interp*, FString name, ParamTypes...);
				auto ok = apply(static_cast<DelegateWrapperFptr>(delegateWrapper), values);
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