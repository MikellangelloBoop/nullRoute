#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NRAwakening.generated.h"
UINTERFACE(MinimalAPI,meta=(CannotImplementInterfaceInBlueprint))
class UAwakening:public UInterface { GENERATED_BODY() };
class NRCORE_API IAwakening {
 GENERATED_BODY()
public:
 virtual void WakeForMutation()=0;
 virtual void FinishMutation()=0;
};
