#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRPrepareFirstPersonCommandlet.generated.h"
UCLASS() class UNRPrepareFirstPersonCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:
 UNRPrepareFirstPersonCommandlet();
 virtual int32 Main(const FString& Params)override;
};
