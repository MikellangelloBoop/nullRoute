#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NRCharacterMovement.generated.h"

// Intent travels in saved movement packets, so sprint is replayed during correction.
UCLASS() class NRGAMEPLAY_API UNRCharacterMovement : public UCharacterMovementComponent
{
 GENERATED_BODY()
public:
 UNRCharacterMovement();
 bool bWantsSprint=false,bWantsQuiet=false;
 int8 LeanInput=0;
 float LeanAmount=0;
 bool IsSprinting() const;
 virtual float GetMaxSpeed() const override;
 virtual void UpdateFromCompressedFlags(uint8 Flags) override;
 virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
protected:
 virtual void OnMovementUpdated(float Dt,const FVector& OldLocation,const FVector& OldVelocity) override;
};
