#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRMetaHumanAssetsCommandlet.generated.h"

UCLASS()
class UNRMetaHumanAssetsCommandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 UNRMetaHumanAssetsCommandlet();
 virtual int32 Main(const FString& Params) override;
};
