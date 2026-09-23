#include "NRCharacter.h"
#include "NRAttributes.h"
#include "NRGameMode.h"
#include "NRHUD.h"
#include "AbilitySystemComponent.h"

bool ANRCharacter::TreatWounded()
{
 const auto* State=GetWorld()->GetGameState<ANRGameState>();
 const float Now=GetWorld()->GetTimeSeconds();
 if(!HasAuthority()||OperatorRole!=ENRRole::Medic||!CanAct()||!State||State->Phase!=ENRPhase::Combat||Now<AbilityReadyTime)return false;
 FVector Origin;FRotator Aim;GetActorEyesViewPoint(Origin,Aim);
 FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(NRTreatment),false,this);
 GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Aim.Vector()*600,ECC_GameTraceChannel1,Query);
 ANRCharacter* Target=Cast<ANRCharacter>(Hit.GetActor());
 if(Target&&(Target->Team!=Team||!Target->IsAlive()))return false;
 if(!Target)Target=this;
 const float Restored=FMath::Min(40.f,100.f-Target->Attributes->GetHealth());
 if(Restored<=0)return false;
 Target->ASC->ApplyModToAttribute(UNRAttributes::GetHealthAttribute(),EGameplayModOp::Additive,Restored);
 AbilityReadyTime=Now+18.f;Target->ForceNetUpdate();ForceNetUpdate();
 ClientTreatment(Restored);if(Target!=this)Target->ClientTreatment(Restored);Say(2);return true;
}
void ANRCharacter::ClientTreatment_Implementation(float Restored)
{
 CombatNotice=FString::Printf(TEXT("ПОЛЕВОЕ ЛЕЧЕНИЕ  +%.0f HP"),Restored);
 CombatNoticeUntil=GetWorld()->GetTimeSeconds()+3.f;
}
FString ANRHUD::TrainingOptions(bool Defend)const
{
 const auto Selected=ENRFaction(FMath::Clamp(TrainingFaction,0,int(NRFactions::Count)-1));
 const auto Opponent=Selected==ENRFaction::Chronos?ENRFaction::Rebel:ENRFaction::Chronos;
 return FString::Printf(TEXT("Training=1?TrainingTeam=%d?SkipMenu=1?AttackerFaction=%s?DefenderFaction=%s"),Defend?0:1,NRFactions::Key(Defend?Opponent:Selected),NRFactions::Key(Defend?Selected:Opponent));
}
