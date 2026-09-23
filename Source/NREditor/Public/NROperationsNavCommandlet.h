#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NROperationsNavCommandlet.generated.h"
UCLASS()class UNROperationsNavCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:UNROperationsNavCommandlet();virtual int32 Main(const FString& Params)override;
};
