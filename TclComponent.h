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
#include <typeinfo>
#include <functional>
#include "Api.hpp"
#include "TupleUtils.hpp"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "TclUnrealEssentials.h"
#include "TclComponent.generated.h"


template<int> struct POPULATE;

template<int numberOfParams> struct COMPILE_ON_PARAMS {
	template<typename TupleSpecialization> FORCEINLINE static bool EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {
		Tcl_Obj* objects[numberOfParams];
		for (int i=0; i<numberOfParams; i++) { objects[i] = const_cast<Tcl_Obj*>(arguments[i+1]); }
		return POPULATE<numberOfParams>::FROM<TupleSpecialization>(interpreter, values, objects);
	}
};
template<> struct COMPILE_ON_PARAMS<0> {
	template<typename TupleSpecialization> FORCEINLINE static bool EXEC(Tcl_Interp* interpreter, Tcl_Obj* const arguments[], TupleSpecialization& values) {
		return true;
	}
};

template<typename> struct IS_TARRAY : std::false_type { static const bool OF_UOBJECTS = false; };
template<typename T> struct IS_TARRAY<TArray<T>> : std::true_type { static const bool OF_UOBJECTS = std::is_base_of<UObject, remove_pointer<T>::type>::value; };

template<typename> struct IS_TSUBCLASSOF : std::false_type { static const bool OF_UOBJECTS = false; };
template<typename T> struct IS_TSUBCLASSOF<TSubclassOf<T>> : std::true_type { static const bool OF_UOBJECTS = std::is_base_of<UObject, T>::value; };

template<typename D> struct WrapperContainer {
	Tcl_Interp* interpreter;
	FString name;
	D* del;
};

template<typename ReturnType, typename ...ParamTypes> struct TCL_WRAPPER;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent)) class PHANTOMGUNSDEMO_API UTclComponent : public UActorComponent {
	GENERATED_UCLASS_BODY()
protected:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInitializeSignature);

	static void* handle;
	static _Tcl_CreateInterpProto _Tcl_CreateInterp;
	static _Tcl_DeleteInterpProto _Tcl_DeleteInterp;
	static _Tcl_EvalProto _Tcl_Eval;
	static _Tcl_CreateObjCommandProto _Tcl_CreateObjCommand;
	static _Tcl_SetObjResultProto _Tcl_SetObjResult;
	static _Tcl_NewObjProto _Tcl_NewObj;
	static _Tcl_IncrRefCountProto _Tcl_IncrRefCount;
	static _Tcl_DecrRefCountProto _Tcl_DecrRefCount;
	static _Tcl_NewBooleanObjProto _Tcl_NewBooleanObj;
	static _Tcl_NewLongObjProto _Tcl_NewLongObj;
	static _Tcl_NewDoubleObjProto _Tcl_NewDoubleObj;
	static _Tcl_NewStringObjProto _Tcl_NewStringObj;
	static _Tcl_NewListObjProto _Tcl_NewListObj;
	static _Tcl_SetVar2ExProto _Tcl_SetVar2Ex;
	static _Tcl_GetVar2ExProto _Tcl_GetVar2Ex;
	static _Tcl_GetBooleanFromObjProto _Tcl_GetBooleanFromObj;
	static _Tcl_GetLongFromObjProto _Tcl_GetLongFromObj;
	static _Tcl_GetDoubleFromObjProto _Tcl_GetDoubleFromObj;
	static _Tcl_GetStringFromObjProto _Tcl_GetStringFromObj;

	Tcl_Interp* interpreter = nullptr;

	Tcl_Obj* buffer = nullptr;

	TArray<TBaseDelegate<void, UTclComponent*>> Initializers;

	int init();

	int eval(const char*);

	template<typename Last> FORCEINLINE static void collect(TArray<Tcl_Obj*>* collector, Last head) {
		collector->Add(NEW_OBJ<Last>::MAKE(head));
	}
	template<typename First, typename... Rest> FORCEINLINE static void collect(TArray<Tcl_Obj*>* collector, First head, Rest... tail) {
		collector->Add(NEW_OBJ<First>::MAKE(head));
		collect<Rest...>(collector, tail...);
	}
	template<typename F, typename ReturnType> struct BIND_CONVERT_SAFE {
		template<typename Cls, typename ...ParamTypes> FORCEINLINE static int IMPL(Tcl_Interp* interpreter, F f, FString name) {
			if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
			else {
				const char* fname = TCHAR_TO_ANSI(*name);
				auto del = new TBaseDelegate<ReturnType, Cls*, ReturnType, ParamTypes...>;
				auto lam = [f](Cls* self, ReturnType deflt, ParamTypes... params) -> ReturnType {
					auto result = deflt;
					if (self != nullptr) { result = (self->*f)(params...); }
					return result;
				};
				del->BindLambda(lam);
				auto data = new WrapperContainer<TBaseDelegate<ReturnType, Cls*, ReturnType, ParamTypes...>>({ interpreter, name, del });
				_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, Cls*, ReturnType, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, Cls*, ReturnType, ParamTypes...>>);
				return TCL_OK;
			}
		}
	};
