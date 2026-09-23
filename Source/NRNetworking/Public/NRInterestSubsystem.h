#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRInterestSubsystem.generated.h"
UCLASS()class NRNETWORKING_API UNRInterestSubsystem:public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 void RegisterActor(AActor* Actor);
 virtual bool ShouldCreateSubsystem(UObject* Outer)const override;
 virtual void Tick(float Dt)override;
 virtual TStatId GetStatId()const override{RETURN_QUICK_DECLARE_CYCLE_STAT(NRInterest,STATGROUP_Tickables);}
private:TArray<TWeakObjectPtr<AActor>> Actors;int32 Cursor=0;float Accum=0;
};
