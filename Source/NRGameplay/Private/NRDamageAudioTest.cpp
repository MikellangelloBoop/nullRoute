#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRAttributes.h"
#include "NRCombatFX.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UNRSmokeSubsystem::RunDamageAudioTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>();if(!FX)return;
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_DAMAGE_AUDIO %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(Stage==0&&T>3)
 {
  Stage=1;GetWorld()->GetAuthGameMode<ANRGameMode>()->BeginCombat();C->UpdateHealthFeedback();
  Check(FX->LastDamageSound<0,TEXT("spawn is silent"));
  UGameplayStatics::ApplyDamage(C,20,nullptr,nullptr,nullptr);C->UpdateHealthFeedback();
  Check(C->Attributes->GetHealth()<100&&FX->LastDamageCue==TEXT("S_Hurt")&&IsValid(FX->DamageAudio)&&FX->DamageAudio->IsPlaying(),TEXT("actual health loss without causer starts audio"));
  Check(FX->DamageAudio&&FX->DamageAudio->bIsUISound&&!FX->DamageAudio->bAllowSpatialization,TEXT("damage is local 2D without distance attenuation"));
  Check(FX->DamageAudio&&FX->DamageAudio->Sound&&FX->DamageAudio->Sound->Priority>=80,TEXT("damage sound has high playback priority"));
  Check(FX->DamageVoices&&FX->DamageVoices->Concurrency.MaxCount==1,TEXT("single dedicated damage voice"));
  auto* Played=FX->DamageAudio.Get();C->Attributes->SetHealth(100);C->UpdateHealthFeedback();
  Check(FX->DamageAudio==Played,TEXT("healing does not play hurt audio"));
  UGameplayStatics::ApplyDamage(C,1,nullptr,nullptr,nullptr);C->UpdateHealthFeedback();
  Check(FX->DamageAudio==Played,TEXT("rapid hits do not stack sounds"));
  auto* Remote=GetWorld()->SpawnActor<ANRCharacter>(FVector(0,0,-1500),FRotator::ZeroRotator);Remote->Attributes->SetHealth(20);Remote->bHealthFeedbackReady=true;Remote->UpdateHealthFeedback();
  Check(FX->DamageAudio==Played,TEXT("uncontrolled pawn cannot play local hurt audio"));Remote->Destroy();
  Check(!FX->DamageFeedback(0,50)&&!FX->DamageFeedback(-10,50),TEXT("non damage values rejected"));
 }
 if(Stage==1&&T>3.6)
 {
  Stage=2;C->Attributes->SetHealth(20);C->UpdateHealthFeedback();
  Check(FX->LastDamageCue==TEXT("S_HurtCritical")&&FX->DamageAudio&&FX->DamageAudio->IsPlaying(),TEXT("GAS health loss at low HP plays critical cue"));
 }
 if(Stage==2&&T>4.6)
 {
  Stage=3;UGameplayStatics::ApplyDamage(C,100,nullptr,nullptr,nullptr);C->UpdateHealthFeedback();
  Check(C->bDead&&FX->DamageAudio&&FX->DamageAudio->IsPlaying()&&FX->LastDamageSound>4.5,TEXT("lethal hit remains audible"));
  auto* Last=FX->DamageAudio.Get();C->ResetForRound();C->UpdateHealthFeedback();C->UpdateHealthFeedback();
  Check(FX->DamageAudio==Last,TEXT("respawn and unchanged health do not retrigger"));
  UE_LOG(LogTemp,Display,TEXT("NR_DAMAGE_AUDIO_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
 }
}