#pragma warning(disable:4701; disable:4703)
	template<typename F, typename ReturnType> struct BIND_CONVERT {
		template<typename Cls, typename ...ParamTypes> FORCEINLINE static int IMPL(Tcl_Interp* interpreter, F f, FString name) {
			if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
				const char* fname = TCHAR_TO_ANSI(*name);
				auto del = new TBaseDelegate<ReturnType, Cls*, ParamTypes...>;
				auto lam = [f](Cls* self, ParamTypes... params) -> ReturnType {
					ReturnType result;
					if(self != nullptr) { result = (self->*f)(params...); }
					return result;
				};
				del->BindLambda(lam);
				auto data = new WrapperContainer<TBaseDelegate<ReturnType, Cls*, ParamTypes...>>({ interpreter, name, del });
				_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, Cls*, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, Cls*, ParamTypes...>>);
				return TCL_OK;
			}
		}
	};
#pragma warning(default:4701; default:4703)
	template<typename F> struct BIND_CONVERT<F, void> {
		template<typename Cls, typename ...ParamTypes> FORCEINLINE static int IMPL(Tcl_Interp* interpreter, F f, FString name) {
			if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
				const char* fname = TCHAR_TO_ANSI(*name);
				auto del = new TBaseDelegate<void, Cls*, ParamTypes...>;
				auto lam = [f](Cls* self, ParamTypes... params) -> void {
					if(self != nullptr) { (self->*f)(params...); }
				};
				del->BindLambda(lam);
				auto data = new WrapperContainer<TBaseDelegate<void, Cls*, ParamTypes...>>({ interpreter, name, del });
				_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<void, Cls*, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<void, Cls*, ParamTypes...>>);
				return TCL_OK;
			}
		}
	};
	template<typename F, typename ReturnType> struct BIND_FLAT_CONVERT {
		template<typename Cls, typename ...ParamTypes> FORCEINLINE static int IMPL(Tcl_Interp* interpreter, F f, FString name) {
			if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
			else {
				const char* fname = TCHAR_TO_ANSI(*name);
				auto del = new TBaseDelegate<ReturnType, Cls, ParamTypes...>;
				auto lam = [f](Cls self, ParamTypes... params) -> ReturnType { return (self.*f)(params...); };
				del->BindLambda(lam);
				auto data = new WrapperContainer<TBaseDelegate<ReturnType, Cls, ParamTypes...>>({ interpreter, name, del });
				_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, Cls, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, Cls, ParamTypes...>>);
				return TCL_OK;
			}
		}
	};
	template<typename F> struct BIND_FLAT_CONVERT<F, void> {
		template<typename Cls, typename ...ParamTypes> FORCEINLINE static int IMPL(Tcl_Interp* interpreter, F f, FString name) {
			if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
			else {
				const char* fname = TCHAR_TO_ANSI(*name);
				auto del = new TBaseDelegate<void, Cls, ParamTypes...>;
				auto lam = [f](Cls self, ParamTypes... params) -> void { (self.*f)(params...); };
				del->BindLambda(lam);
				auto data = new WrapperContainer<TBaseDelegate<void, Cls, ParamTypes...>>({ interpreter, name, del });
				_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<void, Cls, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<void, Cls, ParamTypes...>>);
				return TCL_OK;
			}
		}
	};
	template<typename RetDel, typename ...ParamTypes> FORCEINLINE RetDel GenerateDelegate(FString Filename, FString Code) {
		RetDel del;
		del.BindLambda([=](ParamTypes... params) -> void {
			this->Fill(UTclComponent::pack(params...));
			this->Eval(Filename, Code);
		});
		return del;
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
	static _Tcl_NewListObjProto get_Tcl_NewListObj();
	static _Tcl_GetBooleanFromObjProto get_Tcl_GetBooleanFromObj();
	static _Tcl_GetLongFromObjProto get_Tcl_GetLongFromObj();
	static _Tcl_GetDoubleFromObjProto get_Tcl_GetDoubleFromObj();
	static _Tcl_GetStringFromObjProto get_Tcl_GetStringFromObj();

	static void Tcl_FreeInternalRepProc(Tcl_Obj*);
	static void Tcl_DupInternalRepProc(Tcl_Obj*, Tcl_Obj*);
	static void Tcl_UpdateStringProc(Tcl_Obj *obj);
	static int Tcl_SetFromAnyProc(Tcl_Interp*, Tcl_Obj*);

	void Fill(Tcl_Obj*);
	Tcl_Obj* Purge();

	template<typename D> static void freeWrapperContainer(ClientData clientData) {
		auto data = static_cast<WrapperContainer<D>*>(clientData);
		delete data->del;
		delete data;
		data = nullptr; 
	}

	template<typename ...ParamTypes> static Tcl_Obj* pack(ParamTypes... args) {
		if (handleIsMissing()) { return nullptr; } else {
			TArray<Tcl_Obj*> collector;
			collect<ParamTypes...>(&collector, args...);
			const auto len = sizeof...(ParamTypes);
			const Tcl_Obj* arguments[len];
			for(int i = 0; i < len; i++) { arguments[i] = collector[i]; }
			return _Tcl_NewListObj(len, static_cast<ClientData>(arguments));
		}
	}
	template<typename P> static Tcl_Obj* convert(TArray<P> arr) {
		if (handleIsMissing()) { return nullptr; } else {
			const auto len = arr.Num();
			auto objs = new Tcl_Obj*[len];
			for(int i = 0; i < len; i++) { objs[i] = NEW_OBJ<P>::MAKE(arr[i]); }
			auto result = _Tcl_NewListObj(len, static_cast<ClientData>(objs));
			delete[] objs;
			return result;
		}
	}

	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bind(Cls* self, FString name, FString tclname = "") {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, ParamTypes...>;
			del->BindUFunction(self, fname);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>({ interpreter, tclname.IsEmpty()? name : tclname, del });
			_Tcl_CreateObjCommand(interpreter, tclname.IsEmpty()? fname : TCHAR_TO_ANSI(*tclname), &TCL_WRAPPER<ReturnType, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>);
			return TCL_OK;
		}
	}
	template<typename ReturnType, typename ...ParamTypes> int bindstatic(ReturnType(*f)(ParamTypes...), FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, ParamTypes...>;
			del->BindStatic(f);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindmethod(Cls* self, ReturnType(Cls::*f)(ParamTypes...), FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, ParamTypes...>;
			del->BindUObject(self, f);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindmethod(Cls* self, ReturnType(Cls::*f)(ParamTypes...) const, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, ParamTypes...>;
			del->BindUObject(self, f);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>);
			return TCL_OK;
		}
	}
	template<typename ReturnType, typename ...ParamTypes> int bindlambda(std::function<ReturnType(ParamTypes...)> f, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, ParamTypes...>;
			del->BindLambda(f);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, ParamTypes...>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindconvert(ReturnType(Cls::*f)(ParamTypes...), FString name) {
		typedef ReturnType(Cls::*F)(ParamTypes...);
		return BIND_CONVERT<F, ReturnType>::IMPL<Cls, ParamTypes...>(interpreter, static_cast<F>(f), name);
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindconvert(ReturnType(Cls::*f)(ParamTypes...) const, FString name) {
		typedef ReturnType(Cls::*F)(ParamTypes...) const;
		return BIND_CONVERT<F, ReturnType>::IMPL<Cls, ParamTypes...>(interpreter, static_cast<F>(f), name);
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindconvertsafe(ReturnType(Cls::*f)(ParamTypes...), FString name) {
		typedef ReturnType(Cls::*F)(ParamTypes...);
		return BIND_CONVERT_SAFE<F, ReturnType>::IMPL<Cls, ParamTypes...>(interpreter, static_cast<F>(f), name);
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindconvertsafe(ReturnType(Cls::*f)(ParamTypes...) const, FString name) {
		typedef ReturnType(Cls::*F)(ParamTypes...) const;
		return BIND_CONVERT_SAFE<F, ReturnType>::IMPL<Cls, ParamTypes...>(interpreter, static_cast<F>(f), name);
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindflatconvert(ReturnType(Cls::*f)(ParamTypes...), FString name) {
		typedef ReturnType(Cls::*F)(ParamTypes...);
		return BIND_FLAT_CONVERT<F, ReturnType>::IMPL<Cls, ParamTypes...>(interpreter, static_cast<F>(f), name);
	}
	template<typename Cls, typename ReturnType, typename ...ParamTypes> int bindflatconvert(ReturnType(Cls::*f)(ParamTypes...) const, FString name) {
		typedef ReturnType(Cls::*F)(ParamTypes...) const;
		return BIND_FLAT_CONVERT<F, ReturnType>::IMPL<Cls, ParamTypes...>(interpreter, static_cast<F>(f), name);
	}
	template<typename Cls, typename ReturnType> int bindaccessorsafe(ReturnType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, Cls*, ReturnType>;
			auto lam = [ptr](Cls* self, ReturnType deflt) -> ReturnType {
				auto result = deflt;
				if (self != nullptr) { result = (self->*ptr); }
				return result;
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, Cls*, ReturnType>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, Cls*, ReturnType>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, Cls*, ReturnType>>);
			return TCL_OK;
		}
	}
#pragma warning(disable:4701; disable:4703)
	template<typename Cls, typename ReturnType> int bindaccessor(ReturnType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, Cls*>;
			auto lam = [ptr](Cls* self) -> ReturnType {
				ReturnType result;
				if (self != nullptr) { result = (self->*ptr); }
				return result;
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, Cls*>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, Cls*>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, Cls*>>);
			return TCL_OK;
		}
	}
#pragma warning(default:4701; default:4703)
	template<typename Cls, typename InputType> int bindmutator(InputType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<void, Cls*, InputType>;
			auto lam = [ptr](Cls* self, InputType val) -> void {
				if (self != nullptr) { (self->*ptr) = val; }
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<void, Cls*, InputType>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<void, Cls*, InputType>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<void, Cls*, InputType>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename ReturnType> int bindflataccessor(ReturnType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType, Cls*>;
			auto lam = [ptr](Cls* self) -> ReturnType {
				ReturnType result;
				if (self != nullptr) { result = (self.*ptr); }
				return result;
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType, Cls*>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType, Cls*>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType, Cls*>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename InputType> int bindflatmutator(InputType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<void, Cls*, InputType>;
			auto lam = [ptr](Cls* self, InputType val) -> void {
				if (self != nullptr) { (self.*ptr) = val; }
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<void, Cls*, InputType>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<void, Cls*, InputType>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<void, Cls*, InputType>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename ReturnType> int bindpointeraccessor(ReturnType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType*, Cls*>;
			auto lam = [ptr](Cls* self) -> ReturnType* {
				ReturnType* result = nullptr;
				if (self != nullptr) { result = &(self->*ptr); }
				return result;
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType*, Cls*>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType*, Cls*>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType*, Cls*>>);
			return TCL_OK;
		}
	}
	template<typename Cls, typename ReturnType> int bindflatpointeraccessor(ReturnType Cls::*ptr, FString name) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; }
		else {
			const char* fname = TCHAR_TO_ANSI(*name);
			auto del = new TBaseDelegate<ReturnType*, Cls*>;
			auto lam = [ptr](Cls* self) -> ReturnType* {
				ReturnType* result = nullptr;
				if (self != nullptr) { result = &(self.*ptr); }
				return result;
			};
			del->BindLambda(lam);
			auto data = new WrapperContainer<TBaseDelegate<ReturnType*, Cls*>>({ interpreter, name, del });
			_Tcl_CreateObjCommand(interpreter, fname, &TCL_WRAPPER<ReturnType*, Cls*>::RUN, static_cast<ClientData>(data), &UTclComponent::freeWrapperContainer<TBaseDelegate<ReturnType*, Cls*>>);
			return TCL_OK;
		}
	}

	template<typename T> int define(FString Location, T val, FString Key = "", int flags = TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) {
		if (handleIsMissing() || interpreter == nullptr) { return _TCL_BOOTSTRAP_FAIL_; } else {
			auto obj = UTclComponent::NEW_OBJ<T>::MAKE(val);
			if (Key.IsEmpty()) {
				_Tcl_SetVar2Ex(interpreter, TCHAR_TO_ANSI(*Location), nullptr, obj, flags);
			} else {
				_Tcl_SetVar2Ex(interpreter, TCHAR_TO_ANSI(*Location), TCHAR_TO_ANSI(*Key), obj, flags);
			}
			return TCL_OK;
		}
	}

	template<typename RetDel, typename ...ParamTypes> int32 registerdelegate(FString name) { return this->bindmethod(this, &UTclComponent::GenerateDelegate<RetDel, ParamTypes...>, name); }

	template<typename T> struct NEW_OBJ {
		FORCEINLINE static Tcl_Obj* MAKE(T val) {
			static const auto cleanUpFunc = [](Tcl_Obj* obj) -> void {
					auto ptr = static_cast<T*>(obj->internalRep.otherValuePtr);
					FString tname = obj->typePtr->name;
					delete ptr;
					//UE_LOG(LogClass, Log, TEXT("Tcl has garbage collected an object of type: %s"), *tname)  // uncomment to profile via logging
			};
			static const Tcl_ObjType type = { IS_TARRAY<T>::OF_UOBJECTS? "TArray of UObjects" : IS_TSUBCLASSOF<T>::OF_UOBJECTS? "TSubclassOf of UObjects" : typeid(T).name(), cleanUpFunc, &Tcl_DupInternalRepProc, &Tcl_UpdateStringProc, &Tcl_SetFromAnyProc };
			auto obj = _Tcl_NewObj();
			obj->internalRep.otherValuePtr = static_cast<ClientData>(new T(val));
			obj->typePtr = &type;
			FString tname = obj->typePtr->name;
			return obj;
		}
	};
	template<typename T> struct NEW_OBJ<T*> {
		FORCEINLINE static Tcl_Obj* MAKE(T* val) {
			static const Tcl_ObjType type = { std::is_base_of<UObject, T>::value? "UObject" : typeid(T).name(), &Tcl_FreeInternalRepProc, &Tcl_DupInternalRepProc, &Tcl_UpdateStringProc, &Tcl_SetFromAnyProc };
			auto obj = _Tcl_NewObj();
			obj->internalRep.otherValuePtr = static_cast<ClientData>(val);
			obj->typePtr = &type;
			return obj;
		}
	};
	template<> struct NEW_OBJ<bool> {
		FORCEINLINE static Tcl_Obj* MAKE(bool val) {
			return _Tcl_NewBooleanObj(val);
		}
	};
	template<> struct NEW_OBJ<int8> {
		FORCEINLINE static Tcl_Obj* MAKE(int8 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<uint8> {
		FORCEINLINE static Tcl_Obj* MAKE(uint8 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<int16> {
		FORCEINLINE static Tcl_Obj* MAKE(int16 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<uint16> {
		FORCEINLINE static Tcl_Obj* MAKE(uint16 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<int32> {
		FORCEINLINE static Tcl_Obj* MAKE(int32 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<uint32> {
		FORCEINLINE static Tcl_Obj* MAKE(uint32 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<int64> {
		FORCEINLINE static Tcl_Obj* MAKE(int64 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<uint64> {
		FORCEINLINE static Tcl_Obj* MAKE(uint64 val) {
			return _Tcl_NewLongObj(val);
		}
	};
	template<> struct NEW_OBJ<float> {
		FORCEINLINE static Tcl_Obj* MAKE(float val) {
			return _Tcl_NewDoubleObj(val);
		}
	};
	template<> struct NEW_OBJ<double> {
		FORCEINLINE static Tcl_Obj* MAKE(double val) {
			return _Tcl_NewDoubleObj(val);
		}
	};
	template<> struct NEW_OBJ<FString> {
		FORCEINLINE static Tcl_Obj* MAKE(FString val) {
			return _Tcl_NewStringObj(TCHAR_TO_ANSI(*val), -1);
		}
	};
	template<> struct NEW_OBJ<FName> {
		FORCEINLINE static Tcl_Obj* MAKE(FName val) {
			return _Tcl_NewStringObj(TCHAR_TO_ANSI(*val.ToString()), -1);
		}
	};

	TBaseDelegate<void, UTclComponent*>* AddInitializer() {
		Initializers.AddDefaulted();
		return &Initializers.Last();
	}

	UPROPERTY(BlueprintAssignable, Category = Events)
	FInitializeSignature OnBeginPlay;

	UPROPERTY(EditDefaultsOnly, Category = Initialization)
	TArray<FString> IncludeScriptPaths;

	UPROPERTY(EditDefaultsOnly, Category = Initialization)
	FString MainScriptPath;

	UFUNCTION(BlueprintCallable, Category = Tcl)
	int32 Eval(FString Filename, FString Code);

	UFUNCTION(BlueprintCallable, Category = Tcl)
	int32 SetObj(UObject* Object, FString Location, FString Key);

	UFUNCTION(BlueprintCallable, Category = Tcl)
	int32 SetClass(TSubclassOf<UObject> Class, FString Location, FString Key);

	UFUNCTION(BlueprintCallable, Category = Tcl)
	int32 GetFloat(FString Location, FString Key, float& Result);

	UFUNCTION(BlueprintCallable, Category = Tcl)
	UObject* GetObj(FString Location, FString Key);
	
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
			} else {
				if(clsvalid) { gottenType = repr->GetDescription(); } else {
					gottenType = "nullptr";
					*val = nullptr;
					return TCL_OK;
				}
			}
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
template<typename T> struct IMPL_CONVERT<const T> {
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, const T* val) {
		T* valStripped = const_cast<T*>(val);
		return IMPL_CONVERT<T>::CALL(interpreter, obj, valStripped);
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
		int in = 0;
		auto result = UTclComponent::get_Tcl_GetBooleanFromObj()(interpreter, obj, &in);
		*val = !!in;
		return result;
	}
};
template<> struct IMPL_CONVERT<int8> {  // int8
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int8* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int8>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<uint8> {  // uint8
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, uint8* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<uint8>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<int16> {  // int16
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int16* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int16>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<uint16> {  // uint16
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, uint16* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<uint16>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<int32> {  // int32
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int32* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int32>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<uint32> {  // uint32
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, uint32* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<uint32>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<int64> {  // int64
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, int32* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<int64>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<uint64> {  // int64
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, uint64* val) {
		long in = 0;
		auto result = UTclComponent::get_Tcl_GetLongFromObj()(interpreter, obj, &in);
		*val = static_cast<uint64>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<float> {  // float
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, float* val) {
		double in = 0.0;
		auto result = UTclComponent::get_Tcl_GetDoubleFromObj()(interpreter, obj, &in);
		*val = static_cast<float>(in);
		return result;
	}
};
template<> struct IMPL_CONVERT<double> {  // double
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, double* val) {
		return UTclComponent::get_Tcl_GetDoubleFromObj()(interpreter, obj, val);;
	}
};
template<> struct IMPL_CONVERT<FString> {  // FString
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, FString* val) {
		auto result = UTclComponent::get_Tcl_GetStringFromObj()(obj, nullptr);
		*val = result;
		return TCL_OK;
	}
};
template<> struct IMPL_CONVERT<FName> {  // FName
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, FName* val) {
		auto result = UTclComponent::get_Tcl_GetStringFromObj()(obj, nullptr);
		*val = result;
		return TCL_OK;
	}
};
template<> struct IMPL_CONVERT<Tcl_Obj*> {  // Tcl_Obj*
	FORCEINLINE static int CALL(Tcl_Interp* interpreter, Tcl_Obj* obj, Tcl_Obj** val) {
		if (obj == nullptr) { obj = UTclComponent::get_Tcl_NewObj()(); }
		*val = obj;
		return TCL_OK;
	}
};

template<int idx> struct POPULATE {
	template <typename TupleSpecialization> FORCEINLINE static bool FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {
		auto result = IMPL_CONVERT<std::tuple_element<idx, TupleSpecialization>::type>::CALL(interpreter, objects[idx-1], &(get<idx>(values)));
		return !(result == _TCL_BOOTSTRAP_FAIL_ || result == TCL_ERROR) && POPULATE<idx-1>::FROM<TupleSpecialization>(interpreter, values, objects);
	}
};
template<> struct POPULATE<0> {
	template <typename TupleSpecialization> FORCEINLINE static bool FROM(Tcl_Interp* interpreter, TupleSpecialization& values, Tcl_Obj* objects[]) {
		return true;
	}
};

template<typename T> struct PROCESS_RETURN {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, T val) {
		auto obj = UTclComponent::NEW_OBJ<T>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<typename T> struct PROCESS_RETURN<T*> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, T* val) {
		auto obj = UTclComponent::NEW_OBJ<T*>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<bool> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, bool val) {
		auto obj = UTclComponent::NEW_OBJ<bool>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<int8> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int8 val) {
		auto obj = UTclComponent::NEW_OBJ<int8>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<uint8> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, uint8 val) {
		auto obj = UTclComponent::NEW_OBJ<uint8>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<int16> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int16 val) {
		auto obj = UTclComponent::NEW_OBJ<int16>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<uint16> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, uint16 val) {
		auto obj = UTclComponent::NEW_OBJ<uint16>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<int32> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int32 val) {
		auto obj = UTclComponent::NEW_OBJ<int32>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<uint32> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, uint32 val) {
		auto obj = UTclComponent::NEW_OBJ<uint32>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<int64> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, int64 val) {
		auto obj = UTclComponent::NEW_OBJ<int64>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<uint64> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, uint64 val) {
		auto obj = UTclComponent::NEW_OBJ<uint64>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<float> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, float val) {
		auto obj = UTclComponent::NEW_OBJ<float>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<double> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, double val) {
		auto obj = UTclComponent::NEW_OBJ<double>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<FString> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, FString val) {
		auto obj = UTclComponent::NEW_OBJ<FString>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<FName> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, FName val) {
		auto obj = UTclComponent::NEW_OBJ<FName>::MAKE(val);
		UTclComponent::get_Tcl_SetObjResult()(interpreter, obj);
	}
};
template<> struct PROCESS_RETURN<Tcl_Obj*> {
	FORCEINLINE static void USE(Tcl_Interp* interpreter, Tcl_Obj* val) {
		if (val == nullptr) { val = UTclComponent::get_Tcl_NewObj()(); }
		UTclComponent::get_Tcl_SetObjResult()(interpreter, val);
	}
};

template<typename T, typename ReturnType> struct COMPILE_DELEGATE_ON_PARAMS {
	template<typename ...ParamTypes> FORCEINLINE static bool GO(T* data, ParamTypes... args) {
		if(data->del->IsBound()) {
			auto ret = data->del->Execute(args...);
			PROCESS_RETURN<ReturnType>::USE(data->interpreter, ret);
		}
		return data->del->IsBound();
	}
};
template<typename T> struct COMPILE_DELEGATE_ON_PARAMS<T, void> {
	template<typename ...ParamTypes> FORCEINLINE static bool GO(T* data, ParamTypes... args) {
		return data->del->ExecuteIfBound(args...);
	}
};

template<typename ReturnType, typename ...ParamTypes> struct TCL_WRAPPER {
	template<typename ...StrippedParamTypes> FORCEINLINE static int RUN_INNER(ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) {
		const int numberOfParams = sizeof...(ParamTypes);
		numberOfArgs--;  // proc is counted too
		auto data = static_cast<WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>*>(clientData);
		if (numberOfArgs != numberOfParams) {
			UE_LOG(LogClass, Error, TEXT("Tcl: number of arguments to %s : number of arguments = %d isn't equal to the number of parameters = %d"), *(data->name), numberOfArgs, numberOfParams)
			return TCL_ERROR;
		}
		tuple<WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>*, typename std::remove_reference<StrippedParamTypes>::type...> values;
		get<0>(values) = data;
		auto ok = COMPILE_ON_PARAMS<numberOfParams>::EXEC<tuple<WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>*, typename std::remove_reference<StrippedParamTypes>::type...>>(interpreter, arguments, values);
		if (!ok) {
			UE_LOG(LogClass, Error, TEXT("Tcl: the offending proc's name is %s"), *(data->name))
			return TCL_ERROR;
		}
		ok = apply(&COMPILE_DELEGATE_ON_PARAMS<WrapperContainer<TBaseDelegate<ReturnType, ParamTypes...>>, ReturnType>::GO<typename std::remove_reference<StrippedParamTypes>::type...>, values);
		return ok? TCL_OK : TCL_ERROR;
	}
	FORCEINLINE static int RUN(ClientData clientData, Tcl_Interp* interpreter, int numberOfArgs, Tcl_Obj* const arguments[]) {
		return RUN_INNER<typename std::remove_const<ParamTypes>::type...>(clientData, interpreter, numberOfArgs, arguments);
	}
};