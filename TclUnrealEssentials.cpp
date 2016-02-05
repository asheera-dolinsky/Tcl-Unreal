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
#include "TclUnrealEssentials.h"


UTclUnrealEssentials::UTclUnrealEssentials(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

TSubclassOf<UObject> UTclUnrealEssentials::FindClass(FString Name) { return FindObjectSafe<UClass>(ANY_PACKAGE, *Name); }
TArray<AActor*> UTclUnrealEssentials::AllActorsOf(UWorld* World, TSubclassOf<AActor> Cls) {
	TArray<AActor*> actors;
	if (!(World == nullptr || Cls == nullptr)) { UGameplayStatics::GetAllActorsOfClass(World, Cls, actors); }
	return actors;

}
UActorComponent* UTclUnrealEssentials::FindComponentOf(AActor* Actor, TSubclassOf<UActorComponent> Cls) {
	return Actor == nullptr? nullptr : Actor->FindComponentByClass(Cls);
}
Tcl_Obj* UTclUnrealEssentials::Purge(AActor* Actor) {
	Tcl_Obj* obj = nullptr;
	UTclComponent* comp = nullptr;
	if(Actor != nullptr) { comp = Actor->FindComponentByClass<UTclComponent>(); } else { return obj; }
	if(comp != nullptr) { obj = comp->Purge(); }
	return obj;

}
int32 UTclUnrealEssentials::Eval(AActor* Actor, FString Filename, FString Code) {
	UTclComponent* comp = nullptr;
	if(Actor != nullptr) { comp = Actor->FindComponentByClass<UTclComponent>(); } else { return TCL_ERROR; }
	return (comp == nullptr)? TCL_ERROR : comp->Eval(Filename, Code);

}