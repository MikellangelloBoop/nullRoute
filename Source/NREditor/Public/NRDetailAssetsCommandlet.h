#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRDetailAssetsCommandlet.generated.h"
UCLASS() class UNRDetailAssetsCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UNRDetailAssetsCommandlet();
 virtual int32 Main(const FString& Params)override;
};
