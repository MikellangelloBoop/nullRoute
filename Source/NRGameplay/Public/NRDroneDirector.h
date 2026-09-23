#pragma once
#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "MassEntityTypes.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassProcessor.h"
#include "NRDroneDirector.generated.h"
class UInstancedStaticMeshComponent;
class ANRCharacter;
UENUM()enum class ENRDroneState:uint8{Patrol,Search,Alert,Charging,Disabled,Destroyed};
USTRUCT()struct FNRDroneFragment:public FMassFragment
{
 GENERATED_BODY()
 FVector Position=FVector::ZeroVector,Target=FVector::ZeroVector,Home=FVector::ZeroVector,LastKnown=FVector::ZeroVector,AimPoint=FVector::ZeroVector;
 float Health=60,Speed=180,Yaw=0,Suspicion=0,LastSeen=-100,FireAt=0,NextAttack=0,DisabledUntil=0;
 bool bHasPath=false;uint8 Kind=0;ENRDroneState State=ENRDroneState::Patrol;
 TWeakObjectPtr<ANRCharacter> Tracked;
};
USTRUCT()struct FNRDroneView
{
 GENERATED_BODY()
 UPROPERTY()FVector_NetQuantize10 Position=FVector::ZeroVector;
 UPROPERTY()FVector_NetQuantize AimPoint=FVector::ZeroVector;
 UPROPERTY()uint8 Kind=0;
 UPROPERTY()ENRDroneState State=ENRDroneState::Patrol;
 UPROPERTY()uint8 Health=100;
 UPROPERTY()uint8 Suspicion=0;
 UPROPERTY()int16 Yaw=0;
 UPROPERTY()bool bAlive=true;
};
UCLASS()class NRGAMEPLAY_API UNRDroneProcessor:public UMassProcessor
{
 GENERATED_BODY()
public:UNRDroneProcessor();
protected:virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& Manager)override;virtual void Execute(FMassEntityManager& Manager,FMassExecutionContext& Context)override;
private:FMassEntityQuery Query;
};
UCLASS()class NRGAMEPLAY_API ANRDroneDirector:public AActor
{
 GENERATED_BODY()
public:
 ANRDroneDirector();virtual void Tick(float DeltaSeconds)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 void ResetWave();bool HitDrone(int32 Index,float Damage,ANRCharacter* Shooter=nullptr);
 void ReportNoise(FVector Position,float Radius,ANRCharacter* Source);
 void DisableSecurity(float Seconds);
 const TArray<FNRDroneView>& GetViews()const{return Views;}
 static float HearingRadius(const ANRCharacter* C);
 static const TCHAR* KindName(uint8 Kind);
 UPROPERTY(Replicated)uint8 AlertCount=0;
 UPROPERTY(Replicated)float BlackoutUntil=0;
 UPROPERTY(VisibleAnywhere)TObjectPtr<UInstancedStaticMeshComponent> Instances;
protected:virtual void BeginPlay()override;virtual void EndPlay(EEndPlayReason::Type Reason)override;
private:
 friend class UNRSmokeSubsystem;
 UPROPERTY()TArray<TObjectPtr<UInstancedStaticMeshComponent>> Bodies;
 UPROPERTY()TObjectPtr<UInstancedStaticMeshComponent> Rotors;
 UPROPERTY()TObjectPtr<UInstancedStaticMeshComponent> WarningLines;
 TArray<FVector> RenderPositions;TArray<ENRDroneState> PreviousStates;
 UPROPERTY(ReplicatedUsing=OnRep_Views)TArray<FNRDroneView> Views;
 UFUNCTION()void OnRep_Views();
 UFUNCTION(NetMulticast,Unreliable)void MulticastAttack(FVector_NetQuantize Origin,FVector_NetQuantize End);
 UFUNCTION(NetMulticast,Unreliable)void MulticastDestroyed(FVector_NetQuantize Position);
 void UpdateWave();void Sense(int32 Index,FNRDroneFragment& D,const TArray<ANRCharacter*>& Targets,float Now);
 void Publish(int32 Index,const FNRDroneFragment& D);
 bool CanSee(const FNRDroneFragment& D,const ANRCharacter* C)const;
 TArray<FMassEntityHandle> Entities;FMassArchetypeHandle Archetype;
 FTimerHandle WaveTimer;int32 QueryCursor=0;float NextSquadAttack=0;
};
