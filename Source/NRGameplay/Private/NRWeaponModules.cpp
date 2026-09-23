#include "NRWeaponModules.h"

FNRWeaponStats NRWeaponModules::Resolve(ENRRole Role,uint8 Weapon,const FNRWeaponAssembly& Input,ENRPrimaryWeapon Primary)
{
 FNRWeaponStats S;if(Weapon>1)return S;
 const auto B=Input.IsValid(Weapon)?Input:FNRWeaponAssembly();
 const auto Kind=Effective(Role,Primary);
 S.Capacity=Weapon==1?12:Kind==ENRPrimaryWeapon::Spectre?12:Kind==ENRPrimaryWeapon::Forge12?8:Kind==ENRPrimaryWeapon::Lancer60?60:30;
 S.Reserve=Weapon==1?36:Kind==ENRPrimaryWeapon::Spectre?36:Kind==ENRPrimaryWeapon::Forge12?24:Kind==ENRPrimaryWeapon::Lancer60?120:Kind==ENRPrimaryWeapon::BR74?60:90;
 S.Damage=Weapon==1?34:Kind==ENRPrimaryWeapon::Spectre?78:Kind==ENRPrimaryWeapon::BR74?42:Kind==ENRPrimaryWeapon::Forge12?10:Kind==ENRPrimaryWeapon::Lancer60?26:28;
 S.Speed=Weapon==1?45000:Kind==ENRPrimaryWeapon::Spectre?85000:Kind==ENRPrimaryWeapon::Forge12?42000:65000;
 S.AimSpread=Weapon==0&&Kind==ENRPrimaryWeapon::Spectre?.04f:.16f;
 S.AimFOV=Weapon==0&&Kind==ENRPrimaryWeapon::Spectre?54:72;
 S.Interval=Weapon==1?.24f:Kind==ENRPrimaryWeapon::Spectre?.5f:Kind==ENRPrimaryWeapon::BR74?.12f:Kind==ENRPrimaryWeapon::Forge12?.82f:Kind==ENRPrimaryWeapon::Lancer60?.11f:.085f;
 S.bAutomatic=Weapon==0&&Kind!=ENRPrimaryWeapon::Spectre&&Kind!=ENRPrimaryWeapon::Forge12;
 if(Weapon==0&&Kind==ENRPrimaryWeapon::Forge12){S.Pellets=8;S.PelletCone=4.5f;S.ReloadSeconds=2.4f;S.RecoilScale=2.2f;S.AimRate=12;}
 if(Weapon==0&&Kind==ENRPrimaryWeapon::Lancer60){S.ReloadSeconds=3.2f;S.RecoilScale=1.25f;S.HipSpreadScale=1.4f;S.AimRate=10;}
 if(B.Optic==ENROptic::Reflex){S.AimFOV=FMath::Min(S.AimFOV,66.f);S.AimSpread*=.8f;}
 if(B.Optic==ENROptic::Combat2x){S.AimFOV=48;S.AimSpread*=.65f;S.AimRate=11;}
 if(B.Muzzle==ENRMuzzle::Suppressor){S.Damage*=.9f;S.Speed*=.9f;}
 if(B.Muzzle==ENRMuzzle::Compensator){S.RecoilScale*=.7f;S.HipSpreadScale*=1.15f;}
 if(B.Magazine==ENRMagazine::Extended){S.Capacity=S.Capacity*3/2;S.ReloadSeconds*=1.21875f;S.AimRate*=.85f;}
 if(B.Magazine==ENRMagazine::Quick){S.Capacity=FMath::RoundToInt(S.Capacity*.8f);S.ReloadSeconds*=.75f;}
 return S;
}
const TCHAR* NRWeaponModules::Name(uint8 Socket,uint8 Choice)
{
 static const TCHAR* Names[3][3]={{TEXT("МЕХАНИЧЕСКИЙ"),TEXT("КОЛЛИМАТОР"),TEXT("ОПТИКА 2×")},{TEXT("ШТАТНЫЙ СТВОЛ"),TEXT("ГЛУШИТЕЛЬ"),TEXT("КОМПЕНСАТОР")},{TEXT("ШТАТНЫЙ"),TEXT("УВЕЛИЧЕННЫЙ"),TEXT("БЫСТРЫЙ")}};
 return Socket<3&&Choice<3?Names[Socket][Choice]:TEXT("—");
}
const TCHAR* NRWeaponModules::Description(uint8 Socket,uint8 Choice)
{
 static const TCHAR* Info[3][3]={
 {TEXT("Открытый прицел\nБыстрое наведение"),TEXT("Чистая красная точка\nРазброс в прицеле −20%"),TEXT("Увеличение 2×\nТочнее, медленнее наведение")},
 {TEXT("Полная скорость пули\nПолный урон"),TEXT("Тише выстрел, без вспышки\nУрон и скорость пули −10%"),TEXT("Отдача −30%\nРазброс от бедра +15%")},
 {TEXT("Базовая вместимость\nБазовое время перезарядки"),TEXT("Вместимость +50%\nПерезарядка +22%"),TEXT("Вместимость около −20%\nПерезарядка −25%")}};
 return Socket<3&&Choice<3?Info[Socket][Choice]:TEXT("");
}

ENRPrimaryWeapon NRWeaponModules::Effective(ENRRole Role,ENRPrimaryWeapon Choice)
{
 if(uint8(Choice)>0&&uint8(Choice)<=5)return Choice;
 return Role==ENRRole::Scout?ENRPrimaryWeapon::Spectre:Role==ENRRole::Assault?ENRPrimaryWeapon::BR74:ENRPrimaryWeapon::MX9;
}
const TCHAR* NRWeaponModules::WeaponName(ENRPrimaryWeapon Choice)
{
 static const TCHAR* Names[]={TEXT("ПО КЛАССУ"),TEXT("MX–9"),TEXT("SPECTRE"),TEXT("BR–74"),TEXT("FORGE–12"),TEXT("LANCER–60")};return Names[FMath::Clamp(int(Choice),0,5)];
}
const TCHAR* NRWeaponModules::WeaponDescription(ENRPrimaryWeapon Choice)
{
 static const TCHAR* Text[]={TEXT("Штатный ствол вашего класса"),TEXT("ПП / ближняя дистанция / быстрый темп"),TEXT("Марксман / одиночный огонь / дальняя дистанция"),TEXT("Карабин / автоматический / высокий урон"),TEXT("Дробовик / 8 дробин / штурм помещений"),TEXT("Пулемёт / 60 патронов / долгая перезарядка")};return Text[FMath::Clamp(int(Choice),0,5)];
}
const TCHAR* NRWeaponModules::Mesh(ENRPrimaryWeapon Choice)
{
 static const TCHAR* Paths[]={TEXT("/Game/Art/Meshes/SM_MX9"),TEXT("/Game/Art/Meshes/SM_MX9"),TEXT("/Game/Art/Meshes/SM_Spectre"),TEXT("/Game/Art/Meshes/SM_AR7"),TEXT("/Game/Art/Meshes/SM_Forge12"),TEXT("/Game/Art/Meshes/SM_Lancer60")};return Paths[FMath::Clamp(int(Choice),0,5)];
}
