#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRCharacterMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "NRAttributes.h"
#include "NRNode.h"
#include "NRGraphSubsystem.h"
#include "NRGameMode.h"
#include "NRStructure.h"
#include "NRDroneDirector.h"
#include "NRObjective.h"
#include "NRActivity.h"
#include "NRAlgorithms.h"
#include "NRDebrisSubsystem.h"
#include "NRBallisticsSubsystem.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "NRHUD.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
bool UNRSmokeSubsystem::ShouldCreateSubsystem(UObject* O)const{
#if UE_BUILD_SHIPPING
 return false;
#else
 const auto* W=Cast<UWorld>(O);return W&&W->IsGameWorld()&&(FParse::Param(FCommandLine::Get(),TEXT("NREvolutionNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NREvolutionTest"))||FParse::Param(FCommandLine::Get(),TEXT("NREvolutionVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRRosterTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRRosterVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRRosterNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerSystemTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerJoinTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerLifecycle"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRPreparationTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRPreparationNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRPreparationVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRBreacherNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRBreacherTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRBreacherVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NROperationsTest"))||FParse::Param(FCommandLine::Get(),TEXT("NROperationsNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NROperationsVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRStanceTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRStanceNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRStanceVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRDamageAudioTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRModuleTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRModuleNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRExpansionTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRDetailTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRSmokeTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRVisualTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRMatchTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRCollisionTest")));
#endif
}
void UNRSmokeSubsystem::Tick(float){const float T=GetWorld()->GetTimeSeconds();auto* PC=GetWorld()->GetFirstPlayerController();auto* C=PC?Cast<ANRCharacter>(PC->GetPawn()):nullptr;



 if(FParse::Param(FCommandLine::Get(),TEXT("NREvolutionNetwork"))){RunEvolutionNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NREvolutionTest"))||FParse::Param(FCommandLine::Get(),TEXT("NREvolutionVisual"))){RunEvolution(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRRosterTest"))){RunRosterTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRRosterVisual"))){RunRosterVisual(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRRosterNetwork"))){RunRosterNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRServerSystemTest"))){RunServerSystemTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRServerJoinTest"))){RunServerJoinTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRServerVisual"))){RunServerVisual(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRServerLifecycle"))){for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)GetWorld()->GetTimerManager().ClearTimer(It->WaveTimer);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRPreparationTest"))){RunPreparationTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRPreparationNetwork"))){RunPreparationNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRPreparationVisual"))){RunPreparationVisual(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRBreacherNetwork"))){RunBreacherNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRBreacherTest"))){RunBreacherTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRBreacherVisual"))){RunBreacherVisual(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NROperationsTest"))){RunOperationsTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NROperationsNetwork"))){RunOperationsNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NROperationsVisual"))){RunOperationsVisual(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRStanceTest"))){RunStanceTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRStanceNetwork"))){RunStanceNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRStanceVisual"))){RunStanceVisual(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRDamageAudioTest"))){RunDamageAudioTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRModuleTest"))){RunModuleTest(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRModuleNetwork"))){RunModuleNetwork(C,PC,T);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NRExpansionTest")))
 {
  if(!C||!PC)return;auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();
  auto Check=[this](bool OK,const TCHAR* Label){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_EXPANSION %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
  if(Stage==0&&T>3)
  {
   Stage=1;FVector Reflected;
   Check(NR::MetalRicochet(FVector(40000,0,-4000),FVector::UpVector,0,Reflected)&&Reflected.Z>0&&Reflected.Size()<41000,TEXT("grazing metal hit reflects with energy loss"));
   Check(!NR::MetalRicochet(FVector(0,0,-40000),FVector::UpVector,0,Reflected),TEXT("normal incidence cannot ricochet"));
   Check(!NR::MetalRicochet(FVector(40000,0,-4000),FVector::UpVector,1,Reflected),TEXT("projectile cannot bounce twice"));
   if(auto* Debris=GetWorld()->GetSubsystem<UNRDebrisSubsystem>()){for(int I=0;I<12;++I)Debris->SpawnBurst(FVector(-2300,1900,50),I);Check(Debris->ActivePieces()<=128,TEXT("local physical debris is bounded"));}
   bool Grounded=true;int Stations=0;for(TActorIterator<ANRActivity> It(GetWorld());It;++It){++Stations;FVector Foot=It->GetActorLocation()-FVector(0,0,It->Kind==ENRActivityKind::Target?44:52);FHitResult H;FCollisionQueryParams Q;Q.AddIgnoredActor(*It);Grounded&=GetWorld()->LineTraceSingleByChannel(H,Foot+FVector(0,0,8),Foot-FVector(0,0,15),ECC_Visibility,Q);}
   Check(Grounded&&Stations==6,TEXT("six map activities rest on solid reachable floors"));
   GM->BeginCombat();C->ResetForRound();C->Ammo=17;C->ReserveAmmo=40;C->ServerEquip(1);
   Check(C->EquippedWeapon==1&&C->Ammo==12&&C->ReserveAmmo==36,TEXT("pistol has independent finite magazine"));
   C->ServerFire(true);Check(C->Ammo==12,TEXT("equip delay blocks immediate fire"));
   C->LastEquip=-100;C->ServerEquip(255);Check(C->EquippedWeapon==1,TEXT("invalid weapon id rejected"));
  }
  if(Stage==1&&T>4)
  {
   Stage=2;C->ServerFire(true);Check(C->Ammo==11&&!C->GetWorldTimerManager().IsTimerActive(C->FireTimer),TEXT("pistol fires once per press"));
   C->ServerReload();C->LastEquip=-100;C->ServerEquip(0);
   Check(C->Ammo==17&&C->ReserveAmmo==40&&!C->bReloading,TEXT("swap preserves rifle and cancels pistol reload"));
  }
  if(Stage==2&&T>6)
  {
   Stage=3;C->ServerEquip(1);Check(C->Ammo==11&&C->ReserveAmmo==36,TEXT("cancelled reload does not create ammo"));C->WeaponReadyAt=0;C->Ammo=10;C->ReserveAmmo=1;C->ServerReload();
  }
  if(Stage==3&&T>8)
  {
   Stage=4;Check(C->Ammo==11&&C->ReserveAmmo==0,TEXT("pistol partial reload consumes final round"));
   C->ServerEquip(2);C->WeaponReadyAt=0;C->AimPressed();C->ServerReload();Check(!C->bAiming&&!C->bReloading&&C->Ammo==0,TEXT("knife has no ADS reload or ammo"));
   C->TeleportTo(FVector(-1850,700,90),FRotator::ZeroRotator);PC->SetControlRotation(FRotator::ZeroRotator);C->GetCharacterMovement()->StopMovementImmediately();
   auto* Enemy=GetWorld()->SpawnActor<ANRCharacter>(FVector(-1700,700,88),FRotator::ZeroRotator);Enemy->Team=0;Enemy->Tags.Add(TEXT("ExpansionEnemy"));
   C->LastShot=-100;C->ServerFire(true);
  }
  if(Stage==4&&T>9)
  {
   Stage=5;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("ExpansionEnemy"))){Check(It->Attributes->GetHealth()<100&&It->Attributes->GetHealth()>0,TEXT("knife deals server damage in reach"));It->Attributes->SetHealth(100);}
   auto* Wall=GetWorld()->SpawnActor<AActor>();Wall->Tags.Add(TEXT("ExpansionWall"));auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(10,80,100));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Wall->SetActorLocation(FVector(-1790,700,110));
   C->LastShot=-100;C->ServerFire(true);const float Shot=C->LastShot;C->ServerFire(true);Check(C->LastShot==Shot,TEXT("melee repeat respects cooldown"));
  }
  if(Stage==5&&T>10)
  {
   Stage=6;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("ExpansionEnemy"))){Check(It->Attributes->GetHealth()==100,TEXT("wall blocks melee damage"));It->Destroy();}
   for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("ExpansionWall")))It->Destroy();
   C->TeleportTo(FVector(-1850,1100,90),FRotator::ZeroRotator);C->LastEquip=-100;C->ServerEquip(0);C->ReserveAmmo=0;C->Attributes->SetHealth(40);
   auto* A=GetWorld()->SpawnActor<ANRActivity>(FVector(-1680,1100,60),FRotator::ZeroRotator);A->Kind=ENRActivityKind::Supply;A->Tags.Add(TEXT("ExpansionSupply"));A->Interact(C);A->Interact(C);
   Check(A->bCompleted&&C->ReserveAmmo==30&&C->Attributes->GetHealth()==75,TEXT("supply grants once and cannot be farmed"));A->Destroy();
   auto* Relay=GetWorld()->SpawnActor<ANRActivity>(FVector(-1680,1100,60),FRotator::ZeroRotator);Relay->Kind=ENRActivityKind::Relay;Relay->Tags.Add(TEXT("ExpansionRelay"));Relay->Interact(C);
  }
  if(Stage==6&&T>11)
  {
   Stage=7;for(TActorIterator<ANRActivity> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("ExpansionRelay")))Check(It->Progress>0&&!It->bCompleted,TEXT("relay channels over time"));
   C->TeleportTo(FVector(-2300,1000,90),FRotator::ZeroRotator);
  }
  if(Stage==7&&T>12)
  {
   Stage=8;for(TActorIterator<ANRActivity> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("ExpansionRelay"))){Check(It->Progress==0&&!It->bCompleted,TEXT("leaving relay cancels channel"));C->TeleportTo(FVector(-1850,1100,90),FRotator::ZeroRotator);C->Attributes->SetHardwareTokens(0);It->Interact(C);}
  }
  if(Stage==8&&T>16.5f)
  {
   Stage=9;for(TActorIterator<ANRActivity> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("ExpansionRelay"))){Check(It->bCompleted&&C->Attributes->GetHardwareTokens()==180,TEXT("relay completes once with reward"));It->Interact(C);Check(C->Attributes->GetHardwareTokens()==180,TEXT("relay reward cannot repeat"));It->ResetActivity();Check(!It->bCompleted&&It->Progress==0,TEXT("round resets activity"));}
   auto* Target=GetWorld()->SpawnActor<ANRActivity>(FVector(-1950,0,100),FRotator::ZeroRotator);Target->Kind=ENRActivityKind::Target;float Before=C->Attributes->GetHardwareTokens();FDamageEvent D;Target->TakeDamage(34,D,PC,C);Target->TakeDamage(34,D,PC,C);
   Check(Target->bCompleted&&C->Attributes->GetHardwareTokens()==Before+40,TEXT("range target reward limited to first hit"));
   C->LastEquip=-100;C->ServerEquip(2);C->ResetForRound();Check(C->EquippedWeapon==0&&C->Ammo==30&&C->ReserveAmmo==90,TEXT("round reset restores complete arsenal"));
   auto* Plate=GetWorld()->SpawnActor<AActor>();Plate->Tags.Add(TEXT("SurfaceMetal"));Plate->Tags.Add(TEXT("RicochetTestPlate"));auto* Box=NewObject<UBoxComponent>(Plate);Plate->SetRootComponent(Box);Box->SetBoxExtent(FVector(1000,300,10));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Plate->SetActorLocation(FVector(1000,5000,-10));
   GetWorld()->GetSubsystem<UNRBallisticsSubsystem>()->Fire(C,FVector(0,5000,100),FVector(1,0,-.1),34,40000);
  }
  if(Stage==9&&T>17.2f)
  {
   Stage=10;bool Reflected=false;for(const auto& B:GetWorld()->GetSubsystem<UNRBallisticsSubsystem>()->Bullets)Reflected|=B.Bounces==1&&B.Velocity.Z>0&&FMath::IsNearlyEqual(B.Damage,15.3f,.01f);
   Check(Reflected,TEXT("authoritative projectile physically ricochets from test plate"));
   for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("RicochetTestPlate")))It->Destroy();
   UE_LOG(LogTemp,Display,TEXT("NR_EXPANSION_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
  }
  return;
 }

 if(FParse::Param(FCommandLine::Get(),TEXT("NRDetailTest")))
 {
  if(!C||!PC)return;
  auto Check=[this](bool OK,const TCHAR* Label){if(!OK)bFailed=true;UE_LOG(LogTemp,Display,TEXT("NR_DETAIL %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
  auto* Move=CastChecked<UNRCharacterMovement>(C->GetCharacterMovement());
  if(Stage==0&&T>3)
  {
   Stage=1;C->SetRole(ENRRole::Scout);Check(C->Ammo==12&&C->ReserveAmmo==36,TEXT("scout 12 plus 36 loadout"));
   C->SetRole(ENRRole::Assault);Check(C->Ammo==30&&C->ReserveAmmo==60,TEXT("assault 30 plus 60 loadout"));
   C->SetRole(ENRRole::Engineer);Check(C->Ammo==30&&C->ReserveAmmo==90,TEXT("engineer 30 plus 90 loadout"));
   C->Ammo=27;C->ReserveAmmo=2;C->ServerReload();
   C->TeleportTo(FVector(-1750,-350,100),FRotator::ZeroRotator);PC->SetControlRotation(FRotator::ZeroRotator);
  }
  if(Stage==1&&T>5)
  {
   Stage=2;Check(C->Ammo==29&&C->ReserveAmmo==0&&!C->bReloading,TEXT("partial reload conserves final two rounds"));
   C->ServerReload();Check(!C->bReloading,TEXT("empty reserve rejects reload"));
   C->ServerSelectSkin(2);C->LastSkinRequest=-100;C->ServerSelectSkin(250);Check(C->WeaponSkin==2,TEXT("invalid cosmetic id rejected"));
   C->RefillLoadout();C->Ammo=20;C->ServerReload();C->ServerReload();
   C->SprintPressed();
  }
  if(Stage==2&&T<7)C->AddMovementInput(FVector::ForwardVector,1);
  if(Stage==2&&T>=7)
  {
   Stage=3;Check(C->Ammo==30&&C->ReserveAmmo==80,TEXT("repeated reload request consumes exactly ten rounds"));
   Check(C->IsSprinting()&&C->GetVelocity().Size2D()>500,TEXT("predicted sprint reaches intended speed"));
   C->AimPressed();Check(!C->IsSprinting()&&Move->GetMaxSpeed()==260,TEXT("aim cancels sprint and limits movement"));
   C->AimReleased();Move->StopMovementImmediately();C->CrouchPressed();
  }
  if(Stage==3&&T>=8)
  {
   Stage=4;Check(FMath::IsNearlyEqual(C->GetMesh()->GetRelativeLocation().Z,-56.,.1),TEXT("crouched mesh stays grounded relative to capsule"));Check(C->bIsCrouched&&C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()==56&&Move->GetMaxSpeed()==185,TEXT("crouch changes collision capsule and speed"));
   auto* Roof=GetWorld()->SpawnActor<AActor>();Roof->Tags.Add(TEXT("DetailTestRoof"));auto* Box=NewObject<UBoxComponent>(Roof);Roof->SetRootComponent(Box);
   Box->SetBoxExtent(FVector(100,100,10));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Roof->SetActorLocation(C->GetActorLocation()+FVector(0,0,70));
   C->CrouchReleased();
  }
  if(Stage==4&&T>=9)
  {
   Stage=5;Check(C->bIsCrouched,TEXT("low ceiling prevents standing through collision"));
   for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("DetailTestRoof")))It->Destroy();
  }
  if(Stage==5&&T>=10)
  {
   Stage=6;Check(!C->bIsCrouched&&C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()==88,TEXT("standing restores capsule after obstruction clears"));
   Move->UpdateFromCompressedFlags(FSavedMove_Character::FLAG_Custom_0);Check(Move->bWantsSprint,TEXT("sprint intent decoded from movement packet"));
   Move->UpdateFromCompressedFlags(0);Check(!Move->bWantsSprint,TEXT("sprint release decoded from movement packet"));
   GetWorld()->GetAuthGameMode<ANRGameMode>()->BeginCombat();C->Ammo=0;C->ReserveAmmo=0;const float OldShot=C->LastShot;C->ServerFire(true);C->ServerFire(false);
   Check(C->Ammo==0&&C->LastShot==OldShot,TEXT("empty magazine cannot fire"));
   const auto OldRole=C->OperatorRole;C->ServerSelectClass(ENRRole::Scout);Check(C->OperatorRole==OldRole&&C->ReserveAmmo==0,TEXT("combat class swap cannot refill ammo"));
   C->ResetForRound();Check(C->Ammo==30&&C->ReserveAmmo==90,TEXT("round reset restores finite loadout"));
   UE_LOG(LogTemp,Display,TEXT("NR_DETAIL_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("NRCollisionTest")))
 {
  if(!C||!PC)return;
  auto Check=[this](bool OK,const TCHAR* Label){if(!OK)bFailed=true;UE_LOG(LogTemp,Display,TEXT("NR_COLLISION %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
  if(Stage==0&&T>3)
  {
   Stage=1;C->TeleportTo(FVector(-1660,-1150,90),FRotator::ZeroRotator);PC->SetControlRotation(FRotator::ZeroRotator);
   FHitResult Floor;FCollisionQueryParams Q;Q.AddIgnoredActor(C);
   Check(GetWorld()->LineTraceSingleByChannel(Floor,FVector(-1700,-1150,200),FVector(-1700,-1150,-100),ECC_Visibility,Q)&&Floor.ImpactNormal.Z>.9,TEXT("walkable solid floor"));
  }
  if(Stage==1&&T<6)C->AddMovementInput(FVector::ForwardVector,1);
  if(Stage==1&&T>=6)
  {
   Stage=2;UE_LOG(LogTemp,Display,TEXT("NR_COLLISION stair position %s"),*C->GetActorLocation().ToString());
   Check(C->GetActorLocation().Z>400&&C->GetActorLocation().X>-750,TEXT("capsule climbs all 12 stairs"));
   C->GetCharacterMovement()->StopMovementImmediately();C->TeleportTo(FVector(-1750,-650,90),FRotator::ZeroRotator);
  }
  if(Stage==2&&T<8)C->AddMovementInput(FVector::ForwardVector,1);
  if(Stage==2&&T>=8)
  {
   Stage=3;Check(C->GetActorLocation().X<-1590&&C->GetActorLocation().X>-1700,TEXT("cover blocks player capsule"));
   C->GetCharacterMovement()->StopMovementImmediately();
   FHitResult Floor;FCollisionQueryParams Q;Q.AddIgnoredActor(C);
   Check(GetWorld()->LineTraceSingleByChannel(Floor,FVector(0,0,600),FVector(0,0,200),ECC_Visibility,Q)&&Cast<ANRStructure>(Floor.GetActor()),TEXT("intact Chaos panel supports collision"));
   for(TActorIterator<ANRStructure> It(GetWorld());It;++It)if(It->bFloor&&FMath::Abs(It->GetActorLocation().Y)<50)It->BreachServer(It->GetActorLocation(),FVector::DownVector,20000);
  }
  if(Stage==3&&T>=12)
  {
   Stage=4;FHitResult Floor;FCollisionQueryParams Q;Q.AddIgnoredActor(C);
   bool Hit=GetWorld()->LineTraceSingleByChannel(Floor,FVector(0,0,600),FVector(0,0,200),ECC_Visibility,Q);
   Check(!Hit,TEXT("breach leaves an actual traversable hole"));
   UE_LOG(LogTemp,Display,TEXT("NR_COLLISION_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("NRVisualTest")))
 {
  if(!C||!PC)return;
  if(Stage==0&&T>3){Stage=1;C->SetRole(ENRRole::Assault);if(FParse::Param(FCommandLine::Get(),TEXT("NRDetailVisual"))){int32 Skin=1;FParse::Value(FCommandLine::Get(),TEXT("NRSkin="),Skin);C->ServerSelectSkin(uint8(FMath::Clamp(Skin,0,3)));C->bSkinLoaded=true;}C->TeleportTo(FVector(-1850,-920,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-3,22,0));
   int32 Slot=0;if(FParse::Value(FCommandLine::Get(),TEXT("NRSlot="),Slot))C->ServerEquip(uint8(FMath::Clamp(Slot,0,2)));
   if(FParse::Param(FCommandLine::Get(),TEXT("NRModuleVisual"))){FNRWeaponAssembly B;B.Optic=Slot==1?ENROptic::Reflex:ENROptic::Combat2x;B.Muzzle=ENRMuzzle::Suppressor;B.Magazine=ENRMagazine::Extended;C->ServerConfigureWeapon(uint8(Slot),B);}
   if(FParse::Param(FCommandLine::Get(),TEXT("NRAimPreview"))){C->SetRole(ENRRole::Scout);C->AimPressed();}
   if(FParse::Param(FCommandLine::Get(),TEXT("NRGraphPreview")))
   {
    C->SetRole(ENRRole::Engineer);TArray<ANodeBase*> Nodes;
    for(int32 I=0;I<3;++I){auto* N=GetWorld()->SpawnActor<ANodeBase>(FVector(-1650+I*170,-800,65),FRotator::ZeroRotator);N->Initialize(1,I==0?ENRNodeOp::Sensor:I==1?ENRNodeOp::Delay:ENRNodeOp::Turret);N->CompletePrinting();Nodes.Add(N);}
    auto* G=GetWorld()->GetSubsystem<UNRGraphSubsystem>();G->Connect(Nodes[0],Nodes[1],1);G->Connect(Nodes[1],Nodes[2],1);C->ToggleGraph();
   }
}
  if(Stage==1&&T>7){Stage=2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/01_Arcology.png"),true,false);}
  if(Stage==2&&T>9){Stage=3;GetWorld()->GetAuthGameMode<ANRGameMode>()->BeginCombat();C->FirePressed();if(FParse::Param(FCommandLine::Get(),TEXT("NRDetailVisual"))){C->FireReleased();C->InspectPressed();}}
  if(Stage==3&&T>(C->EquippedWeapon==2?9.22f:9.7f)){Stage=4;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/02_Combat.png"),true,false);}
  if(Stage==4&&T>10){Stage=5;C->FireReleased();C->ReloadPressed();}
  if(Stage==5&&T>10.65f){Stage=6;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/05_Reload.png"),true,false);}
  if(Stage==6&&T>12){Stage=7;C->AimPressed();}
  if(Stage==7&&T>13){Stage=8;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/06_ADS.png"),true,false);}
  if(Stage==8&&T>14){Stage=9;C->AimReleased();const bool Expansion=FParse::Param(FCommandLine::Get(),TEXT("NRExpansionVisual"));auto* Cam=GetWorld()->SpawnActor<ACameraActor>(Expansion?FVector(-1800,1650,240):FVector(-1430,-1700,590),Expansion?FRotator(-3,0,0):FRotator(-15,35,0));Cam->GetCameraComponent()->SetFieldOfView(86);PC->SetViewTarget(Cam);}
  if(Stage==9&&T>15){Stage=10;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/03_Arena.png"),true,false);}
  if(Stage==10&&T>16){Stage=11;
   auto* Operator=GetWorld()->SpawnActor<ANRCharacter>(FVector(-1300,-250,88),FRotator(0,-145,0));Operator->Team=1;Operator->SetRole(ENRRole::Engineer);if(FParse::Param(FCommandLine::Get(),TEXT("NRCrouchPreview"))){Operator->GetCharacterMovement()->bRunPhysicsWithNoController=true;Operator->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Operator->Crouch();}
   const bool CrouchPreview=FParse::Param(FCommandLine::Get(),TEXT("NRCrouchPreview"));const FVector CameraOrigin(-1640,-500,CrouchPreview?135:170);auto* Cam=GetWorld()->SpawnActor<ACameraActor>(CameraOrigin,(FVector(-1300,-250,CrouchPreview?70:136)-CameraOrigin).Rotation());Cam->GetCameraComponent()->SetFieldOfView(55);PC->SetViewTarget(Cam);
  }
  if(Stage==11&&T>17){Stage=12;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/07_Operator.png"),true,false);}
  if(Stage==12&&T>18){Stage=13;if(FParse::Param(FCommandLine::Get(),TEXT("NRModuleVisual"))){PC->SetViewTarget(C);GetWorld()->GetGameState<ANRGameState>()->Phase=ENRPhase::Preparation;C->ToggleWorkbench();}else if(auto* HUD=Cast<ANRHUD>(PC->GetHUD()))HUD->ToggleMenu();}
  if(Stage==13&&T>19){Stage=14;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/04_Menu.png"),true,false);}
  if(Stage==14&&T>21){Stage=15;UE_LOG(LogTemp,Display,TEXT("NR_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0);}
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("NRMatchTest")))
 {
  auto Check=[this](bool OK,const TCHAR* Label){if(!OK)bFailed=true;UE_LOG(LogTemp,Display,TEXT("NR_MATCH %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
  auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();auto* GS=GetWorld()->GetGameState<ANRGameState>();
  if(!C||!GM||!GS)return;
  if(Stage==0&&T>3){Stage=1;C->Team=1-GS->AttackTeam;C->TeleportTo(FVector(-1450,-400,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-55,0,0));C->SetRole(ENRRole::Engineer);C->AbilityPressed();Printed=C->PrintingNode;Check(Printed.IsValid(),TEXT("GAS build request"));}
  if(Stage==1&&T>7){Stage=2;Check(Printed.IsValid()&&Printed->NodeState==ENRNodeState::Active,TEXT("two phase printing"));C->Team=GS->AttackTeam;GM->BeginCombat();C->FirePressed();}
  if(Stage==2&&T>7.5f){Stage=3;C->FireReleased();Check(C->Ammo<30&&C->Ammo>0,TEXT("authoritative weapon fire"));C->ReloadPressed();}
  if(Stage==3&&T>10)
  {
   Stage=4;Check(C->Ammo==30&&!C->bReloading,TEXT("reload completes"));
   for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(!It->bExtraction){const FVector Home=It->GetActorLocation();It->DropAt(FVector(-1600,0,40));It->ResetObjective();Check(It->GetActorLocation().Equals(Home),TEXT("objective returns home"));It->Interact(C);break;}
   Check(C->bCarryingDisk,TEXT("disk pickup"));
   for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(It->bExtraction){It->Interact(C);break;}
   Check(GS->SyndicateScore==1&&GS->Phase==ENRPhase::Resolution&&!C->bCarryingDisk,TEXT("extraction wins round"));
  }
  if(Stage==4&&T>18)
  {
   Stage=5;Check(GS->Round==2&&GS->Phase==ENRPhase::Preparation&&C->IsAlive(),TEXT("next round resets"));
   GM->BeginCombat();for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(!It->bExtraction){It->Interact(C);break;}
   UGameplayStatics::ApplyDamage(C,1000,nullptr,nullptr,nullptr);Check(C->bDead&&!C->bCarryingDisk&&GS->Phase==ENRPhase::Resolution,TEXT("death drops disk and resolves round"));
   bool Hidden=true;for(const auto& Sleeve:C->ViewSleeves)Hidden&=!Sleeve->IsVisible();for(const auto& Cuff:C->ViewCuffs)Hidden&=!Cuff->IsVisible();Check(Hidden,TEXT("viewmodel accessories hidden on death"));
   for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(!It->bExtraction){Check(!It->bTaken&&FVector::Dist(It->GetActorLocation(),C->GetActorLocation())<100,TEXT("dropped disk available"));break;}
  }
  if(Stage==5&&T>27){Stage=6;Check(C->IsAlive()&&C->Ammo==30&&GS->Round==3,TEXT("respawn after elimination"));FFileHelper::SaveStringToFile(bFailed?TEXT("FAILED"):TEXT("PASSED"),*(FPaths::ProjectSavedDir()/TEXT("MatchResult.txt")));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}
  return;
 }

 if(FParse::Param(FCommandLine::Get(),TEXT("NRNetworkMovement"))&&C&&!C->HasAuthority())
 {
  if(NetworkStage==0&&T>3){NetworkStage=1;C->SprintPressed();}
  if(NetworkStage==1&&T<4)C->AddMovementInput(FVector::ForwardVector,1);
  if(NetworkStage==1&&T>=4){NetworkStage=2;const bool OK=C->GetVelocity().Size2D()>500;bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETMOVE sprint %s speed %.1f"),OK?TEXT("PASS"):TEXT("FAIL"),C->GetVelocity().Size2D());C->SprintReleased();C->CrouchPressed();}
  if(NetworkStage==2&&T>4.6){NetworkStage=3;const bool OK=C->bIsCrouched;bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETMOVE crouch %s"),OK?TEXT("PASS"):TEXT("FAIL"));C->CrouchReleased();}
 }
 if(C&&!C->HasAuthority()&&FParse::Param(FCommandLine::Get(),TEXT("NRNetworkMovement")))
 {
  if(NetworkStage==3&&T>11.4){NetworkStage=4;C->SelectWeapon(1);}
  if(NetworkStage==4&&T>12.2){NetworkStage=5;bool OK=C->EquippedWeapon==1&&C->Ammo==12&&C->ReserveAmmo==36;bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETARSENAL pistol %s"),OK?TEXT("PASS"):TEXT("FAIL"));C->SelectWeapon(2);}
  if(NetworkStage==5&&T>13){NetworkStage=6;bool OK=C->EquippedWeapon==2&&C->Ammo==0;bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETARSENAL knife %s"),OK?TEXT("PASS"):TEXT("FAIL"));C->SelectWeapon(0);}
  if(NetworkStage==6&&T>13.8){NetworkStage=7;bool OK=C->EquippedWeapon==0&&C->Ammo==30;bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETARSENAL primary %s"),OK?TEXT("PASS"):TEXT("FAIL"));}
 }
 if(Stage==0&&T>3&&C&&!C->HasAuthority()){PC->SetControlRotation(FRotator(-55,0,0));C->AddMovementInput(FVector::ForwardVector,.01f);}
 if(Stage==0&&T>5){Stage=1;if(!C||!GetWorld()->GetGameState<ANRGameState>()){bFailed=true;UE_LOG(LogTemp,Error,TEXT("NR_SMOKE missing player or GameState"));return;}
 if(C->HasAuthority()){C->TeleportTo(FVector(-1450,-400,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-55,0,0));C->SetRole(ENRRole::Engineer);if(!C->BuildNode()){bFailed=true;UE_LOG(LogTemp,Error,TEXT("NR_SMOKE printing placement rejected"));}Printed=C->PrintingNode;}else C->AbilityPressed();
 FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/NullRoute.png"),true,false);
 }
 if(Stage==1&&T>11){Stage=2;if(C&&!C->HasAuthority()&&(!C->PrintingNode||C->PrintingNode->NodeState!=ENRNodeState::Active)){bFailed=true;UE_LOG(LogTemp,Error,TEXT("NR_SMOKE remote GAS print replication failed"));}if(C&&C->HasAuthority()){
 if(!Printed.IsValid()||Printed->NodeState!=ENRNodeState::Active){bFailed=true;UE_LOG(LogTemp,Error,TEXT("NR_SMOKE print did not complete"));}
 auto* A=GetWorld()->SpawnActor<ANodeBase>(FVector(-1200,-300,60),FRotator::ZeroRotator);auto* B=GetWorld()->SpawnActor<ANodeBase>(FVector(-1000,-300,60),FRotator::ZeroRotator);A->Initialize(1,ENRNodeOp::Sensor,true);B->Initialize(1,ENRNodeOp::Turret);A->CompletePrinting();B->CompletePrinting();auto* G=GetWorld()->GetSubsystem<UNRGraphSubsystem>();
 if(!G->Connect(A,B,1)||G->Connect(B,A,1)){bFailed=true;UE_LOG(LogTemp,Error,TEXT("NR_SMOKE graph validation failed"));}G->Cascade(A);if(B->NodeState!=ENRNodeState::Destroyed){bFailed=true;UE_LOG(LogTemp,Error,TEXT("NR_SMOKE cascade failed"));}
 int32 Structures=0;for(TActorIterator<ANRStructure> It(GetWorld());It;++It){++Structures;if(Structures==1)It->BreachServer(It->GetActorLocation(),FVector::UpVector);}if(Structures==0)bFailed=true;
 }
 }
 if(Stage==2&&T>15){Stage=3;const FString Result=bFailed?TEXT("FAILED"):TEXT("PASSED");FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("SmokeResult.txt")));UE_LOG(LogTemp,Display,TEXT("NR_SMOKE %s"),*Result);FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}
}
