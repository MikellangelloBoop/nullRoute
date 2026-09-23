#pragma once
#include "CoreMinimal.h"
#include "NRNetworkActor.h"
#include "NRTacticalDoor.generated.h"
class UBoxComponent;class UStaticMeshComponent;class ANRCharacter;
UCLASS()
class NRGAMEPLAY_API ANRTacticalDoor : public ANRNetworkActor
{
 GENERATED_BODY()
public:
 ANRTacticalDoor();
 virtual void BeginPlay() override;
 virtual void EndPlay(EEndPlayReason::Type Reason) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 bool Interact(ANRCharacter* Player);
 void ResetDoor();
 FString Prompt()const;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> ConsoleCollision;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> GateCollision;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ConsoleMesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Panel;
 UPROPERTY(EditAnywhere) FVector GateOffset=FVector(0,230,60);
 UPROPERTY(EditAnywhere) FVector GateExtent=FVector(18,160,150);
 UPROPERTY(ReplicatedUsing=OnRep_Door) bool bOpen=true;
 UPROPERTY(Replicated) float ReadyAt=0;
private:
 UFUNCTION()void OnRep_Door();
 void AnimateDoor();bool Occupied()const;
 float OpenAmount=1;FTimerHandle MotionTimer;
};
