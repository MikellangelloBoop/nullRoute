#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRModuleAssetsCommandlet.generated.h"
UCLASS() class UNRModuleAssetsCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:UNRModuleAssetsCommandlet();virtual int32 Main(const FString& Params)override;
};
