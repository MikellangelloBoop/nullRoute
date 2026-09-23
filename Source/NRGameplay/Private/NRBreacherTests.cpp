#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRHUD.h"
#include "NRAttributes.h"
#include "NRPreparationZone.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/PointLight.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "TimerManager.h"

void UNRSmokeSubsystem::RunBreacherTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC||Stage!=0||T<3)return;Stage=1;
 auto Check=[this](bool OK,const TCHAR* Label){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_BREACHER %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 const auto* Settings=GetDefault<UNRFactionSettings>();
 Check(Settings->Resolve(TEXT("/Game/Maps/NR_Arcology")).Defenders==ENRFaction::Chronos,TEXT("arcology selects corporate defenders"));
 Check(Settings->Resolve(TEXT("UEDPIE_0_NR_NeoVeridian")).Defenders==ENRFaction::Police,TEXT("PIE map prefix resolves police profile"));
 Check(Settings->Resolve(TEXT("MissingMap")).Attackers==ENRFaction::Rebel,TEXT("unknown map has deterministic fallback"));
 auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Preparation;
 C->SetRole(ENRRole::Assault);const int Ammo=C->Ammo;const float Health=C->Attributes->GetHealth();
 Check(C->UsesMetaHuman()&&C->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum()>300,TEXT("assault uses exported MetaHuman DNA body skeleton"));
 Check(C->GetMesh()->GetSkeletalMeshAsset()->GetLODNum()==3,TEXT("MetaHuman body has three authored LODs"));
 Check(C->ViewArms->GetSkeletalMeshAsset()->GetName()==TEXT("SKM_BreacherHands"),TEXT("first person uses skinned MetaHuman hands"));
 Check(C->ViewArms->GetSingleNodeInstance()&&C->ViewArms->GetSingleNodeInstance()->GetCurrentAsset()->GetSkeleton()==C->ViewArms->GetSkeletalMeshAsset()->GetSkeleton(),TEXT("first-person animation targets MetaHuman skeleton"));
 for(int F=0;F<3;++F)
 {
  GS->DefenderFaction=ENRFaction(F);C->Team=1-GS->AttackTeam;C->RefreshAppearance();
  Check(C->GetFaction()==ENRFaction(F),TEXT("server side selects requested faction"));
  Check(C->GetMesh()->IsVisible()&&C->UsesMetaHuman(),TEXT("faction refresh retains continuous MetaHuman undersuit"));
  Check(C->ArmorParts.Num()==16,TEXT("bounded shared bone attachments"));
  bool Assets=true,Attached=true,Collision=true;int Triangles=0;
  for(auto& Part:C->ArmorParts)
  {
   Assets&=Part->GetStaticMesh()!=nullptr;Attached&=Part->GetAttachParent()==C->GetMesh()&&C->GetMesh()->DoesSocketExist(Part->GetAttachSocketName());
   Collision&=Part->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Part->CanEverAffectNavigation();
   if(Part->GetStaticMesh()&&Part->IsVisible())Triangles+=Part->GetStaticMesh()->GetNumTriangles(0);
  }
  Check(Assets&&Attached,TEXT("all armor meshes load on valid animated bones"));Check(Collision,TEXT("cosmetic armor preserves movement and hit collision"));
  Check(Triangles>3000&&Triangles<60000,TEXT("armor triangle budget"));UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_TRIANGLES %s %d"),NRFactions::Key(ENRFaction(F)),Triangles);
  Check(C->Weapon->GetStaticMesh()&&C->Weapon->GetStaticMesh()->GetName().Contains(NRFactions::Key(ENRFaction(F))),TEXT("faction changes first-person weapon design"));
  C->WeaponSkin=2;C->OnRep_Skin();Check(C->Weapon->GetNumOverrideMaterials()==0,TEXT("weapon skin preserves faction material slots"));
 }
 Check(C->Ammo==Ammo&&C->Attributes->GetHealth()==Health,TEXT("appearance changes preserve combat state"));
 GS->DefenderFaction=ENRFaction::Chronos;GS->AttackerFaction=ENRFaction::Rebel;C->Team=0;GS->AttackTeam=1;C->RefreshAppearance();Check(C->GetFaction()==ENRFaction::Chronos,TEXT("defending team uses Chronos"));
 GS->AttackTeam=0;C->RefreshAppearance();Check(C->GetFaction()==ENRFaction::Rebel,TEXT("halftime changes team faction"));
 C->SetRole(ENRRole::Engineer);Check(C->ArmorParts[0]->IsVisible()&&C->ArmorParts[0]->GetStaticMesh()->GetName().Contains(TEXT("Engineer")),TEXT("class swap installs engineer geometry"));
 Check(C->UsesMetaHuman()&&C->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SKM_BreacherBody"),TEXT("class swap preserves MetaHuman skeleton"));
 C->SetRole(ENRRole::Assault);C->bDead=true;C->OnRep_Dead();bool Hidden=true;for(auto& Part:C->ArmorParts)Hidden&=!Part->IsVisible();Check(Hidden,TEXT("death hides armor"));
 C->ResetForRound();Check(C->ArmorParts[0]->IsVisible(),TEXT("round reset restores armor"));
 C->LastEquip=-100;C->ServerEquip(1);Check(C->Weapon->GetStaticMesh()->GetName()==TEXT("SM_Pistol"),TEXT("pistol survives faction refresh"));C->RefreshAppearance();Check(C->Weapon->GetStaticMesh()->GetName()==TEXT("SM_Pistol"),TEXT("appearance does not replace equipped secondary"));
 UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
}

void UNRSmokeSubsystem::RunBreacherVisual(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* GS=GetWorld()->GetGameState<ANRGameState>();
 if(Stage==0&&T>3)
 {
  Stage=1;if(auto* H=Cast<ANRHUD>(PC->GetHUD()))if(H->IsMenuOpen())H->ToggleMenu();PC->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
  GetWorld()->GetTimerManager().ClearTimer(GetWorld()->GetAuthGameMode<ANRGameMode>()->PhaseTimer);GS->Phase=ENRPhase::Preparation;
  // Keep the presentation camera clear of the preparation gate's translucent screen.
  for(TActorIterator<ANRPreparationZone> It(GetWorld());It;++It)It->SetActorHiddenInGame(true);
  C->TeleportTo({-1600,700,88},FRotator::ZeroRotator);C->SetRole(ENRRole::Assault);C->GetCharacterMovement()->StopMovementImmediately();C->Team=1-GS->AttackTeam;GS->DefenderFaction=ENRFaction::Chronos;C->RefreshAppearance();
  auto* Display=GetWorld()->SpawnActor<ANRCharacter>(FVector(-1850,700,88),FRotator::ZeroRotator);Display->Team=C->Team;Display->SetRole(ENRRole::Assault);Display->Tags.Add(TEXT("BreacherDisplay"));
  Display->SetActorTickEnabled(false);Display->GetMesh()->PlayAnimation(Display->AnimationForBody(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"))),true);Display->WorldWeapon->SetVisibility(false);
  C->SetActorHiddenInGame(true);
  auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->Tags.Add(TEXT("BreacherCamera"));Cam->SetActorLocation(Display->GetActorLocation()+FVector(390,150,35));Cam->SetActorRotation((Display->GetActorLocation()+FVector(0,0,3)-Cam->GetActorLocation()).Rotation());Cam->GetCameraComponent()->SetFieldOfView(32);Cam->GetCameraComponent()->SetConstraintAspectRatio(false);Cam->GetCameraComponent()->bOverrideAspectRatioAxisConstraint=true;Cam->GetCameraComponent()->SetAspectRatioAxisConstraint(AspectRatio_MaintainXFOV);PC->SetViewTarget(Cam);
  for(int I=0;I<3;++I){auto* Light=GetWorld()->SpawnActor<APointLight>();Light->PointLightComponent->SetMobility(EComponentMobility::Movable);Light->SetActorLocation(Display->GetActorLocation()+FVector(I==2?-140:180,I==0?140:-180,150));Light->PointLightComponent->SetIntensity(I==0?18000:12000);Light->PointLightComponent->SetAttenuationRadius(850);Light->PointLightComponent->SetLightColor(I==2?FLinearColor(.05,.5,1):I==0?FLinearColor(1,.88,.68):FLinearColor(.5,.75,1));}
 }
 for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("BreacherDisplay"))&&It->CachedFaction!=It->GetFaction()){It->RefreshAppearance();It->WorldWeapon->SetVisibility(false);}
 if(Stage==1&&T>9){Stage=2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Breacher_Chronos.png"),false,false);}
 if(Stage==2&&T>10){Stage=3;GS->DefenderFaction=ENRFaction::Police;}
 if(Stage==3&&T>15){Stage=4;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Breacher_Police.png"),false,false);}
 if(Stage==4&&T>16){Stage=5;GS->DefenderFaction=ENRFaction::Rebel;}
 if(Stage==5&&T>21){Stage=6;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Breacher_Rebel.png"),false,false);}
 if(Stage==6&&T>22){Stage=7;C->SetActorHiddenInGame(false);PC->SetViewTarget(C);PC->SetControlRotation(FRotator(0,0,0));GS->DefenderFaction=ENRFaction::Chronos;C->RefreshAppearance();}
 if(Stage==7&&T>27){Stage=8;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Breacher_FirstPerson.png"),true,false);}
 if(Stage==8&&T>28){Stage=9;C->ServerAim(true);}
 if(Stage==9&&T>30){Stage=10;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Breacher_ADS.png"),true,false);}
 if(Stage==10&&T>31){Stage=11;C->ServerAim(false);C->Ammo=FMath::Max(0,C->Ammo-1);C->ServerReload();}
 if(Stage==11&&T>31.6f){Stage=12;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Breacher_Reload.png"),true,false);}
 if(Stage==12&&T>35){UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0);}
}

void UNRSmokeSubsystem::RunBreacherNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS)return;
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)
 {
  if(Stage==0)
  {
   int Players=0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller)++Players;
   if(Players<2)return;Stage=1;OperationsEpoch=T;
   GetWorld()->GetTimerManager().ClearTimer(GetWorld()->GetAuthGameMode<ANRGameMode>()->PhaseTimer);GS->Phase=ENRPhase::Preparation;GS->AttackTeam=1;
   int I=0;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It){It->Team=I++%2;It->SetRole(ENRRole::Assault);It->ForceNetUpdate();if(It->bVisualsReady||!It->ArmorParts.IsEmpty())bFailed=true;}
   UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_NET_SERVER cosmetic-free dedicated server : %s"),bFailed?TEXT("FAIL"):TEXT("PASS"));
  }
  if(Stage==1&&T-OperationsEpoch>7){Stage=2;GS->DefenderFaction=ENRFaction::Police;GS->ForceNetUpdate();}
  if(Stage==2&&T-OperationsEpoch>14){Stage=3;GS->AttackTeam=0;GS->ForceNetUpdate();}
  return;
 }
 if(!C||!PC||C->HasAuthority())return;
 int Count=0;bool Match=true;
 for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->OperatorRole==ENRRole::Assault&&It->ArmorParts.Num()==16)
 {
  ++Count;UStaticMesh* Mesh=It->ArmorParts[0]->GetStaticMesh();if(!Mesh||!Mesh->GetName().Contains(NRFactions::Key(It->GetFaction())))Match=false;
  if(Mesh&&Mesh->GetName().Contains(NRFactions::Key(It->GetFaction())))NetworkStage|=1<<uint8(It->GetFaction());
 }
 if(Count==2&&Match&&NetworkStage==7&&GS->AttackTeam==0){UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_NET_CLIENT PASSED: both characters, all three factions, side swap"));FPlatformMisc::RequestExitWithStatus(false,0);}
 else if(T>50){UE_LOG(LogTemp,Error,TEXT("NR_BREACHER_NET_CLIENT FAILED mask=%d count=%d"),NetworkStage,Count);FPlatformMisc::RequestExitWithStatus(false,1);}
}
