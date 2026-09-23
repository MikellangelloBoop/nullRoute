#include "NRCharacter.h"
#include "NRGameMode.h"
#include "TimerManager.h"
void ANRCharacter::ChoosePrimary(ENRPrimaryWeapon Choice)
{
 WorkshopStatus=TEXT("Запрос оружия…");ServerChoosePrimary(Choice);
}
void ANRCharacter::ServerChoosePrimary_Implementation(ENRPrimaryWeapon Choice)
{
 const double Now=GetWorld()->GetRealTimeSeconds();
 if(!CanCustomize()||uint8(Choice)>5||Now-LastPrimaryRequest<.25){ClientPrimaryResult(PrimaryWeapon,false);return;}
 LastPrimaryRequest=Now;if(PrimaryWeapon==Choice){ClientPrimaryResult(Choice,true);return;}
 // Selection is allowed only in preparation. Changing calibre does not manufacture ammunition.
 const int32 OldTotal=EquippedWeapon==0?Ammo+ReserveAmmo:PrimaryAmmo+PrimaryReserve;
 GetWorldTimerManager().ClearTimer(FireTimer);GetWorldTimerManager().ClearTimer(ReloadTimer);GetWorldTimerManager().ClearTimer(MeleeTimer);
 bReloading=false;bAiming=false;PrimaryWeapon=Choice;const auto Stats=WeaponStats(0);const int32 Total=FMath::Min(OldTotal,Stats.Capacity+Stats.Reserve);
 PrimaryAmmo=FMath::Min(Total,Stats.Capacity);PrimaryReserve=FMath::Max(0,Total-PrimaryAmmo);
 if(EquippedWeapon==0){Ammo=PrimaryAmmo;ReserveAmmo=PrimaryReserve;}
 WeaponReadyAt=GetWorld()->GetTimeSeconds()+.35f;OnRep_Role();ForceNetUpdate();ClientPrimaryResult(Choice,true);
}

void ANRCharacter::ClientPrimaryResult_Implementation(ENRPrimaryWeapon Choice,bool Accepted)
{
 WorkshopStatus=Accepted?TEXT("Оружие выбрано. Для пополнения используйте станцию снабжения."):TEXT("Замена недоступна: дождитесь подготовки и повторите.");
}
