#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRTypes.h"
#include "NRGraphSubsystem.generated.h"
class ANodeBase;class ANRCharacter;
UCLASS() class NRGAMEPLAY_API UNRGraphSubsystem:public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 void RegisterNode(ANodeBase* Node);void RemoveNode(ANodeBase* Node);
 bool Connect(ANodeBase* From,ANodeBase* To,uint8 Team);
 void Cascade(ANodeBase* Origin);
 ANodeBase* Find(FGuid Id)const;
 virtual void Tick(float DeltaTime)override;
 virtual TStatId GetStatId()const override{RETURN_QUICK_DECLARE_CYCLE_STAT(NRGraph,STATGROUP_Tickables);}
 virtual bool ShouldCreateSubsystem(UObject* Outer)const override;
 const TArray<TWeakObjectPtr<ANodeBase>>& GetNodes()const{return Nodes;}
 uint32 GetRevision()const{return Revision;}
private:
 TArray<TWeakObjectPtr<ANodeBase>> Nodes;float Accum=0;uint32 Revision=0;
 TMap<FGuid,double> DelayStarted;
};
