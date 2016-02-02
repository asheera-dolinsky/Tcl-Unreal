namespace Library {
	TSubclassOf<UObject> FindClass(FString Name) { return FindObjectSafe<UClass>(ANY_PACKAGE, *Name); }
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

}