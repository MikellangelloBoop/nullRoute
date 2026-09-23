#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRCharacterMovement.h"
#include "NRLocomotion.h"
#include "NRGameMode.h"
#include "NRHUD.h"
#include "NRDroneDirector.h"
#include "NRTacticalDoor.h"
#include "NRActivity.h"
#include "NRBallisticsSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UnrealClient.h"

void UNRSmokeSubsystem::RunEvolution(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC||T<4)return;
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 const bool Visual=FParse::Param(FCommandLine::Get(),TEXT("NREvolutionVisual"));
 if(Stage==0)
 {
  ++Stage;OperationsEpoch=T;
  if(auto* H=Cast<ANRHUD>(PC->GetHUD()))if(H->IsMenuOpen())H->ToggleMenu();
  auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);GM->BeginCombat();GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);
  for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It){It->DisableSecurity(300);GetWorld()->GetTimerManager().ClearTimer(It->WaveTimer);}
  Check(Cast<UNRLocomotion>(C->GetMesh()->GetAnimInstance())!=nullptr,TEXT("native world animation graph installed"));
  Check(C->ViewArms->GetSingleNodeInstance()!=nullptr,TEXT("first-person animation retained"));
  auto* Anim=Cast<UNRLocomotion>(C->GetMesh()->GetAnimInstance());bool Clips=Anim&&Anim->Clips.Num()==8;
  if(Anim)for(auto& Clip:Anim->Clips)Clips&=Clip!=nullptr;Check(Clips,TEXT("all locomotion and action assets available"));
  Check(C->WorldWeapon->GetAttachParent()==C->GetMesh()&&C->WorldWeapon->GetAttachSocketName()==TEXT("HandGrip_R"),TEXT("remote weapon attached to animated hand"));
  auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Preparation;
  int Total=C->Ammo+C->ReserveAmmo;
  for(int Kind=0;Kind<=5;++Kind)
  {
   C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon(Kind));const auto Stats=C->WeaponStats(0);
   Check(C->PrimaryWeapon==ENRPrimaryWeapon(Kind)&&C->Ammo+C->ReserveAmmo<=Total&&C->ReserveAmmo<=Stats.Reserve,TEXT("primary selection preserves finite ammo without creating surplus"));
   Total=C->Ammo+C->ReserveAmmo;
   Check(C->Weapon->GetStaticMesh()!=nullptr&&Stats.Capacity>0&&Stats.Reserve>0&&Stats.Interval>0,TEXT("weapon model and finite combat specification"));
  }
  C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon::Forge12);
  Check(C->WeaponStats(0).Pellets==8&&C->WeaponStats(0).Capacity==8&&!C->WeaponStats(0).bAutomatic,TEXT("shotgun has eight pellets and semi-auto shell fire"));
  GS->Phase=ENRPhase::Combat;C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon::Lancer60);
  auto* Ballistics=GetWorld()->GetSubsystem<UNRBallisticsSubsystem>();Ballistics->ClearProjectiles();
  const FVector SavedAt=C->GetActorLocation();C->SetActorLocation({0,0,5000});C->LastShot=-100;C->WeaponReadyAt=0;C->bAiming=true;const int BeforeShell=C->Ammo;C->FireShot();
  Check(Ballistics->Bullets.Num()==8&&C->Ammo==BeforeShell-1,TEXT("shotgun creates eight server projectiles per one ammo unit"));
  C->FireShot();Check(Ballistics->Bullets.Num()==8&&C->Ammo==BeforeShell-1,TEXT("shotgun fire cadence enforced"));Ballistics->ClearProjectiles();C->SetActorLocation(SavedAt);C->bAiming=false;
  Check(C->PrimaryWeapon==ENRPrimaryWeapon::Forge12,TEXT("combat forbids arsenal changes"));
  GS->Phase=ENRPhase::Preparation;C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon(255));Check(C->PrimaryWeapon==ENRPrimaryWeapon::Forge12,TEXT("invalid primary selection rejected"));
  C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon::RoleDefault);GS->Phase=ENRPhase::Combat;
  PC->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
  C->TeleportTo({-2070,680,88},FRotator(0,0,0));PC->SetControlRotation(FRotator(0,0,0));
  for(int I=0;I<3;++I)
  {
   auto* P=GetWorld()->SpawnActor<ANRCharacter>(FVector(-1780,440+I*170,88),FRotator(0,180,0));
   P->Team=0;P->SetRole(ENRRole(I));P->EquippedWeapon=I;P->OnRep_Equipped();P->Tags.Add(TEXT("EvolutionDisplay"));
   P->GetCharacterMovement()->DisableMovement();
  }
 }
 const float E=T-OperationsEpoch;
 if(Stage==1&&E>2)
 {
  ++Stage;
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("EvolutionDisplay")))
  {
   Check(Cast<UNRLocomotion>(It->GetMesh()->GetAnimInstance())!=nullptr,TEXT("all weapon types use native world graph"));
   const FVector Grip=It->GetMesh()->GetSocketLocation(TEXT("HandGrip_R"));
   Check(FVector::Dist(Grip,It->WorldWeapon->GetComponentLocation())<1,TEXT("third-person grip does not detach"));
   const FVector Head=It->GetMesh()->GetBoneLocation(TEXT("head"));Check(!Head.ContainsNaN()&&Head.Z>120&&Head.Z<225,TEXT("retargeted head height and pose finite"));
   if(It->EquippedWeapon<2)Check(FVector::DotProduct(It->WorldWeapon->GetComponentTransform().TransformVectorNoScale(FVector::RightVector),It->GetActorForwardVector())>.99f,TEXT("world barrel aligned with forward aim"));
   UE_LOG(LogTemp,Display,TEXT("NR_POSE slot=%d head=%s muzzle=%s forward=%s"),It->EquippedWeapon,*Head.ToString(),*It->MuzzlePosition().ToString(),*It->WorldWeapon->GetComponentTransform().TransformVectorNoScale(FVector::RightVector).ToString());
  }
  if(Visual)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Evolution-Operators.png"),false,false);
 }
 if(Stage==2&&E>4)
 {
  ++Stage;
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("EvolutionDisplay")))
  {
   if(It->EquippedWeapon<2){It->bReloading=true;It->ReloadStartedAt=GetWorld()->GetGameState<ANRGameState>()->GetServerWorldTimeSeconds();}
   else It->MulticastMelee();
   CastChecked<UNRCharacterMovement>(It->GetCharacterMovement())->LeanAmount=.9;
  }
 }
 if(Stage==3&&E>4.3)
 {
  ++Stage;bool Feet=true;
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("EvolutionDisplay")))Feet&=FMath::Abs(It->GetMesh()->GetRelativeRotation().Roll)<.1;
  Check(Feet,TEXT("lean no longer rolls character around feet"));
  if(Visual)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Evolution-Actions.png"),false,false);
 }
 if(Visual&&Stage==4&&E>5.3f&&NetworkStage==0)
 {
  NetworkStage=1;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("EvolutionDisplay"))&&It->EquippedWeapon==0){It->bDead=true;It->OnRep_Dead();Check(It->bPresentedDeath,TEXT("death produces local animated presentation"));break;}
 }
 if(Visual&&Stage==4&&E>6.3f&&NetworkStage==1){NetworkStage=-1;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Evolution-Death.png"),false,false);}
 if(Stage==4&&E>7)
 {
  ++Stage;NetworkStage=0;
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("EvolutionDisplay")))It->Destroy();
  ANRTacticalDoor* Door=nullptr;int Doors=0;for(TActorIterator<ANRTacticalDoor> It(GetWorld());It;++It){++Doors;Door=*It;}
  Check(Doors==2,TEXT("two authored route shutters"));
  if(Door)
  {
   C->TeleportTo(Door->GetActorLocation()+FVector(0,-140,0),FRotator::ZeroRotator);
   Check(Door->Interact(C),TEXT("nearby player closes shutter"));Check(!Door->Interact(C),TEXT("door spam rejected by cooldown"));
  }
 }
 if(Stage==5&&E>8.5)
 {
  ++Stage;
  for(TActorIterator<ANRTacticalDoor> It(GetWorld());It;++It)if(!It->bOpen)
  {
   Check(It->GateCollision->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics,TEXT("closed shutter blocks movement and bullets"));
   It->ResetDoor();
  }
 }
 if(Stage==6&&E>10)
 {
  ++Stage;
  for(TActorIterator<ANRTacticalDoor> It(GetWorld());It;++It)
  {
   Check(It->bOpen&&It->GateCollision->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("round reset opens route"));
   C->SetActorLocation(It->GetActorTransform().TransformPosition(It->GateOffset));
   auto* Blocker=GetWorld()->SpawnActor<ANRCharacter>(C->GetActorLocation(),FRotator::ZeroRotator);Blocker->GetCharacterMovement()->DisableMovement();
   C->TeleportTo(It->GetActorLocation()+FVector(0,It->GetActorLocation().Y>0?140.f:-140.f,0),FRotator::ZeroRotator);
   Check(!It->Interact(C),TEXT("occupied doorway cannot close"));Blocker->Destroy();
  }
  Check(GetWorld()->GetMapName().Contains(TEXT("Switchyard")),TEXT("new map loaded"));
  int Activities=0;for(TActorIterator<ANRActivity> It(GetWorld());It;++It)++Activities;Check(Activities==7,TEXT("supply, two relay objectives and range present"));
  // Query static collision at intended human routes, not only point traces at the floor.
  FCollisionQueryParams Q;Q.AddIgnoredActor(C);
  for(FVector P:{FVector(-2060,-350,92),FVector(1950,-350,92),FVector(-1420,1040,92),FVector(0,1120,92),FVector(1500,-1450,92),FVector(0,-190,492)})
   Check(!GetWorld()->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q),TEXT("spawn/route landing capsule clear"));
  if(!Visual){UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}
  C->SetActorHiddenInGame(true);
 }
 if(Visual&&Stage>=7&&Stage<=11&&E>12+(Stage-7)*3)
 {
  static const FVector Positions[]={{-1500,-1700,530},{-1380,1180,180},{-1200,-1190,180},{-980,-380,575},{1400,-350,195}};
  static const FVector Targets[]={{450,700,180},{1000,1450,180},{900,-1460,130},{500,0,420},{1880,0,140}};
  auto* Cam=GetWorld()->SpawnActor<ACameraActor>(Positions[Stage-7],(Targets[Stage-7]-Positions[Stage-7]).Rotation());
  Cam->GetCameraComponent()->SetFieldOfView(85);PC->SetViewTarget(Cam);NetworkStage=Stage++;PreparationEpoch=T;
 }
 if(Visual&&NetworkStage>=7&&T-PreparationEpoch>.9)
 {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Evolution-Map%d.png"),NetworkStage-7),false,false);NetworkStage=0;
 }
 if(Visual&&Stage==12&&E>29)
 {
  Stage=13;C->SetActorHiddenInGame(false);C->TeleportTo({-2060,-650,88},FRotator::ZeroRotator);PC->SetControlRotation({-2,20,0});PC->SetViewTarget(C);
  auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Preparation;C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon::Forge12);GS->Phase=ENRPhase::Combat;
 }
 if(Visual&&Stage==13&&E>31){Stage=14;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Evolution-Forge12.png"),true,false);}
 if(Visual&&Stage==14&&E>33){Stage=15;auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Preparation;C->LastPrimaryRequest=-100;C->ServerChoosePrimary(ENRPrimaryWeapon::Lancer60);GS->Phase=ENRPhase::Combat;}
 if(Visual&&Stage==15&&E>35){Stage=16;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Evolution-Lancer60.png"),true,false);}
 if(Visual&&Stage==16&&E>37){Stage=17;GetWorld()->GetGameState<ANRGameState>()->Phase=ENRPhase::Preparation;C->ToggleWorkbench();}
 if(Visual&&Stage==17&&E>39){Stage=18;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Evolution-Arsenal.png"),true,false);}
 if(Visual&&Stage==18&&E>41){UE_LOG(LogTemp,Display,TEXT("NR_EVOLUTION_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}
}
