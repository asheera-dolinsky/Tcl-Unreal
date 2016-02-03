namespace Library {
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