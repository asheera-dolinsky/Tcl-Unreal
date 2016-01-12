#pragma once

#include "BPInterfaceLibrary.generated.h"


UCLASS()
class UBPInterfaceLibrary : public UBlueprintFunctionLibrary
{
  GENERATED_UCLASS_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "Tcl")
  static FString LoadStringFromFile(FString Filename);

};