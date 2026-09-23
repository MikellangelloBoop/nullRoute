#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRBallisticsSubsystem.generated.h"
class ANRCharacter;
struct FNRBullet {FVector Position,Velocity;float Damage=30;float Life=0;int32 Penetrations=0;int32 Bounces=0;bool bPractice=false;TWeakObjectPtr<ANRCharacter> Shooter;TWeakObjectPtr<AActor> LastSurface;};
UCLASS()class NRGAMEPLAY_API UNRBallisticsSubsystem:public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 void ClearProjectiles(){Bullets.Reset();}
 void Fire(ANRCharacter* Shooter,FVector Origin,FVector Direction,float Damage,float Speed);
 virtual void Tick(float Dt)override;
 virtual TStatId GetStatId()const override{RETURN_QUICK_DECLARE_CYCLE_STAT(NRBallistics,STATGROUP_Tickables);}
 virtual bool ShouldCreateSubsystem(UObject* Outer)const override;
private:friend class UNRSmokeSubsystem;TArray<FNRBullet> Bullets;
};
