# TOptionalPtr

## Overview

TOptionalPtr is a wrapper class designed for Unreal Engine that takes an object pointer as it's constructor argument and proceeds to call member functions and fields on the object pointed to by the provided pointer. Additionally, TOptionalPtr can even call static functions where the wrapped object is the first argument. The main purpose of the wrapper is to reduce space in our functions taken by validation checks and make the functions more readable as the result. The best way to explain how it works is by an example. 

Imagine a static function in a utils class :

```
const FDatabaseEntry* UUtils::GetDataBaseEntry(const FString & entry_name, const UObject* object)
{
	UGameInstance* instance = UGameInstance::GetGameInstance(object);
	if (instance == nullptr)
		return nullptr;

	UGameDatabase* database = instance->GetGameDatabase();
	if (database == nullptr)
		return nullptr;

	const FDatabaseEntryHandler handler = UGameDatabase::CalculateHandler(entry_name);

	return database->GetEntry(handler);
}
```

And this is how it can be rewritten with TOptionalPtr :

```
const FDatabaseEntry* UUtils::GetDataBaseEntry(const FString & entry_name, const UObject* object)
{
	return TOptionalPtr<UGameInstance>(UGameInstance::GetGameInstance(object))
          	.Map(&UGameInstance::GetGameDatabase)
           	.Map(&UGameDatabase::GetEntry, UGameDatabase::CalculateHandler(entry_name))
            	.Get();
}
```

Another good example is this function:

```
APawn* UBlueprintLibrary::GetLocalPlayerPawn(const UObject* const world_context_object)
{
	const UWorld* world = world_context_object->GetWorld();
	if (IsValid(world))
	{
		const APlayerController* controller = world->GetFirstPlayerController();
		if (IsValid(controller))
		{
			return controller->GetPawn();
		}
	}

	return nullptr;
}
```

And after:

```
APawn* UBlueprintLibrary::GetLocalPlayerPawn(const UObject* const world_context_object)
{
	return TOptionalPtr<const UObject>(world_context_object)
			.Map(&UObject::GetWorld)
			.Map(&UWorld::GetFirstPlayerController<APlayerController>)
			.Map(&APlayerController::GetPawn<APawn>)
			.Get();
}
```

As you can see, the code doesn't contain repetitive validation checks, it doesn't require explicit types for intermediate objects and can deal with both UObjects and non-UObjects without any runtime overhead due to compile-time checks.

To ensure functionality and performance of the TOptionalPtr code a comprehensive automation spec (unit and performance tests) has been implemented and can be found in the file OptionalPtrSpec.cpp. TOptionalPtr is also documented using documentation comments inside the code describing each function of the class.

## Disadvantages

There are few disadvantages to the wrapper which are discussed in the following paragraphs to give a clear idea of when to use it.

### Run-time performance

The first disadvantage is the performance overhead of creating the TOptionalPtr objects over regular invalidation checks. For this reason, performance tests have been implemented and can be found in Unreal Editor's Test Automation under OptionalPtr. Performance. All the performance tests work with a simple function that creates and returns new object. The results are averaged out over 100 000 repetitions. Following are results for when a non-UObject is supplied:

| # of consecutive function calls | Regular | TOptionalPtr | Total Delta | Delta per # of consecutive calls |
| --- | --- | --- | --- | --- |
| 1 | 34ns | 38ns | 4ns | 4ns |
| 2 | 65ns | 69ns | 4ns | 2ns |
| 3 | 96ns | 100ns | 4ns | 1ns |
| 4 | 128ns | 134ns | 6ns | 1ns |

Following are results for when a UObject is supplied:

| # of consecutive function calls | Regular | TOptionalPtr | Total Delta | Delta per # of consecutive calls |
| --- | --- | --- | --- | --- |
| 1 | 41ns | 77ns | 36ns | 36ns |
| 2 | 78ns | 106ns | 28ns | 14ns |
| 3 | 117ns | 157ns | 40ns | 13ns |
| 4 | 155ns | 204ns | 49ns | 12ns |

There are few takeaways from these results. The first one is that delta between regular and TOptionalPtr flow is much smaller for Non-UObjects. The second takeaway is that with each consecutive call the run-time overhead gets smaller, meaning that the higher the number of consecutive calls the more suitable TOptionalPtr becomes. In conclusion, TOptionalPtr shouldn't probably be used in a performance-critical code such as happening on each tick and rather be used in once-per-lifecycle or event-triggered functions.

### No early exit
TOptionalPtr is meant mainly for happy paths, meaning paths expected to succeed in a predominant majority of the times, for example checking that player controller or player pawn is valid. TOptionalPtr is not meant for functions where early exit is expected to happen often because the flow will still have to go through all the Map functions to return the result. Some may argue that they want the ability to early exit even on happy path functions for performance sake to which the counter-argument is that if your function is failing a happy path flow you probably have much bigger problems than a slight increase in the execution time due to TOptionalPtr. 

### Overloaded functions
Lastly, when using overloaded function pointers they have to be explicitly cast to the intended type by static_cast. The Map function cannot detect the right function based solely on arguments provided.
