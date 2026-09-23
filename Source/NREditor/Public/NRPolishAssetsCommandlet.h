#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRPolishAssetsCommandlet.generated.h"
UCLASS()
class UNRPolishAssetsCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UNRPolishAssetsCommandlet();
 virtual int32 Main(const FString& Params) override;
};
