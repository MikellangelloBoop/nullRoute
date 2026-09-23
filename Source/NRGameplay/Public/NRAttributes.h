#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NRAttributes.generated.h"
#define NR_ATTR(Name) GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UNRAttributes,Name) GAMEPLAYATTRIBUTE_VALUE_GETTER(Name) GAMEPLAYATTRIBUTE_VALUE_SETTER(Name) GAMEPLAYATTRIBUTE_VALUE_INITTER(Name)
UCLASS() class NRGAMEPLAY_API UNRAttributes:public UAttributeSet {
 GENERATED_BODY()
public:
 UNRAttributes();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 virtual void PreAttributeChange(const FGameplayAttribute& Attribute,float& NewValue)override;
 virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)override;
 UPROPERTY(ReplicatedUsing=OnRep_Health) FGameplayAttributeData Health; NR_ATTR(Health)
 UPROPERTY(ReplicatedUsing=OnRep_Armor) FGameplayAttributeData ArmorDurability; NR_ATTR(ArmorDurability)
 UPROPERTY(ReplicatedUsing=OnRep_Credits) FGameplayAttributeData HardwareTokens; NR_ATTR(HardwareTokens)
 UPROPERTY(ReplicatedUsing=OnRep_Ergonomics) FGameplayAttributeData ErgonomicsPenalty; NR_ATTR(ErgonomicsPenalty)
 UFUNCTION()void OnRep_Health(const FGameplayAttributeData& Old);
 UFUNCTION()void OnRep_Armor(const FGameplayAttributeData& Old);
 UFUNCTION()void OnRep_Credits(const FGameplayAttributeData& Old);
 UFUNCTION()void OnRep_Ergonomics(const FGameplayAttributeData& Old);
};
#undef NR_ATTR
