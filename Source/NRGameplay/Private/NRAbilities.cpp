#include "NRAbilities.h"
#include "NRCharacter.h"
#include "NativeGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_BlackNode,"State.BUP.BlackNode");
UNRAbility::UNRAbility(){InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::ServerOnly;}
void UNRAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event){
 auto* C=Info?Cast<ANRCharacter>(Info->AvatarActor.Get()):nullptr;
 const bool bOK=C&&C->HasAuthority()&&CommitAbility(Handle,Info,Activation)&&ExecuteServer(*C);
 EndAbility(Handle,Info,Activation,true,!bOK);
}
bool UGA_BuildNode::ExecuteServer(ANRCharacter& C){return C.BuildNode();}
bool UGA_PacketSniffer::ExecuteServer(ANRCharacter& C){return C.ScanNode();}
bool UGA_DirectionalBreach::ExecuteServer(ANRCharacter& C){return C.Breach();}
bool UGA_FieldTreatment::ExecuteServer(ANRCharacter& C){return C.TreatWounded();}
UGE_BlackNode::UGE_BlackNode(){DurationPolicy=EGameplayEffectDurationType::Infinite;FInheritedTagContainer T;T.Added.AddTag(TAG_BlackNode);auto* Tags=CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("BlackNodeTags"));GEComponents.Add(Tags);Tags->SetAndApplyTargetTagChanges(T);}

