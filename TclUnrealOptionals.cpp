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
#include "TclUnrealOptionals.h"


UTclUnrealOptionals::UTclUnrealOptionals(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

Tcl_Obj* UTclUnrealOptionals::GetCursorHit(APlayerController* PlayerController, TArray<TEnumAsByte<EObjectTypeQuery>> Types, bool TraceComplex) {
	FHitResult out;
	auto impactPoint = FVector(0.f);
	auto hitSuccess = PlayerController != nullptr && PlayerController->GetHitResultUnderCursorForObjects(Types, TraceComplex, out);
	if (hitSuccess) { impactPoint = out.ImpactPoint; }
	return UTclComponent::pack<bool, FVector>(hitSuccess, impactPoint);

}

FVector UTclUnrealOptionals::RotationToVector(FRotator Rot) { return Rot.Vector(); }

void UTclUnrealOptionals::SetActorLocation(AActor* Actor, FVector Location) { if(Actor != nullptr) { Actor->SetActorLocation(Location); } }

void UTclUnrealOptionals::SetCollisionProfileByName(UPrimitiveComponent* Component, FName Name) { if(Component != nullptr) { Component->SetCollisionProfileName(Name); } }