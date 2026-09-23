#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRGenerateArenaCommandlet.generated.h"
UCLASS()class UNRGenerateArenaCommandlet:public UCommandlet {
 GENERATED_BODY()
public:UNRGenerateArenaCommandlet();virtual int32 Main(const FString& Params)override;
};
