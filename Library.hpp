namespace Library {
	TSubclassOf<UObject> FindClass(FString Name) { return FindObjectSafe<UClass>(ANY_PACKAGE, *Name); }
	Tcl_Obj* Purge(AActor* Actor) {
		Tcl_Obj* obj = nullptr;
		UTclComponent* comp = nullptr;
		if(Actor != nullptr) { comp = Actor->FindComponentByClass<UTclComponent>(); } else { return obj; }
		if(comp != nullptr) { obj = comp->purge(); }
		return obj;
	}

}