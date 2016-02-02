namespace Library {
	TSubclassOf<UObject> FindClass(FString Name) { return FindObjectSafe<UClass>(ANY_PACKAGE, *Name); }
}