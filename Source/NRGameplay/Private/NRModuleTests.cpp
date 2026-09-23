#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRAttributes.h"
#include "NRHUD.h"
#include "NRBallisticsSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UNRSmokeSubsystem::RunModuleTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS)return;
 auto Check=[this](bool OK,const TCHAR* Label){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_MODULE %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(Stage==0&&T>3)
 {
  Stage=1;C->SetRole(ENRRole::Engineer);C->bModulesRestored=true;
  Check(C->MagazineCapacity()==30&&FMath::IsNearlyEqual(C->WeaponStats(0).ReloadSeconds,1.6f),TEXT("stock weapon unchanged"));
  FNRWeaponAssembly B;B.Optic=ENROptic::Combat2x;C->ServerConfigureWeapon(1,B);
  Check(C->PistolAssembly==FNRWeaponAssembly(),TEXT("pistol rejects incompatible optic"));
  B.Optic=ENROptic(255);C->ServerConfigureWeapon(0,B);C->ServerConfigureWeapon(255,{});
  Check(C->PrimaryAssembly==FNRWeaponAssembly()&&C->Ammo==30,TEXT("malformed ids rejected"));
  B.Optic=ENROptic::Reflex;B.Muzzle=ENRMuzzle::Compensator;B.Magazine=ENRMagazine::Extended;C->ServerConfigureWeapon(0,B);
  Check(C->PrimaryAssembly==B&&C->MagazineCapacity()==45,TEXT("server installs extended primary"));
  Check(C->Ammo==30&&C->ReserveAmmo==90,TEXT("install does not create ammo"));
  Check(FMath::IsNearlyEqual(C->WeaponStats(0).RecoilScale,.7f)&&C->WeaponStats(0).HipSpreadScale>1,TEXT("compensator tradeoff applied"));
  Check(C->WeaponStats(0).AimFOV==66&&C->WeaponStats(0).AimSpread<.16f,TEXT("optic changes aim"));
  C->ServerConfigureWeapon(0,{});Check(C->PrimaryAssembly==B,TEXT("rapid requests rejected"));
  bool Meshes=C->ModuleVisuals.Num()==6;for(const auto& V:C->ModuleVisuals)Meshes&=V&&V->GetStaticMesh()&&V->IsVisible()&&V->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
  Check(Meshes,TEXT("local and world attachments constructed without collision"));
 }
 if(Stage==1&&T>3.6){Stage=2;C->ServerReload();Check(C->bReloading,TEXT("extended reload starts"));}
 if(Stage==2&&T>5.65)
 {
  Stage=3;Check(!C->bReloading&&C->Ammo==45&&C->ReserveAmmo==75,TEXT("extended reload transfers finite ammo"));
  FNRWeaponAssembly B;B.Magazine=ENRMagazine::Quick;C->ServerConfigureWeapon(0,B);
  Check(C->Ammo==24&&C->ReserveAmmo==96,TEXT("smaller magazine returns surplus without loss"));
  C->AwardSupply();Check(C->ReserveAmmo==96,TEXT("supply preserves surplus reserve"));
  B.Optic=ENROptic::Reflex;B.Muzzle=ENRMuzzle::Suppressor;C->ServerConfigureWeapon(1,B);
  Check(C->PistolAmmo==10&&C->PistolReserve==38,TEXT("inactive weapon bank updated independently"));
  Check(FMath::IsNearlyEqual(C->WeaponStats(1).Damage,30.6f,.001f)&&C->WeaponStats(1).Speed==40500,TEXT("suppressor ballistic tradeoff"));
 }
 if(Stage==3&&T>6.1)
 {
  Stage=4;C->ServerEquip(1);Check(C->Ammo==10&&C->ReserveAmmo==38&&C->MagazineCapacity()==10,TEXT("switch restores configured pistol"));
 }
 if(Stage==4&&T>6.6)
 {
  Stage=5;C->Ammo=5;C->ServerReload();Check(C->bReloading,TEXT("quick reload begins"));
  C->ServerConfigureWeapon(1,{});Check(!C->bReloading&&C->Ammo==5&&C->ReserveAmmo==38,TEXT("module change cancels pending reload without refill"));
  GS->Phase=ENRPhase::Combat;C->ServerConfigureWeapon(0,{});Check(C->PrimaryAssembly.Magazine==ENRMagazine::Quick,TEXT("combat rejects module changes"));
 }
 if(Stage==5&&T>8.1)
 {
  Stage=6;Check(C->Ammo==5&&C->ReserveAmmo==38,TEXT("cancelled reload cannot complete later"));
  GS->Phase=ENRPhase::Preparation;C->bDead=true;C->ServerConfigureWeapon(0,{});Check(C->PrimaryAssembly.Magazine==ENRMagazine::Quick,TEXT("dead player cannot customize"));C->bDead=false;
  C->ResetForRound();Check(C->PrimaryAssembly.Magazine==ENRMagazine::Quick&&C->Ammo==24&&C->ReserveAmmo==90,TEXT("round reset keeps build and refills correct capacity"));
  C->SetRole(ENRRole::Scout);Check(C->Ammo==10&&C->MagazineCapacity()==10,TEXT("class change resolves compatible capacity"));
  C->LastEquip=-100;C->ServerEquip(2);bool Hidden=true;for(const auto& V:C->ModuleVisuals)Hidden&=!V->IsVisible();Check(Hidden&&C->MagazineCapacity()==0,TEXT("knife hides firearm modules"));
  // Module statistics drive the live projectile subsystem, not just the menu.
  FNRWeaponAssembly Suppressed;Suppressed.Muzzle=ENRMuzzle::Suppressor;C->ServerConfigureWeapon(1,Suppressed);
  C->LastEquip=-100;C->ServerEquip(1);C->WeaponReadyAt=0;GS->Phase=ENRPhase::Combat;C->LastShot=-100;
  auto* Ballistics=GetWorld()->GetSubsystem<UNRBallisticsSubsystem>();Ballistics->Bullets.Reset();C->FireShot();
  Check(Ballistics->Bullets.Num()==1&&FMath::IsNearlyEqual(Ballistics->Bullets.Last().Damage,30.6f,.001f)&&FMath::IsNearlyEqual(float(Ballistics->Bullets.Last().Velocity.Size()),40500.f,.1f),TEXT("configured fire creates authoritative projectile"));
  UE_LOG(LogTemp,Display,TEXT("NR_MODULE_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
 }
}

void UNRSmokeSubsystem::RunModuleNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 // Test-only preparation barrier: editor clients may take longer than a normal round's 20 seconds to start.
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)
 {
  if(Stage==0&&T>1)if(auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>())
  {Stage=1;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Preparation;GS->PhaseEnd=GS->GetServerWorldTimeSeconds()+90;GS->ForceNetUpdate();}
  if(Stage==1&&C&&C->PrimaryAssembly.Magazine==ENRMagazine::Extended){Stage=2;UE_LOG(LogTemp,Display,TEXT("NR_NETMODULE_SERVER cosmetics absent : %s"),C->ModuleVisuals.IsEmpty()?TEXT("PASS"):TEXT("FAIL"));}
  return;
 }
 if(!C||!PC||C->HasAuthority())return;
 auto Check=[this](bool OK,const TCHAR* Label){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETMODULE %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(FParse::Param(FCommandLine::Get(),TEXT("NRModuleObserver")))
 {
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(*It!=C&&It->PrimaryAssembly.Magazine==ENRMagazine::Extended&&It->PistolAssembly.Muzzle==ENRMuzzle::Suppressor)
  {
   const bool Ready=It->ModuleVisuals.Num()==6&&It->ModuleVisuals[3]->IsVisible()&&It->ModuleVisuals[3]->GetStaticMesh();
   if(!Ready)continue;Check(Ready,TEXT("other player world attachments replicate"));
   UE_LOG(LogTemp,Display,TEXT("NR_NETMODULE_OBSERVER PASSED"));FPlatformMisc::RequestExitWithStatus(false,0);return;
  }
  if(T>14){Check(false,TEXT("other player world attachments timed out"));FPlatformMisc::RequestExitWithStatus(false,1);}return;
 }
 if(Stage==0&&T>3){Stage=1;FNRWeaponAssembly B;B.Optic=ENROptic::Reflex;B.Muzzle=ENRMuzzle::Compensator;B.Magazine=ENRMagazine::Extended;C->ConfigureWeapon(0,B);}
 if(Stage==1&&T>4){Stage=2;Check(C->PrimaryAssembly.Magazine==ENRMagazine::Extended&&C->PrimaryAssembly.Optic==ENROptic::Reflex&&C->MagazineCapacity()==45&&C->Ammo==30,TEXT("primary selection replicates from server"));C->ServerReload();}
 if(Stage==2&&T>6.5){Stage=3;Check(C->Ammo==45&&C->ReserveAmmo==75&&!C->bReloading,TEXT("configured reload replicates ammo"));FNRWeaponAssembly B;B.Optic=ENROptic::Reflex;B.Muzzle=ENRMuzzle::Suppressor;B.Magazine=ENRMagazine::Quick;C->ConfigureWeapon(1,B);}
 if(Stage==3&&T>7.5){Stage=4;Check(C->PistolAssembly.Magazine==ENRMagazine::Quick&&C->PistolAssembly.Muzzle==ENRMuzzle::Suppressor,TEXT("independent pistol config replicates"));C->SelectWeapon(1);}
 if(Stage==4&&T>8.5){Stage=5;Check(C->EquippedWeapon==1&&C->Ammo==10&&C->ReserveAmmo==38,TEXT("pistol bank conserves ammo on remote client"));FNRWeaponAssembly Invalid;Invalid.Optic=ENROptic::Combat2x;C->ServerConfigureWeapon(1,Invalid);}
 if(Stage==5&&T>9.5){Stage=6;Check(C->PistolAssembly.Optic==ENROptic::Reflex,TEXT("incompatible remote request rejected"));C->SelectWeapon(2);}
 if(Stage==6&&T>10.5){Stage=7;bool Hidden=true;for(const auto& V:C->ModuleVisuals)Hidden&=!V->IsVisible();Check(C->EquippedWeapon==2&&Hidden,TEXT("knife hides remote cosmetics"));UE_LOG(LogTemp,Display,TEXT("NR_NETMODULE_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}
}
