#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NRPreparationZone.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class ANRCharacter;

// Six targets and two supply benches. No actor tick; only hit flash changes replicate.
UCLASS() class NRGAMEPLAY_API ANRPreparationProp : public AActor
{
 GENERATED_BODY()
public:
 ANRPreparationProp();
 virtual void BeginPlay() override;
 virtual void EndPlay(EEndPlayReason::Type Reason) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
 void SetAvailable(bool Value);
 bool Interact(ANRCharacter* Player);
 FString Prompt() const;
 UPROPERTY(ReplicatedUsing=OnRep_State) bool bStation=false;
 UPROPERTY(Replicated) bool bAttackSide=false;
 UPROPERTY(ReplicatedUsing=OnRep_State) bool bAvailable=true;
 UPROPERTY(ReplicatedUsing=OnRep_State) bool bHit=false;
 UPROPERTY() TObjectPtr<UBoxComponent> Collision;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
private:
 bool ValidOperator(const ANRCharacter* Player) const;
 UFUNCTION() void OnRep_State();
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TargetDetails;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> TargetMarker;
 FTimerHandle FlashTimer;
 float LastHit=-100;
};

// One always-relevant actor carries the barrier state, including for late joiners.
// Decorative components are local; the dedicated server creates only collision and props.
UCLASS() class NRGAMEPLAY_API ANRPreparationZone : public AActor
{
 GENERATED_BODY()
public:
 ANRPreparationZone();
 virtual void BeginPlay() override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 void SetPreparing(bool Preparing);
 UPROPERTY(ReplicatedUsing=OnRep_Closed) bool bClosed=true;
 UPROPERTY() TObjectPtr<UBoxComponent> Barrier;
private:
 UFUNCTION() void OnRep_Closed();
 void CreateLocalVisuals();
 UPROPERTY() TArray<TObjectPtr<UPrimitiveComponent>> GateVisuals;
 UPROPERTY() TArray<TObjectPtr<ANRPreparationProp>> Props;
};
