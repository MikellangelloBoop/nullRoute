#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "NRAbilities.generated.h"
UCLASS(Abstract)class NRGAMEPLAY_API UNRAbility:public UGameplayAbility {
 GENERATED_BODY()
public:UNRAbility();
protected:
 virtual bool ExecuteServer(class ANRCharacter& Character){return false;}
 virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event)override;
};
UCLASS()class NRGAMEPLAY_API UGA_BuildNode:public UNRAbility {GENERATED_BODY() protected:virtual bool ExecuteServer(ANRCharacter& Character)override;};
UCLASS()class NRGAMEPLAY_API UGA_PacketSniffer:public UNRAbility {GENERATED_BODY() protected:virtual bool ExecuteServer(ANRCharacter& Character)override;};
UCLASS()class NRGAMEPLAY_API UGA_DirectionalBreach:public UNRAbility {GENERATED_BODY() protected:virtual bool ExecuteServer(ANRCharacter& Character)override;};
UCLASS()class NRGAMEPLAY_API UGA_FieldTreatment:public UNRAbility {GENERATED_BODY() protected:virtual bool ExecuteServer(ANRCharacter& Character)override;};
UCLASS()class NRGAMEPLAY_API UGE_BlackNode:public UGameplayEffect {GENERATED_BODY() public:UGE_BlackNode();};
