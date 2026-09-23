#include "NRCharacter.h"
#include "NRPreparationZone.h"
#include "NRAttributes.h"
#include "NRGameMode.h"
#include "NRNode.h"
#include "NRActivity.h"
#include "NRDroneDirector.h"
#include "NRCombatFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void ANRCharacter::PrimaryPressed(){SelectWeapon(0);}
void ANRCharacter::PistolPressed(){SelectWeapon(1);}
void ANRCharacter::KnifePressed(){SelectWeapon(2);}
void ANRCharacter::SelectWeapon(uint8 Slot){if(!IsUIBlocking())ServerEquip(Slot);}
void ANRCharacter::ServerEquip_Implementation(uint8 Slot)
{
 const float Now=GetWorld()->GetTimeSeconds();
 if(!CanAct()||Slot>2||Slot==EquippedWeapon||Now-LastEquip<.35f)return;
 if(EquippedWeapon==0){PrimaryAmmo=Ammo;PrimaryReserve=ReserveAmmo;}
 if(EquippedWeapon==1){PistolAmmo=Ammo;PistolReserve=ReserveAmmo;}
 GetWorldTimerManager().ClearTimer(FireTimer);GetWorldTimerManager().ClearTimer(ReloadTimer);GetWorldTimerManager().ClearTimer(MeleeTimer);
 bReloading=false;bAiming=false;EquippedWeapon=Slot;LastEquip=Now;WeaponReadyAt=Now+.35f;
 Ammo=Slot==0?PrimaryAmmo:Slot==1?PistolAmmo:0;ReserveAmmo=Slot==0?PrimaryReserve:Slot==1?PistolReserve:0;
 OnRep_Equipped();ForceNetUpdate();
}
void ANRCharacter::PlayReadyAnimation()
{
 if(!bVisualsReady)return;
 const TCHAR* Clip=EquippedWeapon==0?(bReloading?TEXT("/Game/Art/Animations/A_FP_Reload"):TEXT("/Game/Art/Animations/A_FP_Ready")):
  EquippedWeapon==1?(bReloading?TEXT("/Game/Art/Animations/A_FP_PistolReload"):TEXT("/Game/Art/Animations/A_FP_Pistol")):TEXT("/Game/Art/Animations/A_FP_Knife");
 ViewArms->PlayAnimation(AnimationForBody(LoadObject<UAnimSequence>(nullptr,Clip)),!bReloading);
}
void ANRCharacter::OnRep_Equipped()
{
 if(!bVisualsReady)return;EquipPresentedAt=GetWorld()->GetTimeSeconds();InspectStart=-100;OnRep_Role();PlayReadyAnimation();
 if(IsLocallyControlled())if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_Equip"),GetActorLocation(),.35f,true);
}
void ANRCharacter::MeleeStrike()
{
 auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();const float Now=GetWorld()->GetTimeSeconds();
 if(!HasAuthority()||!CanAct()||!GM||EquippedWeapon!=2||Now<WeaponReadyAt||Now-LastShot<.65f)return;
 LastShot=Now;MulticastMelee();GetWorldTimerManager().SetTimer(MeleeTimer,this,&ANRCharacter::ResolveMelee,.16f,false);
}
void ANRCharacter::ResolveMelee()
{
 auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();
 if(!HasAuthority()||!CanAct()||!GM||EquippedWeapon!=2)return;
 FVector O;FRotator R;GetActorEyesViewPoint(O,R);FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRMelee),false,this);
 // First blocking hit prevents melee through walls. No client-provided hit or damage values.
 if(!GetWorld()->SweepSingleByChannel(H,O,O+R.Vector()*155,FQuat::Identity,ECC_GameTraceChannel1,FCollisionShape::MakeSphere(16),Q))return;
 if(!GM->IsCombat())
 {
  if(auto* Target=Cast<ANRPreparationProp>(H.GetActor()))UGameplayStatics::ApplyPointDamage(Target,65,R.Vector(),H,Controller,this,nullptr);
  MulticastImpact(H.ImpactPoint,H.ImpactNormal,false);return;
 }
 auto* Enemy=Cast<ANRCharacter>(H.GetActor());bool Hit=false;
 if(Enemy&&Enemy->Team!=Team&&Enemy->IsAlive()){UGameplayStatics::ApplyPointDamage(Enemy,65,R.Vector(),H,Controller,this,nullptr);Hit=true;}
 else if(auto* D=Cast<ANRDroneDirector>(H.GetActor()))Hit=D->HitDrone(H.Item,65,this);
 else if(Cast<ANodeBase>(H.GetActor())||Cast<ANRActivity>(H.GetActor())){UGameplayStatics::ApplyPointDamage(H.GetActor(),65,R.Vector(),H,Controller,this,nullptr);Hit=true;}
 MulticastImpact(H.ImpactPoint,H.ImpactNormal,Enemy!=nullptr);if(Hit&&!Cast<ANRDroneDirector>(H.GetActor()))ClientHitFeedback(Enemy&&!Enemy->IsAlive());
}
void ANRCharacter::MulticastMelee_Implementation()
{
 if(GetNetMode()==NM_DedicatedServer)return;MeleeVisualAt=GetWorld()->GetTimeSeconds();LastPresentedMelee=MeleeVisualAt;
 if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_Knife"),GetActorLocation(),.5f,IsLocallyControlled());
}
void ANRCharacter::AwardSupply()
{
 if(!HasAuthority()||!IsAlive())return;
 const int32 Cap=WeaponStats(0).Reserve;
 if(EquippedWeapon==0)ReserveAmmo=FMath::Max(ReserveAmmo,FMath::Min(Cap,ReserveAmmo+FMath::Max(8,WeaponStats(0).Capacity)));else PrimaryReserve=FMath::Max(PrimaryReserve,FMath::Min(Cap,PrimaryReserve+FMath::Max(8,WeaponStats(0).Capacity)));
 if(EquippedWeapon==1)ReserveAmmo=FMath::Max(ReserveAmmo,FMath::Min(36,ReserveAmmo+12));else PistolReserve=FMath::Max(PistolReserve,FMath::Min(36,PistolReserve+12));
 Attributes->SetHealth(FMath::Min(100.f,Attributes->GetHealth()+35));Say(3);
}
void ANRCharacter::Say(uint8 Cue)
{
 if(!HasAuthority()||Cue>7)return;const float Now=GetWorld()->GetTimeSeconds();
 if(Cue<5&&Now-LastVoice<3.5f)return;LastVoice=Now;ClientVoice(Cue,uint8(OperatorRole));
}
void ANRCharacter::ClientVoice_Implementation(uint8 Cue,uint8 VoiceRole)
{
 if(GetNetMode()==NM_DedicatedServer||Cue>7||VoiceRole>3)return;
 const TCHAR* Lines[4][5]={
 {TEXT("Инженер на связи. Сеть под контролем."),TEXT("Перезарядка."),TEXT("Запускаю печать. Прикройте."),TEXT("Запасы приняты. Продолжаем."),TEXT("Связи установлены.")},
 {TEXT("Разведчик на позиции. Работаю тихо."),TEXT("Перезаряжаюсь."),TEXT("Читаю пакеты. Внедряюсь в сеть."),TEXT("Снаряжение пополнено."),TEXT("Шифр снят. Передача завершена.")},
 {TEXT("Штурмовик готов. Открываем проход."),TEXT("Перезарядка. Прикройте."),TEXT("Заряд установлен. Отойдите."),TEXT("Патроны получены."),TEXT("Двигаемся дальше.")},
 {TEXT("Медик на позиции."),TEXT("Перезаряжаюсь. Прикройте."),TEXT("Лечение завершено."),TEXT("Медицинские запасы пополнены."),TEXT("Готов к поддержке.")}};
 const TCHAR* Elara[]={TEXT("Я Элара. Найдите моё ядро. Я отмечу путь."),TEXT("Я с вами. Несите ядро к янтарному шлюзу."),TEXT("Связь восстановлена. Спасибо, что не оставили меня.")};
 const FString Line=Cue<5?Lines[VoiceRole][Cue]:Elara[Cue-5];
 const FString Name=Cue<5?FString::Printf(TEXT("V_R%d_%d"),VoiceRole,Cue):FString::Printf(TEXT("V_Elara_%d"),Cue-5);
 auto* Wave=VoiceRole==3&&Cue<5?nullptr:LoadObject<USoundBase>(nullptr,*(TEXT("/Game/Art/Audio/")+Name));
 if(DialogueAudio)DialogueAudio->Stop();
 if(Wave)DialogueAudio=UGameplayStatics::SpawnSound2D(this,Wave,.72f);
 Subtitle=(Cue>=5?TEXT("ЭЛАРА  /  "):VoiceRole==0?TEXT("ИНЖЕНЕР  /  "):VoiceRole==1?TEXT("РАЗВЕДЧИК  /  "):VoiceRole==2?TEXT("ШТУРМОВИК  /  "):TEXT("МЕДИК  /  "))+Line;
 SubtitleUntil=GetWorld()->GetTimeSeconds()+(Wave?FMath::Max(3.f,Wave->GetDuration()):4.5f);
}
