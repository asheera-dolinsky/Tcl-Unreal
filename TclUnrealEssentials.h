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

#include "TclUnrealEssentials.generated.h"


UCLASS() class UTclUnrealEssentials : public UBlueprintFunctionLibrary {
  GENERATED_UCLASS_BODY()
private:
	template<typename T> struct SPECIALIZED_ACCESSOR  {
		FORCEINLINE static TTuple<bool, T> ENGAGE(UObject* self, UProperty* prop) {  // no need to check for prop nullptr, already done in GENERAL_ACCESSOR
			auto result = T();
			auto success = false;
			if(self != nullptr) {
				auto cast = Cast<UProperty>(prop);
				success = cast != nullptr;
				if (success) {
					auto valPtr = prop->ContainerPtrToValuePtr<T>(self);
					result = *valPtr;
				}
			}
			return TTuple<bool, T>(success, result);
		}
	};
	template<> struct SPECIALIZED_ACCESSOR<float> {
		FORCEINLINE static TTuple<bool, float> ENGAGE(UObject* self, UProperty* prop) {  // no need to check for prop nullptr, already done in GENERAL_ACCESSOR
			auto result = 0.f;
			auto success = false;
			if(self != nullptr) {
				auto cast = Cast<UNumericProperty>(prop);
				success = cast != nullptr && cast->IsFloatingPoint();
				if (success) {
					auto valPtr = prop->ContainerPtrToValuePtr<void>(self);
					result = cast->GetFloatingPointPropertyValue(valPtr);
				}
			}
			return TTuple<bool, float>(success, result);
		}
	};

	template<typename ReturnType> struct SPECIALIZED_MUTATOR {
		//kept empty to raise a compile time error if not used with a proper type
	};
	template<> struct SPECIALIZED_MUTATOR<float> {
		FORCEINLINE static bool ENGAGE(UObject* self, UProperty* prop, float val) {  // no need to check for prop nullptr, already done in GENERAL_MUTATOR
			auto success = false;
			if(self != nullptr) {
				auto cast = Cast<UNumericProperty>(prop);
				success = cast != nullptr && cast->IsFloatingPoint();
				if (success) {
					auto valPtr = prop->ContainerPtrToValuePtr<void>(self);
					cast->SetFloatingPointPropertyValue(valPtr, val);
				}
			}
			return success;
		}
	};
	template<> struct SPECIALIZED_MUTATOR<int64> {
		FORCEINLINE static bool ENGAGE(UObject* self, UProperty* prop, int64 val) {  // no need to check for prop nullptr, already done in GENERAL_MUTATOR
			auto success = false;
			if(self != nullptr) {
				auto cast = Cast<UNumericProperty>(prop);
				success = cast != nullptr && cast->IsInteger();
				if (success) {
					auto valPtr = prop->ContainerPtrToValuePtr<void>(self);
					cast->SetIntPropertyValue(valPtr, val);
				}
			}
			return success;
		}
	};
public:
	static TSubclassOf<UObject> FindClass(FString);
	static TArray<AActor*> AllActorsOf(UWorld*, TSubclassOf<AActor>);
	static UActorComponent* FindComponentOf(AActor*, TSubclassOf<UActorComponent>);
	static Tcl_Obj* LineTraceSingleByChannel(UWorld*, FVector, FVector, int32);
	static Tcl_Obj* SweepSingleByChannel(UWorld*, FVector, FVector, FQuat, FCollisionShape, int32);
	static FVector GetActorLocation(AActor*);
	static Tcl_Obj* SetActorLocation(AActor*, FVector, bool, int32);
	static Tcl_Obj* AddActorWorldOffset(AActor*, FVector, bool, int32);
	static Tcl_Obj* FindComponentsOfByTag(AActor*, TSubclassOf<UActorComponent>, FName);
	static Tcl_Obj* Purge(AActor*);
	static int32 Eval(AActor*, FString, FString);
	static void PrintString(FString, int32);
	static FString TypeOf(Tcl_Obj* obj);

	template<typename Ret, typename Left, typename Right> struct ADD {
		FORCEINLINE static Ret CONCRETE(Left First, Right Second) { return First + Second; }
	};
	template<typename Ret, typename Left, typename Right> struct SUB {
		FORCEINLINE static Ret CONCRETE(Left First, Right Second) { return First - Second; }
	};
	template<typename Ret, typename Left, typename Right> struct MUL {
		FORCEINLINE static Ret CONCRETE(Left First, Right Second) { return First * Second; }
	};
	template<typename Ret, typename Left, typename Right> struct DIV {
		FORCEINLINE static Ret CONCRETE(Left First, Right Second) { return First / Second; }
	};
	template<typename T, typename ...ParamTypes> struct MAKE {
		FORCEINLINE static T CONCRETE(ParamTypes... args) { return T(args...); }
	};
	template<typename T> struct EQ {
		FORCEINLINE static bool CONCRETE(T First, T Second) { return First == Second; }
	};

#pragma warning(disable:4701) 
	template<typename ReturnType> struct GENERAL_ACCESSOR {
		FORCEINLINE static ReturnType CONCRETE(UObject* self, FString name) {
			auto success = false;
			ReturnType result;
			FString clsName;
			TSubclassOf<UObject> cls = self == nullptr? nullptr : self->GetClass();
			if(cls != nullptr && cls->IsValidLowLevel() && !name.IsEmpty()) {
				clsName = self->GetName();
				for (TFieldIterator<UProperty> propIt(cls); propIt; ++propIt) {
					auto prop = *propIt;
					if(prop != nullptr && prop->GetNameCPP() == name) {
						auto returned = SPECIALIZED_ACCESSOR<ReturnType>::ENGAGE(self, prop);
						success = returned.Get<0>();
						result = returned.Get<1>();
					}
				}
			}
			if(!success) {
				result = SPECIALIZED_ACCESSOR<ReturnType>::ENGAGE(nullptr, nullptr).Get<1>();
				UE_LOG(LogClass, Warning, TEXT("Tcl warning: an accessor could not retrieve a value by the name of %s in an object of the type %s"), *name, *clsName)
			}
			return result;
		}
	};
#pragma warning(default:4701) 
	template<typename T> struct GENERAL_MUTATOR {
		FORCEINLINE static void CONCRETE(UObject* self, FString name, T val) {
			auto success = false;
			FString clsName;
			TSubclassOf<UObject> cls = self == nullptr? nullptr : self->GetClass();
			if(cls != nullptr && cls->IsValidLowLevel() && !name.IsEmpty()) {
				for (TFieldIterator<UProperty> propIt(cls); propIt; ++propIt) {
					auto prop = *propIt;
					if(prop != nullptr && prop->GetNameCPP() == name) { success = SPECIALIZED_MUTATOR<T>::ENGAGE(self, prop, val); }
				}
			}
			if(!success) { UE_LOG(LogClass, Warning, TEXT("Tcl warning: a mutator could not adjust a value by the name of %s in an object of the type %s"), *name, *clsName) }
		}
	};

	template<typename P> struct GENERAL_CONVERTER {
		FORCEINLINE static Tcl_Obj* CONCRETE(TArray<P> arr) { return UTclComponent::convert(arr); }
	};

};