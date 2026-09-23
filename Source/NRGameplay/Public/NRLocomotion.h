#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NRLocomotion.generated.h"

// Client presentation only. The dedicated server never constructs this instance.
UCLASS(Transient)
class NRGAMEPLAY_API UNRLocomotion : public UAnimInstance
{
 GENERATED_BODY()
public:
 virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
 virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
 UPROPERTY(Transient) TArray<TObjectPtr<class UAnimationAsset>> Clips;
 void LoadClips();
};
