#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRGraphVisuals.generated.h"
class UInstancedStaticMeshComponent;class UPostProcessComponent;
UCLASS()class NRGAMEPLAY_API UNRGraphVisuals:public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 virtual bool ShouldCreateSubsystem(UObject* Outer)const override;
 virtual void Tick(float Delta)override;
 virtual TStatId GetStatId()const override{RETURN_QUICK_DECLARE_CYCLE_STAT(NRGraphVisuals,STATGROUP_Tickables);}
private:
 UPROPERTY()TObjectPtr<AActor> Owner;
 UPROPERTY()TObjectPtr<UInstancedStaticMeshComponent> Lines;
 UPROPERTY()TObjectPtr<UPostProcessComponent> Visor;
 float Accum=0;uint32 LastHash=0;
};
