#pragma once
#include "CoreMinimal.h"
#include "NRNetworkActor.h"
#include "NRActivity.generated.h"
class ANRCharacter;class UBoxComponent;class UStaticMeshComponent;
UENUM()enum class ENRActivityKind:uint8{Supply,Relay,Target};
UCLASS()class NRGAMEPLAY_API ANRActivity:public ANRNetworkActor
{
 GENERATED_BODY()
public:
 ANRActivity();
 virtual void BeginPlay()override;
 virtual void EndPlay(EEndPlayReason::Type Reason)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* DamageInstigator,AActor* Causer)override;
 void Interact(ANRCharacter* Player);void ResetActivity();
 FString Prompt()const;
 UPROPERTY(EditAnywhere,ReplicatedUsing=OnRep_State)ENRActivityKind Kind=ENRActivityKind::Supply;
 UPROPERTY(ReplicatedUsing=OnRep_State)bool bCompleted=false;
 UPROPERTY(Replicated)uint8 Progress=0;
 UPROPERTY(VisibleAnywhere)TObjectPtr<UBoxComponent> Collision;
 UPROPERTY(VisibleAnywhere)TObjectPtr<UStaticMeshComponent> Visual;
private:
 UFUNCTION()void OnRep_State();
 void UpdateChannel();bool ValidOperator(ANRCharacter* Player)const;
 TWeakObjectPtr<ANRCharacter> Operator;
 FTimerHandle ChannelTimer;
};
