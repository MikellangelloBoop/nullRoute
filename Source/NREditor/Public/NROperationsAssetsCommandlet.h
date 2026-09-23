#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NROperationsAssetsCommandlet.generated.h"
UCLASS()class UNROperationsAssetsCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:UNROperationsAssetsCommandlet();virtual int32 Main(const FString& Params)override;
};
