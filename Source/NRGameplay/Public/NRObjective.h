#pragma once
#include "CoreMinimal.h"
#include "NRNetworkActor.h"
#include "NRObjective.generated.h"
class UStaticMeshComponent;class ANRCharacter;
UCLASS()class NRGAMEPLAY_API ANRObjective:public ANRNetworkActor {
 GENERATED_BODY()
public:
 ANRObjective();virtual void BeginPlay()override;void DropAt(FVector Position);void Interact(ANRCharacter* Player);void ResetObjective();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 UPROPERTY(EditAnywhere)bool bExtraction=false;
 UPROPERTY(ReplicatedUsing=OnRep_Taken)bool bTaken=false;
 UPROPERTY(VisibleAnywhere)TObjectPtr<UStaticMeshComponent> Mesh;
private:FVector HomePosition=FVector::ZeroVector;UFUNCTION()void OnRep_Taken();
};
