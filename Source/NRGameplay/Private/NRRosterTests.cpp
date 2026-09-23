#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRHUD.h"
#include "NRAttributes.h"
#include "NRAbilities.h"
#include "NRPreparationZone.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/PointLight.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace { const TCHAR* RoleKeys[]={TEXT("Engineer"),TEXT("Scout"),TEXT("Breacher"),TEXT("Medic")}; }
void UNRSmokeSubsystem::RunRosterTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC||T<3||Stage)return;Stage=1;
 auto Check=[this](bool OK,const TCHAR* Label){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_ROSTER %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto* GS=GetWorld()->GetGameState<ANRGameState>();auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);GS->Phase=ENRPhase::Preparation;
 for(int Role=0;Role<4;++Role)for(int F=0;F<5;++F)
 {
  C->ServerSelectClass(ENRRole(Role));GS->DefenderFaction=ENRFaction(F);C->Team=1-GS->AttackTeam;C->RefreshAppearance();
  bool Valid=C->ArmorParts.Num()==16;int Triangles=0;
  for(auto& Part:C->ArmorParts)
  {
   UStaticMesh* Mesh=Part->GetStaticMesh();Valid&=Mesh&&Mesh->GetName().Contains(FString(RoleKeys[Role])+TEXT("_")+NRFactions::Key(ENRFaction(F)));
   Valid&=Part->GetAttachParent()==C->GetMesh()&&C->GetMesh()->DoesSocketExist(Part->GetAttachSocketName())&&Part->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Part->CanEverAffectNavigation();
   if(Mesh&&Part->IsVisible())Triangles+=Mesh->GetNumTriangles(0);
  }
  const FString Label=FString::Printf(TEXT("%s / %s assets, bones, collision and geometry"),RoleKeys[Role],NRFactions::Key(ENRFaction(F)));
  Check(Valid&&Triangles>3000&&Triangles<70000,*Label);
  Check(C->UsesMetaHuman()&&C->OperatorRole==ENRRole(Role)&&C->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum()>300,TEXT("class selection retains genuine MetaHuman body"));
  Check(C->ViewArms->GetSingleNodeInstance()&&C->ViewArms->GetSingleNodeInstance()->GetCurrentAsset()->GetSkeleton()==C->ViewArms->GetSkeletalMeshAsset()->GetSkeleton(),TEXT("hands animate on matching skeleton"));
  bool Sleeves=true;for(auto& Sleeve:C->ViewSleeves)Sleeves&=Sleeve->GetStaticMesh()&&Sleeve->GetStaticMesh()->GetName().Contains(NRFactions::Key(ENRFaction(F)));Check(Sleeves,TEXT("first-person sleeves follow faction"));
  const int Ammo=C->Ammo;const float Health=C->Attributes->GetHealth();C->RefreshAppearance();Check(C->Ammo==Ammo&&C->Attributes->GetHealth()==Health,TEXT("cosmetic refresh preserves gameplay state"));
  for(int Slot=1;Slot<=2;++Slot){C->LastEquip=-100;C->ServerEquip(Slot);UStaticMesh* Mesh=C->Weapon->GetStaticMesh();C->RefreshAppearance();Check(C->Weapon->GetStaticMesh()==Mesh&&Mesh,TEXT("secondary survives appearance refresh"));}
  C->LastEquip=-100;C->ServerEquip(0);
 }
 auto Pair=NRFactions::WithOptions(FNRMapFactions(),TEXT("?AttackerFaction=RustHounds?DefenderFaction=Ascended"));Check(Pair.Attackers==ENRFaction::RustHounds&&Pair.Defenders==ENRFaction::Ascended,TEXT("server map options resolve new factions"));
 Pair=NRFactions::WithOptions(FNRMapFactions(),TEXT("?AttackerFaction=Chronos?DefenderFaction=Chronos"));Check(Pair.Attackers!=Pair.Defenders,TEXT("identical factions rejected"));
 Pair=NRFactions::WithOptions(FNRMapFactions(),TEXT("?AttackerFaction=invalid?DefenderFaction=invalid"));Check(Pair.Attackers==ENRFaction::Rebel&&Pair.Defenders==ENRFaction::Chronos,TEXT("invalid options retain map defaults"));
 C->ServerSelectClass(ENRRole(255));Check(C->OperatorRole==ENRRole::Medic,TEXT("invalid class rejected"));
 GS->AttackerFaction=ENRFaction::RustHounds;GS->DefenderFaction=ENRFaction::Ascended;C->Team=0;GS->AttackTeam=1;C->RefreshAppearance();const auto Before=C->ArmorParts[0]->GetStaticMesh();GS->AttackTeam=0;C->RefreshAppearance();Check(C->GetFaction()==ENRFaction::RustHounds&&C->ArmorParts[0]->GetStaticMesh()!=Before,TEXT("halftime changes design as well as badge"));
 C->bDead=true;C->OnRep_Dead();bool Hidden=true;for(auto& Part:C->ArmorParts)Hidden&=!Part->IsVisible();Check(Hidden,TEXT("death hides all medic armor"));C->ResetForRound();Check(C->ArmorParts[0]->IsVisible(),TEXT("round reset restores medic"));
 C->SetActorLocation({0,0,5000});C->GetCharacterMovement()->DisableMovement();PC->SetControlRotation(FRotator::ZeroRotator);C->Team=0;C->Attributes->SetHealth(40);C->AbilityReadyTime=0;
 Check(!C->TreatWounded(),TEXT("treatment blocked during preparation"));GS->Phase=ENRPhase::Combat;C->LastRequest=-100;C->ServerAbility();Check(C->Attributes->GetHealth()==80&&C->AbilityReadyTime>T,TEXT("GAS medic activation heals self and starts cooldown"));
 Check(!C->TreatWounded()&&C->Attributes->GetHealth()==80,TEXT("cooldown blocks repeated healing"));
 C->AbilityReadyTime=0;Check(C->TreatWounded()&&C->Attributes->GetHealth()==100,TEXT("healing clamps to maximum health"));C->AbilityReadyTime=0;Check(!C->TreatWounded()&&C->AbilityReadyTime==0,TEXT("full health consumes no cooldown"));
 auto* Ally=GetWorld()->SpawnActor<ANRCharacter>(FVector(300,0,5000),FRotator::ZeroRotator);Ally->Team=0;Ally->GetCharacterMovement()->DisableMovement();Ally->Attributes->SetHealth(30);
 Check(C->TreatWounded()&&Ally->Attributes->GetHealth()==70&&C->Attributes->GetHealth()==100,TEXT("aimed friendly receives treatment"));
 C->AbilityReadyTime=0;Ally->Team=1;Check(!C->TreatWounded()&&Ally->Attributes->GetHealth()==70,TEXT("enemy cannot receive treatment"));Ally->Team=0;
 auto* Wall=GetWorld()->SpawnActor<AActor>();auto* Block=NewObject<UStaticMeshComponent>(Wall);Wall->SetRootComponent(Block);Block->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube")));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Block->SetCollisionResponseToAllChannels(ECR_Block);Block->RegisterComponent();Wall->SetActorLocation({150,0,5050});Wall->SetActorScale3D({.1,2,3});
 Check(!C->TreatWounded()&&Ally->Attributes->GetHealth()==70,TEXT("wall blocks treatment of ally"));Wall->Destroy();Ally->SetActorLocation({900,0,5000});Check(!C->TreatWounded()&&Ally->Attributes->GetHealth()==70,TEXT("range limits treatment"));
 C->SetRole(ENRRole::Engineer);C->Attributes->SetHealth(40);Check(!C->TreatWounded(),TEXT("other classes cannot heal"));C->SetRole(ENRRole::Medic);C->bDead=true;Check(!C->TreatWounded(),TEXT("dead medic cannot heal"));C->bDead=false;
 GS->Phase=ENRPhase::Combat;C->ServerSelectClass(ENRRole::Scout);Check(C->OperatorRole==ENRRole::Medic,TEXT("combat cannot change class"));
 UE_LOG(LogTemp,Display,TEXT("NR_ROSTER_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
}

void UNRSmokeSubsystem::RunRosterVisual(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* GS=GetWorld()->GetGameState<ANRGameState>();
 if(Stage==0&&T>3)
 {
  Stage=1;OperationsEpoch=T;auto* HUD=Cast<ANRHUD>(PC->GetHUD());if(HUD&&HUD->IsMenuOpen())HUD->ToggleMenu();PC->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
  GetWorld()->GetTimerManager().ClearTimer(GetWorld()->GetAuthGameMode<ANRGameMode>()->PhaseTimer);GS->Phase=ENRPhase::Preparation;GS->DefenderFaction=ENRFaction::Chronos;
  for(TActorIterator<ANRPreparationZone> It(GetWorld());It;++It)It->SetActorHiddenInGame(true);
  C->SetActorHiddenInGame(true);C->TeleportTo({-1600,700,88},FRotator::ZeroRotator);
  for(int Role=0;Role<4;++Role)
  {
   auto* Display=GetWorld()->SpawnActor<ANRCharacter>(FVector(-1850,795-Role*115,88),FRotator::ZeroRotator);Display->Team=1-GS->AttackTeam;Display->SetRole(ENRRole(Role));Display->Tags.Add(TEXT("RosterDisplay"));Display->SetActorTickEnabled(false);
   Display->GetMesh()->PlayAnimation(Display->AnimationForBody(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"))),true);Display->WorldWeapon->SetVisibility(false);
   TArray<UStaticMeshComponent*> Parts;Display->GetComponents(Parts);for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("RifleDetail")))Part->SetVisibility(false);
  }
  const FVector Center(-1850,622.5,91);auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->SetActorLocation(Center+FVector(590,0,32));Cam->SetActorRotation((Center-Cam->GetActorLocation()).Rotation());Cam->GetCameraComponent()->SetFieldOfView(48);Cam->GetCameraComponent()->SetConstraintAspectRatio(false);Cam->GetCameraComponent()->bOverrideAspectRatioAxisConstraint=true;Cam->GetCameraComponent()->SetAspectRatioAxisConstraint(AspectRatio_MaintainXFOV);PC->SetViewTarget(Cam);
  for(int I=0;I<4;++I){auto* Light=GetWorld()->SpawnActor<APointLight>();Light->PointLightComponent->SetMobility(EComponentMobility::Movable);Light->SetActorLocation(Center+FVector(I==3?-150:180,-220+I*145,170));Light->PointLightComponent->SetIntensity(17000);Light->PointLightComponent->SetAttenuationRadius(1000);Light->PointLightComponent->SetLightColor(I==3?FLinearColor(.2,.5,1):FLinearColor(.88,.93,1));}
 }
 if(Stage==1&&FParse::Param(FCommandLine::Get(),TEXT("NRRosterMenuOnly"))){C->SetRole(ENRRole::Medic);Stage=13;OperationsEpoch=T-3;}
 if(Stage>=1&&Stage<=10&&T-OperationsEpoch>6)
 {
  OperationsEpoch=T;
  if(Stage%2==1){const int F=(Stage-1)/2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Roster_%s.png"),NRFactions::Key(ENRFaction(F))),false,false);++Stage;}
  else
  {
   const int F=Stage/2;++Stage;if(F<5){GS->DefenderFaction=ENRFaction(F);for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("RosterDisplay"))){It->RefreshAppearance();It->WorldWeapon->SetVisibility(false);TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("RifleDetail")))Part->SetVisibility(false);}}
  }
 }
 if(Stage==11&&T-OperationsEpoch>2){Stage=12;C->SetActorHiddenInGame(false);C->Team=1-GS->AttackTeam;C->SetRole(ENRRole::Medic);GS->DefenderFaction=ENRFaction::RustHounds;C->RefreshAppearance();PC->SetViewTarget(C);PC->SetControlRotation(FRotator(0,0,0));OperationsEpoch=T;}
 if(Stage==12&&T-OperationsEpoch>5){Stage=13;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Roster_MedicFP.png"),true,false);OperationsEpoch=T;}
 if(Stage==13&&T-OperationsEpoch>2){Stage=14;PC->ConsoleCommand(TEXT("r.SetRes 1600x900w"));if(auto* H=Cast<ANRHUD>(PC->GetHUD()))H->ToggleMenu();UGameplayStatics::SetGamePaused(this,false);OperationsEpoch=T;}
 if(Stage==14&&T-OperationsEpoch>3){Stage=15;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Roster_Menu.png"),true,false);OperationsEpoch=T;}
 if(Stage==15&&T-OperationsEpoch>2){UE_LOG(LogTemp,Display,TEXT("NR_ROSTER_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0);}
}

void UNRSmokeSubsystem::RunRosterNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS)return;
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)
 {
  if(Stage==0){int Count=0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller)++Count;if(Count<2)return;GetWorld()->GetTimerManager().ClearTimer(GetWorld()->GetAuthGameMode<ANRGameMode>()->PhaseTimer);GS->Phase=ENRPhase::Combat;int I=0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller){It->Team=I++;It->SetRole(ENRRole::Medic);It->SetActorLocation({-2300,1900+I*500.,3000});It->GetCharacterMovement()->DisableMovement();It->Attributes->SetHealth(40);It->AbilityReadyTime=0;It->ForceNetUpdate();bFailed|=It->bVisualsReady||!It->ArmorParts.IsEmpty();}Stage=1;OperationsEpoch=T;GS->ForceNetUpdate();}
  if(Stage==1){bool Healed=true;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller)Healed&=It->Attributes->GetHealth()==80&&It->AbilityReadyTime>0;if(!Healed){if(T-OperationsEpoch>25){UE_LOG(LogTemp,Error,TEXT("NR_ROSTER_NET_SERVER treatment RPC failed"));FPlatformMisc::RequestExitWithStatus(false,1);}return;}UE_LOG(LogTemp,Display,TEXT("NR_ROSTER_NET_SERVER treatment RPC and replicated health PASS"));UE_LOG(LogTemp,Display,TEXT("NR_ROSTER_NET_SERVER cosmetics-free %s"),bFailed?TEXT("FAIL"):TEXT("PASS"));if(bFailed){FPlatformMisc::RequestExitWithStatus(false,1);return;}Stage=2;OperationsEpoch=T;}
  if(Stage>=2&&Stage<22&&T-OperationsEpoch>3){const int Step=Stage-2,F=Step/4,Role=Step%4;GS->DefenderFaction=ENRFaction(F);GS->AttackerFaction=ENRFaction((F+1)%5);GS->AttackTeam=Step<10?1:0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller){It->SetRole(ENRRole(Role));It->ForceNetUpdate();}GS->ForceNetUpdate();OperationsEpoch=T;++Stage;}
  if(Stage==22&&T-OperationsEpoch>6){UE_LOG(LogTemp,Display,TEXT("NR_ROSTER_NET_SERVER cosmetics-free %s"),bFailed?TEXT("FAIL"):TEXT("PASS"));Stage=23;}
  return;
 }
 if(!C||!PC||C->HasAuthority())return;
 if(C->OperatorRole==ENRRole::Medic&&C->Attributes->GetHealth()==40&&!(NetworkStage&(1<<21))){if(auto* H=Cast<ANRHUD>(PC->GetHUD()))if(H->IsMenuOpen())H->ToggleMenu();C->AbilityPressed();NetworkStage|=1<<21;}
 if(C->Attributes->GetHealth()==80&&C->AbilityReadyTime>0)NetworkStage|=1<<20;
 int Count=0;bool Match=true;
 for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ArmorParts.Num()==16){++Count;UStaticMesh* Mesh=It->ArmorParts[0]->GetStaticMesh();const int Role=uint8(It->OperatorRole),F=uint8(It->GetFaction());const bool OK=Role<4&&F<5&&It->UsesMetaHuman()&&Mesh&&Mesh->GetName().Contains(FString(RoleKeys[Role])+TEXT("_")+NRFactions::Key(It->GetFaction()));Match&=OK;if(OK)NetworkStage|=1<<(F*4+Role);}
 if(Count==2&&Match&&(NetworkStage&0x1fffff)==0x1fffff&&GS->AttackTeam==0){UE_LOG(LogTemp,Display,TEXT("NR_ROSTER_NET_CLIENT PASSED all 20 appearances, team swap, medic RPC"));FPlatformMisc::RequestExitWithStatus(false,0);}
 if(T>140){UE_LOG(LogTemp,Error,TEXT("NR_ROSTER_NET_CLIENT FAILED mask=%x count=%d"),NetworkStage,Count);FPlatformMisc::RequestExitWithStatus(false,1);}
}
