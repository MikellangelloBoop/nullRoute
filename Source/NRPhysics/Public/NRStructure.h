#pragma once
#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "NRAwakening.h"
#include "NRStructure.generated.h"
class ANavLinkProxy;
UCLASS()
class NRPHYSICS_API ANRStructure:public AGeometryCollectionActor,public IAwakening {
 GENERATED_BODY()
public:
 ANRStructure(const FObjectInitializer& Init);
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 virtual void WakeForMutation() override;
 virtual void FinishMutation() override;
 void ResetForRound();
 bool BreachServer(FVector Origin,FVector Direction,float Strain=500000.f);
 virtual float TakeDamage(float Amount,FDamageEvent const& Event,AController* EventInstigator,AActor* Causer) override;
 UPROPERTY(EditAnywhere,Category="Structure") bool bFloor=false;
 UPROPERTY(ReplicatedUsing=OnRep_Collapsed) bool bCollapsed=false;
 UPROPERTY(EditAnywhere,Category="Structure") float Integrity=180.f;
protected:
 virtual void BeginPlay() override;
private:
 UFUNCTION(NetMulticast,Unreliable)void MulticastFragments(FVector_NetQuantize Origin,FVector_NetQuantizeNormal Direction,uint32 Seed);
 UFUNCTION() void OnRep_Collapsed();
 void UpdateNavigation();
 FTimerHandle SettleTimer;
};

