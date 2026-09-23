#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRMetaHumanArmorCommandlet.generated.h"
UCLASS()
class UNRMetaHumanArmorCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:
 UNRMetaHumanArmorCommandlet();
 virtual int32 Main(const FString& Params)override;
};
