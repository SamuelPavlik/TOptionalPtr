# TOptionalPtr

## Overview

TOptionalPtr is a wrapper class that takes an object pointer as it's constructor argument and proceeds to call member functions and fields on the object pointed to by the provided pointer. Additionally, TOptionalPtr can even call static functions where the wrapped object is the first argument. The main purpose of the wrapper is to reduce space in our functions taken by validation checks and make the functions more readable as the result. The best way to explain how it works is by an example. 

This is a function from the class UCAHUDUtils :

```
const FDatabaseEntry* UCAHUDUtils::GetDataBaseEntry(const FString & entry_name, const UObject* object)
{
	UCAGameInstance* instance = UCAGameInstance::GetCAGameInstance(object);
	if (instance == nullptr)
		return nullptr;

	UCAGameDatabase* database = instance->GetGameDatabase();
	if (database == nullptr)
		return nullptr;

	const FDatabaseEntryHandler handler = UCAGameDatabase::CalculateHandler(entry_name);

	return database->GetEntry(handler);
}
```

And this is how it can be rewritten with TOptionalPtr :

```
const FDatabaseEntry* UCAHUDUtils::GetDataBaseEntry(const FString & entry_name, const UObject* object)
{
	return TOptionalPtr<UCAGameInstance>(UCAGameInstance::GetCAGameInstance(object))
          	.Map(&UCAGameInstance::GetGameDatabase)
           	.Map(&UCAGameDatabase::GetEntry, UCAGameDatabase::CalculateHandler(entry_name))
            .Get();
}
```

Another good example is this function:

```
int32 UCAHUDUtils::GetLocalPlayerId(const UObject* world_context)
{
    if (!IsValid(world_context))
    {
        return -1;
    }
 
    const ACACharacter_MainPlayer * const owning_player = UCABlueprintLibrary::GetLocalPlayerCharacter(world_context);
    if (IsValid(owning_player) && owning_player->GetPlayerState())
    {
        return owning_player->GetPlayerState()->GetPlayerId();
    }
 
    return -1;
}
```

And after:

```
int32 UCAHUDUtils::GetLocalPlayerId(const UObject* world_context)
{
    return TOptionalPtr<const UObject>(world_context)
            .MapStatic(&UCABlueprintLibrary::GetLocalPlayerCharacter)
            .Map(&ACACharacter_MainPlayer::GetPlayerState<APlayerState>)
            .MapToValue(-1, &APlayerState::GetPlayerId);
}
```

As you can see, the code doesn't contain repetitive validation checks, it doesn't require explicit types for intermediate objects and can deal with both UObjects and non-UObjects without any runtime overhead due to compile-time checks.

To ensure functionality and performance of the TOptionalPtr code a comprehensive automation spec (unit and performance tests) has been implemented and can be found in the file Source/keaton/Helpers/OptionalPtrSpec.cpp. TOptionalPtr is also documented using documentation comments inside the code describing each function of the class.

## Disadvantages

There are few disadvantages to the wrapper which are discussed in the following paragraphs to give a clear idea of when to use it.

### Run-time performance

The first disadvantage is the performance overhead of creating the TOptional objects over regular invalidation checks. For this reason, performance tests have been implemented and can be found in Unreal Editor's Test Automation under OptionalPtr.Performance. All the performance tests work with a simple function that creates and returns new object. The results are averaged out over 100 000 repetitions. Following are results for when a non-UObject is supplied:


