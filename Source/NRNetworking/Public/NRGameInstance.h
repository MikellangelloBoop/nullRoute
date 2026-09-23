#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NRGameInstance.generated.h"

UCLASS() class NRNETWORKING_API UNRGameInstance : public UGameInstance
{
 GENERATED_BODY()
public:
 virtual void Init() override;
};
