#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRDroneDirector.h"
#include "NRLocomotion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UNRSmokeSubsystem::RunEvolutionNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)
 {
  auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();
  if(GM&&GM->GetNumPlayers()>=2&&Stage==0)
  {
   ++Stage;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);
   for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)GetWorld()->GetTimerManager().ClearTimer(It->WaveTimer);
   bool Clean=true;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It){const bool OK=!It->ViewArms&&!It->WorldWeapon&&!It->ViewFill&&!It->IsActorTickEnabled()&&!It->GetMesh()->GetAnimInstance();Clean&=OK;
   UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_NET server pawn cosmetics=%d tick=%d anim=%d"),bool(It->ViewArms||It->WorldWeapon||It->ViewFill),It->IsActorTickEnabled(),bool(It->GetMesh()->GetAnimInstance()));}
   UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_NET server has no cosmetic graph: %s"),Clean?TEXT("PASS"):TEXT("FAIL"));
  }
  return;
 }
 const bool Driver=FParse::Param(FCommandLine::Get(),TEXT("NRPoseDriver"));
 // Completion must survive the observer leaving and the server returning to its lobby.
 if(Driver&&Stage>0&&T-OperationsEpoch>24){UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_NET_DRIVER_COMPLETE"));FPlatformMisc::RequestExit(false);return;}
 if(!C||!PC)return;const auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS||GS->Phase!=ENRPhase::Preparation)return;
 if(Stage==0){Stage=1;OperationsEpoch=T;}
 const float E=T-OperationsEpoch;
 if(Driver)
 {
  if(Stage==1&&E>2){++Stage;C->ChoosePrimary(ENRPrimaryWeapon::Forge12);}
  if(Stage==2&&E>4){++Stage;C->FirePressed();C->FireReleased();}
  if(Stage==3&&E>6){++Stage;C->ReloadPressed();}
  if(Stage==4&&E>10){++Stage;C->SelectWeapon(1);}
  if(Stage==5&&E>12){++Stage;C->AimPressed();C->LeanRightPressed();PC->SetControlRotation(FRotator(35,PC->GetControlRotation().Yaw,0));}
  if(E>13&&E<14)C->Right(.4);
  if(Stage==6&&E>15){++Stage;C->CrouchPressed();}
  if(Stage==7&&E>18){++Stage;C->AimReleased();C->LeanRightReleased();C->CrouchReleased();C->SelectWeapon(2);}
  if(Stage==8&&E>20){++Stage;C->FirePressed();C->FireReleased();}
  if(E>24){UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_NET_DRIVER_COMPLETE"));FPlatformMisc::RequestExit(false);}
 }
 else
 {
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(*It!=C&&It->GetLocalRole()==ROLE_SimulatedProxy)
  {
   if(It->PrimaryWeapon==ENRPrimaryWeapon::Forge12)NetworkStage|=1;
   if(It->bReloading)NetworkStage|=2;if(It->EquippedWeapon==1)NetworkStage|=4;if(It->EquippedWeapon==2)NetworkStage|=8;
   if(It->ReplicatedLean>50)NetworkStage|=16;if(It->bIsCrouched)NetworkStage|=32;
   if(It->LastPresentedShot>0)NetworkStage|=64;if(It->LastPresentedMelee>0)NetworkStage|=128;
   if(Cast<UNRLocomotion>(It->GetMesh()->GetAnimInstance())&&It->WorldWeapon&&It->WorldWeapon->GetAttachSocketName()==TEXT("HandGrip_R"))NetworkStage|=256;
   if(It->GetBaseAimRotation().Pitch>20&&It->GetBaseAimRotation().Pitch<50)NetworkStage|=512;
   if(It->GetVelocity().Size2D()>20)NetworkStage|=1024;
  }
  if(E>23)
  {
   const TCHAR* Names[]={TEXT("primary selection"),TEXT("reload"),TEXT("pistol"),TEXT("knife"),TEXT("lean"),TEXT("crouch"),TEXT("shot cue"),TEXT("melee cue"),TEXT("native pose and weapon attachment"),TEXT("aim pitch"),TEXT("locomotion velocity")};
   for(int I=0;I<11;++I)UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_NET remote %s : %s"),Names[I],NetworkStage&(1<<I)?TEXT("PASS"):TEXT("FAIL"));
   UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_NET_OBSERVER %s mask=%d"),NetworkStage==2047?TEXT("PASSED"):TEXT("FAILED"),NetworkStage);
   FPlatformMisc::RequestExitWithStatus(false,NetworkStage==2047?0:1);
  }
 }
}
