#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRHUD.h"
#include "NRCombatFX.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

FNRWeaponStats ANRCharacter::WeaponStats(uint8 Slot)const{return NRWeaponModules::Resolve(OperatorRole,Slot,Assembly(Slot),PrimaryWeapon);}
bool ANRCharacter::CanCustomize()const
{
 const auto* GS=GetWorld()->GetGameState<ANRGameState>();return IsAlive()&&GS&&GS->Phase==ENRPhase::Preparation;
}
void ANRCharacter::ToggleWorkbench()
{
 FireReleased();AimReleased();SprintReleased();StopJumping();ResetTacticalStance();RestoreWeaponModules();
 if(auto* PC=Cast<APlayerController>(Controller))if(auto* H=Cast<ANRHUD>(PC->GetHUD()))H->ToggleWorkbench();
}
void ANRCharacter::ConfigureWeapon(uint8 Slot,FNRWeaponAssembly Build)
{
 if(Slot>1)return;WorkshopStatus=TEXT("Ожидание подтверждения…");ServerConfigureWeapon(Slot,Build);
}
void ANRCharacter::ServerConfigureWeapon_Implementation(uint8 Slot,FNRWeaponAssembly Build)
{
 const double Now=GetWorld()->GetRealTimeSeconds();
 if(Slot>1||!Build.IsValid(Slot)||!CanCustomize()){ClientModuleResult(Slot,{},false);return;}
 if(Now-LastModuleRequest[Slot]<.2){ClientModuleResult(Slot,Assembly(Slot),false);return;}
 LastModuleRequest[Slot]=Now;
 auto& Current=Slot==1?PistolAssembly:PrimaryAssembly;
 if(!(Current==Build))
 {
  // Cancel delayed actions before changing capacity. Never refill on a module request.
  GetWorldTimerManager().ClearTimer(FireTimer);GetWorldTimerManager().ClearTimer(ReloadTimer);GetWorldTimerManager().ClearTimer(MeleeTimer);
  bReloading=false;bAiming=false;Current=Build;
  int32& Mag=Slot==EquippedWeapon?Ammo:Slot==0?PrimaryAmmo:PistolAmmo;
  int32& Reserve=Slot==EquippedWeapon?ReserveAmmo:Slot==0?PrimaryReserve:PistolReserve;
  const int32 Excess=FMath::Max(0,Mag-WeaponStats(Slot).Capacity);Mag-=Excess;Reserve+=Excess;
  // Excess ammo is kept even above the supply cap; supplies never remove it.
  WeaponReadyAt=GetWorld()->GetTimeSeconds()+.35f;OnRep_Modules();ForceNetUpdate();
 }
 ClientModuleResult(Slot,Build,true);
}
void ANRCharacter::ClientModuleResult_Implementation(uint8 Slot,FNRWeaponAssembly Build,bool Accepted)
{
 WorkshopStatus=Accepted?TEXT("Комплект установлен. Патроны сохранены."):TEXT("Замена недоступна: дождитесь подготовки и повторите.");
 if(!Accepted||Slot>1||!Build.IsValid(Slot))return;
 // The accepted RPC persists only the validated selection, never client-supplied stats.
 if(!FString(FCommandLine::Get()).Contains(TEXT("-NR")))
 {
  const FString Key=Slot==0?TEXT("Primary"):TEXT("Pistol");
  const int32 Packed=int32(Build.Optic)+int32(Build.Muzzle)*4+int32(Build.Magazine)*16;
  GConfig->SetInt(TEXT("NullRoute.WeaponModules"),*Key,Packed,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
 }
 if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_Equip"),GetActorLocation(),.35f,true);
}
void ANRCharacter::RestoreWeaponModules()
{
 if(bModulesRestored||!IsLocallyControlled()||!CanCustomize())return;bModulesRestored=true;
 if(FString(FCommandLine::Get()).Contains(TEXT("-NR")))return;
 for(uint8 Slot=0;Slot<2;++Slot)
 {
  int32 Packed=0;if(!GConfig->GetInt(TEXT("NullRoute.WeaponModules"),Slot==0?TEXT("Primary"):TEXT("Pistol"),Packed,GGameUserSettingsIni)||Packed<0||Packed>63)continue;
  FNRWeaponAssembly B;B.Optic=ENROptic(Packed&3);B.Muzzle=ENRMuzzle((Packed>>2)&3);B.Magazine=ENRMagazine((Packed>>4)&3);
  if(B.IsValid(Slot))ServerConfigureWeapon(Slot,B);
 }
}
void ANRCharacter::OnRep_Modules(){RefreshModuleVisuals();}
void ANRCharacter::RefreshModuleVisuals()
{
 // Reconstruct six cosmetic components from two tiny replicated structs. None on DS.
 if(!bVisualsReady||GetNetMode()==NM_DedicatedServer)return;
 if(ModuleVisuals.IsEmpty())for(int View=0;View<2;++View)for(int Socket=0;Socket<3;++Socket)
 {
  auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(View==0?Weapon.Get():WorldWeapon.Get());
  C->SetLightingChannels(true,View==0,false);C->SetOnlyOwnerSee(View==0);C->SetOwnerNoSee(View==1);C->SetCastShadow(View==1);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
  if(View==0)C->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
  C->RegisterComponent();ModuleVisuals.Add(C);
 }
 const auto B=Assembly(EquippedWeapon);const bool Pistol=EquippedWeapon==1;
 const uint8 Choices[]={uint8(B.Optic),uint8(B.Muzzle),uint8(B.Magazine)};
 static const TCHAR* Meshes[3][2]={{TEXT("SM_ModReflex"),TEXT("SM_Mod2x")},{TEXT("SM_ModSuppressor"),TEXT("SM_ModCompensator")},{TEXT("SM_ModExtended"),TEXT("SM_ModQuick")}};
 for(int I=0;I<ModuleVisuals.Num();++I)
 {
  auto* C=ModuleVisuals[I].Get();const int Socket=I%3;const uint8 Choice=Choices[Socket];
  const bool Show=!bDead&&EquippedWeapon<2&&Choice>0&&Choice<3;
  C->SetVisibility(Show);if(!Show)continue;
  C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Meshes/%s"),Meshes[Socket][Choice-1])));
  C->SetRelativeRotation(FRotator::ZeroRotator);
  C->SetRelativeScale3D(FVector(Pistol&&Socket!=1?.7f:1.f));
  C->SetRelativeLocation(Socket==0?FVector(0,Pistol?1:5,Pistol?13:16):Socket==1?FVector(0,Pistol?23:51,4):FVector(0,Pistol?-3:7,Pistol?-9:-13));
 }
}
