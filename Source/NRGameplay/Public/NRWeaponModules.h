#pragma once
#include "CoreMinimal.h"
#include "NRTypes.h"
#include "NRWeaponModules.generated.h"

UENUM() enum class ENRPrimaryWeapon:uint8 { RoleDefault, MX9, Spectre, BR74, Forge12, Lancer60 };

UENUM() enum class ENROptic:uint8 { Iron, Reflex, Combat2x };
UENUM() enum class ENRMuzzle:uint8 { Standard, Suppressor, Compensator };
UENUM() enum class ENRMagazine:uint8 { Standard, Extended, Quick };

// Replicate selections only. Damage, timing and compatibility are derived on the server.
USTRUCT() struct NRGAMEPLAY_API FNRWeaponAssembly
{
 GENERATED_BODY()
 UPROPERTY() ENROptic Optic=ENROptic::Iron;
 UPROPERTY() ENRMuzzle Muzzle=ENRMuzzle::Standard;
 UPROPERTY() ENRMagazine Magazine=ENRMagazine::Standard;
 bool operator==(const FNRWeaponAssembly& B)const{return Optic==B.Optic&&Muzzle==B.Muzzle&&Magazine==B.Magazine;}
 bool IsValid(uint8 Weapon)const{return Weapon<2&&uint8(Optic)<3&&uint8(Muzzle)<3&&uint8(Magazine)<3&&!(Weapon==1&&Optic==ENROptic::Combat2x);}
};

struct NRGAMEPLAY_API FNRWeaponStats
{
 int32 Capacity=0,Reserve=0,Pellets=1;
 float Interval=.12f,PelletCone=0;bool bAutomatic=false;
 float Damage=0,Speed=0,ReloadSeconds=1.6f,AimSpread=.16f,HipSpreadScale=1,RecoilScale=1,AimFOV=72,AimRate=15;
};

namespace NRWeaponModules
{
 NRGAMEPLAY_API FNRWeaponStats Resolve(ENRRole Role,uint8 Weapon,const FNRWeaponAssembly& Build,ENRPrimaryWeapon Primary=ENRPrimaryWeapon::RoleDefault);
 NRGAMEPLAY_API ENRPrimaryWeapon Effective(ENRRole Role,ENRPrimaryWeapon Choice);
 NRGAMEPLAY_API const TCHAR* WeaponName(ENRPrimaryWeapon Choice);
 NRGAMEPLAY_API const TCHAR* WeaponDescription(ENRPrimaryWeapon Choice);
 NRGAMEPLAY_API const TCHAR* Mesh(ENRPrimaryWeapon Choice);
 NRGAMEPLAY_API const TCHAR* Name(uint8 Socket,uint8 Choice);
 NRGAMEPLAY_API const TCHAR* Description(uint8 Socket,uint8 Choice);
}
