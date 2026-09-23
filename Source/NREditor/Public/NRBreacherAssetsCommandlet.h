#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NRBreacherAssetsCommandlet.generated.h"
UCLASS() class UNRBreacherAssetsCommandlet:public UCommandlet
{
 GENERATED_BODY()
public:
 UNRBreacherAssetsCommandlet();
 virtual int32 Main(const FString& Params)override;
};
