#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRGenerateArtCommandlet.generated.h"
UCLASS()
class UNRGenerateArtCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UNRGenerateArtCommandlet();
    virtual int32 Main(const FString& Params) override;
};
