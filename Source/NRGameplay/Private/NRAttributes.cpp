#include "NRAttributes.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
UNRAttributes::UNRAttributes(){InitHealth(100);InitArmorDurability(100);InitHardwareTokens(600);InitErgonomicsPenalty(.15f);}
void UNRAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME_CONDITION_NOTIFY(UNRAttributes,Health,COND_None,REPNOTIFY_Always);
 DOREPLIFETIME_CONDITION_NOTIFY(UNRAttributes,ArmorDurability,COND_OwnerOnly,REPNOTIFY_Always);
 DOREPLIFETIME_CONDITION_NOTIFY(UNRAttributes,HardwareTokens,COND_OwnerOnly,REPNOTIFY_Always);
 DOREPLIFETIME_CONDITION_NOTIFY(UNRAttributes,ErgonomicsPenalty,COND_OwnerOnly,REPNOTIFY_Always);
}
void UNRAttributes::OnRep_Health(const FGameplayAttributeData& Old){GAMEPLAYATTRIBUTE_REPNOTIFY(UNRAttributes,Health,Old);}
void UNRAttributes::OnRep_Armor(const FGameplayAttributeData& Old){GAMEPLAYATTRIBUTE_REPNOTIFY(UNRAttributes,ArmorDurability,Old);}
void UNRAttributes::OnRep_Credits(const FGameplayAttributeData& Old){GAMEPLAYATTRIBUTE_REPNOTIFY(UNRAttributes,HardwareTokens,Old);}
void UNRAttributes::OnRep_Ergonomics(const FGameplayAttributeData& Old){GAMEPLAYATTRIBUTE_REPNOTIFY(UNRAttributes,ErgonomicsPenalty,Old);}
void UNRAttributes::PreAttributeChange(const FGameplayAttribute& A,float& V){Super::PreAttributeChange(A,V);if(!FMath::IsFinite(V))V=0;
 V=FMath::Clamp(V,0.f,A==GetHealthAttribute()?100.f:A==GetArmorDurabilityAttribute()?500.f:A==GetErgonomicsPenaltyAttribute()?1.f:9999.f);}
void UNRAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& D){Super::PostGameplayEffectExecute(D);SetHealth(FMath::Clamp(GetHealth(),0.f,100.f));SetArmorDurability(FMath::Clamp(GetArmorDurability(),0.f,500.f));SetHardwareTokens(FMath::Clamp(GetHardwareTokens(),0.f,9999.f));}
