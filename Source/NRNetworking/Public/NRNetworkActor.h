#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NRAwakening.h"
#include "NRNetworkActor.generated.h"
UCLASS(Abstract)
class NRNETWORKING_API ANRNetworkActor:public AActor,public IAwakening {
 GENERATED_BODY()
public:
 ANRNetworkActor();
 virtual void WakeForMutation() override;
 virtual void FinishMutation() override;
protected:
 virtual void BeginPlay() override;
};
