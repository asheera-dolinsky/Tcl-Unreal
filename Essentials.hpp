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

namespace TclUnrealEssentials {
	TSubclassOf<UObject> FindClass(FString Name) { return FindObjectSafe<UClass>(ANY_PACKAGE, *Name); }
	TArray<AActor*> AllActorsOf(UWorld* World, TSubclassOf<AActor> Cls) {
		TArray<AActor*> actors;
		if (!(World == nullptr || Cls == nullptr)) { UGameplayStatics::GetAllActorsOfClass(World, Cls, actors); }
		return actors;

	}
	Tcl_Obj* Purge(AActor* Actor) {
		Tcl_Obj* obj = nullptr;
		UTclComponent* comp = nullptr;
		if(Actor != nullptr) { comp = Actor->FindComponentByClass<UTclComponent>(); } else { return obj; }
		if(comp != nullptr) { obj = comp->Purge(); }
		return obj;

	}
	int32 Eval(AActor* Actor, FString Filename, FString Code) {
		UTclComponent* comp = nullptr;
		if(Actor != nullptr) { comp = Actor->FindComponentByClass<UTclComponent>(); } else { return TCL_ERROR; }
		return (comp == nullptr)? TCL_ERROR : comp->Eval(Filename, Code);

	}
	template<typename T> struct ADD {
		FORCEINLINE static T CONCRETE(T First, T Second) { return First + Second; }
	};
	template<typename T, typename ...ParamTypes> struct MAKE {
		FORCEINLINE static T CONCRETE(ParamTypes... args) { return T(args...); }
	};

}