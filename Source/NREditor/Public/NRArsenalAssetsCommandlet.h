#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRArsenalAssetsCommandlet.generated.h"
UCLASS() class UNRArsenalAssetsCommandlet : public UCommandlet
{
 GENERATED_BODY()
public:UNRArsenalAssetsCommandlet();virtual int32 Main(const FString& Params)override;
};
