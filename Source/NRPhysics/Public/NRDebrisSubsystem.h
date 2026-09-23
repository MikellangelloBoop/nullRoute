#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRDebrisSubsystem.generated.h"
class UStaticMeshComponent;
USTRUCT() struct FNRLocalDebris {
 GENERATED_BODY()
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
 float Quiet=0;float Age=0;
};
UCLASS() class NRPHYSICS_API UNRDebrisSubsystem:public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 virtual bool ShouldCreateSubsystem(UObject* Outer)const override;
 virtual void Tick(float DeltaTime)override;
 virtual TStatId GetStatId()const override {RETURN_QUICK_DECLARE_CYCLE_STAT(NRDebris,STATGROUP_Tickables);}
 void SpawnBurst(FVector Position,uint32 Seed,FVector Direction=FVector::UpVector);
 int32 ActivePieces()const{return Pieces.Num();}
private:
 UPROPERTY() TObjectPtr<class UPhysicalMaterial> DebrisMaterial;
 UPROPERTY() TArray<FNRLocalDebris> Pieces;
 UPROPERTY() TObjectPtr<AActor> PoolOwner;
};
