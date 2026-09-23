#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRExpansionAssetsCommandlet.generated.h"
UCLASS()class UNRExpansionAssetsCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:UNRExpansionAssetsCommandlet();virtual int32 Main(const FString& Params)override;
};
