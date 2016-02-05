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
	template<typename ReturnType> struct SPECIALIZED_ACCESSOR {
		//kept empty to raise a compile time error if not used with a proper type
	};
	template<> struct SPECIALIZED_ACCESSOR<float> {
		FORCEINLINE static float ENGAGE(UObject* self, UProperty* prop) {  // no need to check for prop nullptr, already done in GENERAL_ACCESSOR
			auto result = 0.f;
			if(self != nullptr) {
				auto cast = Cast<UNumericProperty>(prop);
				if (cast != nullptr && cast->IsFloatingPoint()) {
					auto valPtr = prop->ContainerPtrToValuePtr<void>(self);
					result = cast->GetFloatingPointPropertyValue(valPtr);
				}
			}
			return result;
		}
	};
public:
	static TSubclassOf<UObject> FindClass(FString);
	static TArray<AActor*> AllActorsOf(UWorld*, TSubclassOf<AActor>);
	static UActorComponent* FindComponentOf(AActor*, TSubclassOf<UActorComponent>);
	static Tcl_Obj* Purge(AActor*);
	static int32 Eval(AActor*, FString, FString);

	template<typename T> struct ADD {
		FORCEINLINE static T CONCRETE(T First, T Second) { return First + Second; }
	};
	template<typename T, typename ...ParamTypes> struct MAKE {
		FORCEINLINE static T CONCRETE(ParamTypes... args) { return T(args...); }
	};

	template<typename ReturnType> struct GENERAL_ACCESSOR {
		FORCEINLINE static ReturnType CONCRETE(UObject* self, TSubclassOf<UObject> cls, FString name) {
			if(cls != nullptr) {
				for (TFieldIterator<UProperty> propIt(cls); propIt; ++propIt) {
					auto prop = *propIt;
					if(prop != nullptr) { UE_LOG(LogClass, Warning, TEXT("Reflections test %s"), *(prop->GetNameCPP())) }
					if(prop != nullptr && prop->GetNameCPP() == name) {
						UE_LOG(LogClass, Warning, TEXT("Reflections test2 %d"), self == nullptr)
						return SPECIALIZED_ACCESSOR<ReturnType>::ENGAGE(self, prop);
					}
				}
			}
			return SPECIALIZED_ACCESSOR<ReturnType>::ENGAGE(nullptr, nullptr);
		}
	};

};