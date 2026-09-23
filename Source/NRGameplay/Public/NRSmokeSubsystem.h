#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRSmokeSubsystem.generated.h"
UCLASS()class NRGAMEPLAY_API UNRSmokeSubsystem:public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 virtual bool ShouldCreateSubsystem(UObject* O)const override;
 virtual void Tick(float Delta)override;
 virtual TStatId GetStatId()const override{RETURN_QUICK_DECLARE_CYCLE_STAT(NRSmoke,STATGROUP_Tickables);}
private:
 void RunEvolutionNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunEvolution(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunRosterTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunRosterVisual(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunRosterNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunServerSystemTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunServerJoinTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunServerVisual(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunPreparationTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunPreparationNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunPreparationVisual(class ANRCharacter* C,class APlayerController* PC,float T);
 float PreparationEpoch=0;
 void RunBreacherTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunBreacherVisual(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunBreacherNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 float OperationsEpoch=0;
 void RunOperationsTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunOperationsNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunOperationsVisual(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunStanceTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunStanceNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunStanceVisual(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunDamageAudioTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunModuleTest(class ANRCharacter* C,class APlayerController* PC,float T);
 void RunModuleNetwork(class ANRCharacter* C,class APlayerController* PC,float T);
 int32 NetworkStage=0;int32 Stage=0;bool bFailed=false;TWeakObjectPtr<class ANodeBase> Printed;
};
