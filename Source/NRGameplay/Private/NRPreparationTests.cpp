#include "NRSmokeSubsystem.h"
#include "Engine/DamageEvents.h"
#include "NRGameMode.h"
#include "NRPreparationZone.h"
#include "NRCharacter.h"
#include "NRAttributes.h"
#include "NRNode.h"
#include "NRGraphSubsystem.h"
#include "NRBallisticsSubsystem.h"
#include "NRObjective.h"
#include "NRDroneDirector.h"
#include "NRHUD.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void UNRSmokeSubsystem::RunPreparationTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GM||!GS)return;
 auto* Bullets=GetWorld()->GetSubsystem<UNRBallisticsSubsystem>();
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_PREPARATION %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(Stage==0&&T>3)
 {
  Stage=1;PreparationEpoch=T;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);
  Check(GS->Phase==ENRPhase::Preparation&&GS->PhaseEnd-GS->GetServerWorldTimeSeconds()>35&&GS->PhaseEnd-GS->GetServerWorldTimeSeconds()<=45,TEXT("round starts with 45 seconds of preparation"));
  Check(GM->PreparationZone&&GM->PreparationZone->bClosed&&GM->PreparationZone->Barrier->GetCollisionEnabled()!=ECollisionEnabled::NoCollision,TEXT("server barrier blocks before the round"));
  int Targets=0,Stations=0;ANRPreparationProp* Target=nullptr;ANRPreparationProp* Station=nullptr;
  for(TActorIterator<ANRPreparationProp> It(GetWorld());It;++It){Targets+=!It->bStation;Stations+=It->bStation;if(It->bAttackSide){if(It->bStation)Station=*It;else if(!Target)Target=*It;}}
  Check(Targets==6&&Stations==2&&Target&&Station,TEXT("both sides have three targets and a loadout bench"));
  C->Team=GS->AttackTeam;GM->RestartPlayer(PC);Check(GS->CanPrepareAt(C->Team,C->GetActorLocation()),TEXT("attack spawns inside the ready bay"));
  C->Team=1-GS->AttackTeam;GM->RestartPlayer(PC);Check(GS->CanPrepareAt(C->Team,C->GetActorLocation()),TEXT("defense spawns inside the objective area"));
  GS->AttackTeam=0;C->Team=0;GM->RestartPlayer(PC);Check(C->GetActorLocation().X<GS->PreparationBoundaryX,TEXT("side swap moves the new attackers west"));
  C->Team=1;GM->RestartPlayer(PC);Check(C->GetActorLocation().X>GS->PreparationBoundaryX,TEXT("side swap moves the new defenders east"));GS->AttackTeam=1;
  C->Team=1;C->SetActorLocation(FVector(-1200,1500,90));C->ConstrainPreparationMovement();Check(GS->CanPrepareAt(C->Team,C->GetActorLocation(),47),TEXT("authority clamps attack attempts to bypass the barrier"));
  C->Team=0;C->SetActorLocation(FVector(-1900,1500,900));C->ConstrainPreparationMovement();Check(GS->CanPrepareAt(C->Team,C->GetActorLocation(),47),TEXT("defense cannot jump into the attack bay"));
  FHitResult H;const bool Blocked=GetWorld()->LineTraceSingleByChannel(H,FVector(-1750,0,500),FVector(-1400,0,500),ECC_GameTraceChannel1);
  Check(Blocked&&H.GetActor()==GM->PreparationZone,TEXT("closed barrier stops weapon traces"));
  const float Health=C->Attributes->GetHealth(),Armor=C->Attributes->GetArmorDurability();UGameplayStatics::ApplyDamage(C,1000,nullptr,nullptr,nullptr);
  Check(C->IsAlive()&&C->Attributes->GetHealth()==Health&&C->Attributes->GetArmorDurability()==Armor,TEXT("preparation protects health and armor from damage"));
  C->Team=1;C->SetRole(ENRRole::Engineer);C->SetActorLocation(FVector(-1450,-400,110));PC->SetControlRotation(FRotator(-55,0,0));
  const float Credits=C->Attributes->GetHardwareTokens();const int Power=GS->Team1Power;
  Check(!C->BuildNode()&&C->Attributes->GetHardwareTokens()==Credits&&GS->Team1Power==Power,TEXT("attack cannot build across the boundary or spend resources on rejection"));
  C->SetRole(ENRRole::Scout);Check(!C->ScanNode(),TEXT("packet interception is locked until combat"));
  C->SetRole(ENRRole::Engineer);C->Team=1;
  if(Target)
  {
   const FVector At=Target->GetActorLocation()+FVector(300,0,42);C->SetActorLocation(At);C->GetCharacterMovement()->StopMovementImmediately();
   PC->SetControlRotation((Target->GetActorLocation()-C->GetPawnViewLocation()).Rotation());C->LastShot=-100;C->WeaponReadyAt=0;
   const int Ammo=C->Ammo,Hits=C->PracticeHits;C->FireShot();
   Check(C->Ammo==Ammo-1&&Bullets->Bullets.Num()==1&&Bullets->Bullets[0].bPractice,TEXT("server creates finite-ammo practice projectiles"));Bullets->Tick(.05f);
   Check(C->PracticeHits==Hits+1&&Target->bHit,TEXT("physical target hit produces practice feedback"));
   Check(C->Attributes->GetHardwareTokens()==Credits&&C->Kills==0&&C->DroneKills==0,TEXT("practice hits cannot farm credits or combat stats"));
   C->Team=0;Check(Target->TakeDamage(10,FDamageEvent(),PC,C)==0,TEXT("opposite side cannot score on a practice target"));C->Team=1;
  }
  if(Station)
  {
   C->SetActorLocation(Station->GetActorLocation()+FVector(140,0,38));C->Ammo=2;C->ReserveAmmo=0;
   Check(Station->Interact(C)&&C->Ammo==C->MagazineCapacity()&&C->ReserveAmmo==C->ReserveCapacity(),TEXT("nearby bench restores a finite loadout"));
   C->SetActorLocation(FVector(-2000,-1600,90));C->Ammo=2;Check(!Station->Interact(C)&&C->Ammo==2,TEXT("bench rejects remote refill requests"));
  }
  auto* Victim=GetWorld()->SpawnActor<ANRCharacter>(FVector(-2150,1200,100),FRotator::ZeroRotator);Victim->Team=0;
  C->SetActorLocation(FVector(-1960,1200,100));PC->SetControlRotation((Victim->GetActorLocation()-C->GetPawnViewLocation()).Rotation());
  Bullets->Fire(C,C->GetPawnViewLocation(),PC->GetControlRotation().Vector(),1000,42000);Bullets->Tick(.02f);
  Check(Victim->IsAlive()&&Victim->Attributes->GetHealth()==100,TEXT("practice bullets do not damage enemy players"));
  C->EquippedWeapon=2;C->SetActorLocation(FVector(-2040,1200,100));PC->SetControlRotation((Victim->GetActorLocation()-C->GetPawnViewLocation()).Rotation());C->ResolveMelee();
  Check(Victim->Attributes->GetHealth()==100,TEXT("practice melee does not damage players"));Victim->Destroy();C->RefillLoadout();
  for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(!It->bExtraction){It->Interact(C);break;}
  Check(!C->bCarryingDisk,TEXT("objective pickup stays locked during preparation"));
  C->Team=0;C->SetRole(ENRRole::Engineer);C->SetActorLocation(FVector(-1450,-400,110));PC->SetControlRotation(FRotator(-55,0,0));C->LastRequest=-100;C->AbilityReadyTime=0;C->AbilityPressed();Printed=C->PrintingNode;
  Check(Printed.IsValid()&&Printed->Team==0&&Printed->NodeState==ENRNodeState::Printing,TEXT("defender GAS builds real paid infrastructure during preparation"));
  Check(GS->Team0Power==90&&C->Attributes->GetHardwareTokens()==Credits-80,TEXT("preparation construction uses normal team power and credits"));
 }
 if(Stage==1&&T>PreparationEpoch+3.4f)
 {
  Stage=2;Check(Printed.IsValid()&&Printed->NodeState==ENRNodeState::Active,TEXT("defensive polymer print completes before combat"));
  ANodeBase* Second=GetWorld()->SpawnActor<ANodeBase>(FVector(-1000,-450,65),FRotator::ZeroRotator);Second->Initialize(0,ENRNodeOp::Turret);Second->CompletePrinting();
  auto* Graph=GetWorld()->GetSubsystem<UNRGraphSubsystem>();Check(Printed.IsValid()&&Graph->Connect(Printed.Get(),Second,0),TEXT("defenders connect logic during preparation"));
  C->LastEquip=-100;C->ServerEquip(1);C->Ammo=1;C->ReserveAmmo=0;C->AbilityReadyTime=T+30;
  Bullets->Fire(C,C->GetPawnViewLocation(),FVector::UpVector,30,42000);Check(Bullets->Bullets.Num()>0,TEXT("practice projectile exists before phase transition"));
  const float Credits=C->Attributes->GetHardwareTokens();GM->BeginCombat();
  Check(GS->Phase==ENRPhase::Combat&&!GM->PreparationZone->bClosed&&GM->PreparationZone->Barrier->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("combat opens barrier and starts objective timer"));
  Check(Bullets->Bullets.IsEmpty(),TEXT("transition removes all queued practice projectiles"));
  Check(C->EquippedWeapon==1&&C->Ammo==C->MagazineCapacity()&&C->ReserveAmmo==36,TEXT("combat refills both ammo banks and preserves selected sidearm"));
  Check(C->AbilityReadyTime==0&&C->Attributes->GetHardwareTokens()==Credits,TEXT("combat readies gadget without resetting economy"));
  Check(Printed.IsValid()&&Printed->NodeState==ENRNodeState::Active&&Printed->OutputPins.Num()==1&&GS->Team0Power==90,TEXT("combat preserves prepared nodes links and resource spending"));
  bool Hidden=true,Locked=true;for(TActorIterator<ANRPreparationProp> It(GetWorld());It;++It){Hidden&=!It->bAvailable;if(!It->bStation)Hidden&=It->Collision->GetCollisionEnabled()==ECollisionEnabled::NoCollision;Locked&=!It->Interact(C);}
  Check(Locked,TEXT("combat cannot use preparation resupply"));
  Check(Hidden,TEXT("warmup targets retract and stop blocking combat"));
  C->SetActorLocation(FVector(-1800,1500,100));const FVector Position=C->GetActorLocation();C->ConstrainPreparationMovement();Check(C->GetActorLocation().Equals(Position),TEXT("combat removes preparation movement restriction"));
  UGameplayStatics::ApplyDamage(C,10,nullptr,nullptr,nullptr);Check(C->Attributes->GetHealth()<100,TEXT("combat restores real damage"));
  const auto Old=Printed;GM->BeginRound();
  Check(GS->Phase==ENRPhase::Preparation&&GM->PreparationZone->bClosed&&!Old.IsValid()&&C->PracticeHits==0,TEXT("next round resets infrastructure gate and practice counters"));
  auto* OtherPC=GetWorld()->SpawnActor<APlayerController>();auto* OtherPawn=GetWorld()->SpawnActor<ANRCharacter>(FVector(1800,1700,100),FRotator::ZeroRotator);OtherPawn->Team=1;OtherPC->Possess(OtherPawn);GM->bTraining=false;GS->Round=6;GM->BeginRound();
  Check(GS->Round==7&&GS->AttackTeam==0&&C->GetActorLocation().X<GS->PreparationBoundaryX,TEXT("round seven swaps side roles and respawns correctly"));
  bool Ready=true;for(TActorIterator<ANRPreparationProp> It(GetWorld());It;++It)Ready&=It->bAvailable;Check(Ready,TEXT("new round reactivates all preparation stations and targets"));
  UE_LOG(LogTemp,Display,TEXT("NR_PREPARATION_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
 }
 if(T>40){UE_LOG(LogTemp,Error,TEXT("NR_PREPARATION_TIMEOUT"));FPlatformMisc::RequestExitWithStatus(false,1);}
}

void UNRSmokeSubsystem::RunPreparationNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS)return;
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_PREPNET %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(GetWorld()->GetNetMode()!=NM_Client)
 {
  auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();if(!GM)return;
  if(Stage==0&&GM->GetNumPlayers()>=2)
  {
   Stage=1;PreparationEpoch=T;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);GS->Phase=ENRPhase::Preparation;GS->PhaseEnd=T+10;GS->Announcement=TEXT("NR_PREP_NETWORK");GS->ForceNetUpdate();GM->PreparationZone->SetPreparing(true);
   // Select unobstructed real map lanes so cover and combat AI cannot mask a gate test.
   for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)GetWorld()->GetTimerManager().ClearTimer(It->WaveTimer);
   FCollisionQueryParams Query;Query.AddIgnoredActor(GM->PreparationZone);for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)Query.AddIgnoredActor(*It);
   TArray<float> Lanes;for(float Y:{1800.f,-1800.f,1600.f,-1600.f,1100.f,-1100.f,700.f,-700.f,300.f,-300.f})
   {FHitResult Hit;if(!GetWorld()->SweepSingleByChannel(Hit,FVector(-1870,Y,100),FVector(-1270,Y,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Query))Lanes.Add(Y);}
   Check(Lanes.Num()>=2,TEXT("two physical crossing lanes are clear of map cover"));
   int Side=0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller){const int Index=Side++;It->Team=Index%2;It->ResetForRound();const float Y=Lanes.Num()>Index?Lanes[Index]:0;It->SetActorLocation(FVector(It->Team==GS->AttackTeam?-1850:-1300,Y,100));It->Controller->SetControlRotation(FRotator(0,It->Team==GS->AttackTeam?0:180,0));It->ForceNetUpdate();}
   GetWorld()->GetTimerManager().SetTimer(GM->PhaseTimer,GM,&ANRGameMode::BeginCombat,10.f,false);
  }
  if(Stage==1&&T>PreparationEpoch+7)
  {
   Stage=2;int Count=0;bool Inside=true;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller){++Count;Inside&=GS->CanPrepareAt(It->Team,It->GetActorLocation(),44);}
   Check(Count==2&&Inside&&GM->PreparationZone->bClosed,TEXT("authority contains both moving teams during preparation"));
  }
  if(Stage==2&&T>PreparationEpoch+14)
  {
   Stage=3;bool Crossed=true;int Count=0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller){++Count;Crossed&=It->Team==GS->AttackTeam?It->GetActorLocation().X>GS->PreparationBoundaryX:It->GetActorLocation().X<GS->PreparationBoundaryX;UE_LOG(LogTemp,Display,TEXT("NR_PREPNET server team %d position %s"),It->Team,*It->GetActorLocation().ToString());}
   Check(Count==2&&Crossed&&GS->Phase==ENRPhase::Combat&&!GM->PreparationZone->bClosed,TEXT("authority lets both teams cross after automatic timer"));UE_LOG(LogTemp,Display,TEXT("NR_PREPNET_SERVER_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));
  }
  return;
 }
 if(!C||!PC)return;
 if(Stage==0&&GS->Announcement==TEXT("NR_PREP_NETWORK")){Stage=1;PreparationEpoch=T;}
 if(Stage==1&&GS->Phase==ENRPhase::Preparation)
 {
  C->AddMovementInput(FVector(C->Team==GS->AttackTeam?1:-1,0,0));
  if(T>PreparationEpoch+5&&NetworkStage==0)
  {
   NetworkStage=1;Check(GS->CanPrepareAt(C->Team,C->GetActorLocation(),40),TEXT("predicted owner remains on assigned side"));
   ANRPreparationZone* Zone=nullptr;for(TActorIterator<ANRPreparationZone> It(GetWorld());It;++It)Zone=*It;
   Check(Zone&&Zone->bClosed&&Zone->Barrier->GetCollisionEnabled()!=ECollisionEnabled::NoCollision,TEXT("client receives closed barrier and collision"));
   Check(GS->PhaseEnd-GS->GetServerWorldTimeSeconds()>0,TEXT("client shares server preparation deadline"));
  }
 }
 if(Stage==1&&GS->Phase==ENRPhase::Combat)
 {
  Stage=2;PreparationEpoch=T;ANRPreparationZone* Zone=nullptr;for(TActorIterator<ANRPreparationZone> It(GetWorld());It;++It)Zone=*It;
  Check(C->Ammo==C->MagazineCapacity()&&C->ReserveAmmo==C->ReserveCapacity(),TEXT("owner receives combat loadout"));
 }
 if(Stage==2)
 {
  C->AddMovementInput(FVector(C->Team==GS->AttackTeam?1:-1,0,0));
  if(T>PreparationEpoch+6)
  {
   Stage=3;UE_LOG(LogTemp,Display,TEXT("NR_PREPNET owner team %d position %s"),C->Team,*C->GetActorLocation().ToString());Check(C->Team==GS->AttackTeam?C->GetActorLocation().X>GS->PreparationBoundaryX:C->GetActorLocation().X<GS->PreparationBoundaryX,TEXT("owner passes through the opened barrier"));
   ANRPreparationZone* Zone=nullptr;for(TActorIterator<ANRPreparationZone> It(GetWorld());It;++It)Zone=*It;
   Check(Zone&&!Zone->bClosed&&Zone->Barrier->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("replicated combat state removes client barrier collision"));
   UE_LOG(LogTemp,Display,TEXT("NR_PREPNET_CLIENT_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
  }
 }
 if(T>55){Check(false,TEXT("network preparation timeout"));FPlatformMisc::RequestExitWithStatus(false,1);}
}

void UNRSmokeSubsystem::RunPreparationVisual(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();if(!GM)return;
 if(Stage==0&&T>3){Stage=1;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);C->Team=1;C->SetRole(ENRRole::Engineer);C->TeleportTo(FVector(-2050,-250,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-2,24,0));}
 if(Stage==1&&T>6){Stage=2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/PreparationAttack.png"),true,false);}
 if(Stage==2&&T>7){Stage=3;C->TeleportTo(FVector(-1970,1000,100),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-9,180,0));}
 if(Stage==3&&T>9){Stage=4;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/PreparationRange.png"),true,false);}
 if(Stage==4&&T>10){Stage=5;C->Team=0;C->TeleportTo(FVector(1500,-430,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-42,5,0));C->LastRequest=-100;C->AbilityReadyTime=0;C->AbilityPressed();}
 if(Stage==5&&T>14){Stage=6;PC->SetControlRotation(FRotator(-4,10,0));FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/PreparationDefense.png"),true,false);}
 if(Stage==6&&T>15){Stage=7;GM->BeginCombat();C->Team=1;C->TeleportTo(FVector(-1700,0,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-3,0,0));}
 if(Stage==7&&T>18){Stage=8;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/PreparationOpen.png"),true,false);}
 if(Stage==8&&T>20){UE_LOG(LogTemp,Display,TEXT("NR_PREPARATION_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0);}
}
