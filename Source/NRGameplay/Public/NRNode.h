#pragma once
#include "CoreMinimal.h"
#include "NRNetworkActor.h"
#include "NRTypes.h"
#include "NRNode.generated.h"
class UAbilitySystemComponent;class UStaticMeshComponent;class UNavigationInvokerComponent;class ANRCharacter;
UCLASS() class NRGAMEPLAY_API ANodeBase:public ANRNetworkActor {
 GENERATED_BODY()
public:
 ANodeBase();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 void Initialize(uint8 InTeam,ENRNodeOp InOperation,bool bBlack=false);
 void StartPrinting();void CompletePrinting();void SetOutput(bool Value);
 void NetExecute();bool Capture(ANRCharacter* Scanner);
 virtual float TakeDamage(float Amount,FDamageEvent const& Event,AController* Instigator,AActor* Causer)override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UNavigationInvokerComponent> Invoker;
 UPROPERTY(Replicated) FGuid NodeID;
 UPROPERTY(Replicated) uint8 Team=255;
 UPROPERTY(ReplicatedUsing=OnRep_State) ENRNodeState NodeState=ENRNodeState::Dormant;
 UPROPERTY(ReplicatedUsing=OnRep_State) ENRNodeOp Operation=ENRNodeOp::Relay;
 UPROPERTY(Replicated) TArray<FNRNodeLink> OutputPins;
 UPROPERTY(Replicated) uint32 Revision=0;
 UPROPERTY(Replicated) bool bOutput=false;
 UPROPERTY(Replicated) float PhaseEnd=0;
 UPROPERTY(Replicated) ENRPrintPhase PrintPhase=ENRPrintPhase::None;
 UPROPERTY(EditAnywhere,Category="Node") float TriggerDelay=1.f;
 bool IsBlackNode()const{return bBlackNode;}
 float GetHealth()const{return Health;}
protected:
 virtual void BeginPlay()override;virtual void EndPlay(EEndPlayReason::Type Reason)override;
private:
 UFUNCTION()void OnRep_State();
 void ChangeFilament();void Reinforce();
 UPROPERTY()TObjectPtr<UAbilitySystemComponent> PrivateStateASC;
 bool bBlackNode=false;float Health=100;float LastAttack=0;
 FTimerHandle PrintTimer;
};
