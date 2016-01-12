#include "PhantomGunsDemo.h"
#include "BPInterfaceLibrary.h"


UBPInterfaceLibrary::UBPInterfaceLibrary(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

FString UBPInterfaceLibrary::LoadStringFromFile(FString Filename) {
	FString result;
	auto fname = FPaths::GameContentDir() + "/Scripts/" + Filename;
	if(FPaths::FileExists(fname)) {
		FFileHelper::LoadFileToString(result, *fname);
	} else {
		UE_LOG(LogClass, Error, TEXT("File at path: %s doesn't exist!"), *fname)
	}
	return FString(result);

}