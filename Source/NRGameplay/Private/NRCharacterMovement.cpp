#include "NRCharacterMovement.h"
#include "NRCharacter.h"

class FNRSavedMove final : public FSavedMove_Character
{
public:
 using Super=FSavedMove_Character;
 bool bSprint=false,bQuiet=false;int8 Lean=0;float StartLean=0;
 virtual void Clear() override {Super::Clear();bSprint=false;bQuiet=false;Lean=0;StartLean=0;}
 virtual uint8 GetCompressedFlags() const override {return Super::GetCompressedFlags()|(bSprint?FLAG_Custom_0:0)|(bQuiet?FLAG_Custom_1:0)|(Lean<0?FLAG_Custom_2:Lean>0?FLAG_Custom_3:0);}
 virtual bool CanCombineWith(const FSavedMovePtr& New,ACharacter* C,float Dt) const override
 {return bQuiet==static_cast<const FNRSavedMove*>(New.Get())->bQuiet&&Lean==static_cast<const FNRSavedMove*>(New.Get())->Lean&&bSprint==static_cast<const FNRSavedMove*>(New.Get())->bSprint&&Super::CanCombineWith(New,C,Dt);}
 virtual void SetMoveFor(ACharacter* C,float Dt,const FVector& A,FNetworkPredictionData_Client_Character& Data) override
 {Super::SetMoveFor(C,Dt,A,Data);auto* M=CastChecked<UNRCharacterMovement>(C->GetCharacterMovement());bSprint=M->bWantsSprint;bQuiet=M->bWantsQuiet;Lean=M->LeanInput;StartLean=M->LeanAmount;}
 virtual void PrepMoveFor(ACharacter* C) override
 {Super::PrepMoveFor(C);auto* M=CastChecked<UNRCharacterMovement>(C->GetCharacterMovement());M->bWantsSprint=bSprint;M->bWantsQuiet=bQuiet;M->LeanInput=Lean;M->LeanAmount=StartLean;}
};
class FNRPredictionData final : public FNetworkPredictionData_Client_Character
{
public:
 explicit FNRPredictionData(const UCharacterMovementComponent& C):FNetworkPredictionData_Client_Character(C){}
 virtual FSavedMovePtr AllocateNewMove() override{return FSavedMovePtr(new FNRSavedMove());}
};
UNRCharacterMovement::UNRCharacterMovement()
{
 MaxWalkSpeed=420;MaxWalkSpeedCrouched=185;MaxAcceleration=1900;BrakingDecelerationWalking=2100;
 GroundFriction=6.5f;BrakingFrictionFactor=1;AirControl=.23f;JumpZVelocity=430;GravityScale=1.15f;
 MaxStepHeight=42;SetCrouchedHalfHeight(56);GetNavAgentPropertiesRef().bCanCrouch=true;
}
bool UNRCharacterMovement::IsSprinting()const
{
 const auto* C=Cast<ANRCharacter>(CharacterOwner);
 // Speed eligibility is recomputed on the server; the client sends intent, never a speed.
 return C&&bWantsSprint&&!bWantsQuiet&&LeanInput==0&&FMath::Abs(LeanAmount)<.05f&&C->IsAlive()&&!C->bAiming&&!C->bReloading&&!C->bIsCrouched&&IsMovingOnGround()
  &&FVector::DotProduct(Acceleration.GetSafeNormal2D(),C->GetActorForwardVector())>.65f;
}
float UNRCharacterMovement::GetMaxSpeed()const
{
 const auto* C=Cast<ANRCharacter>(CharacterOwner);if(!C)return Super::GetMaxSpeed();
 if(!C->IsAlive())return 0;
 float Speed=C->bIsCrouched?185.f:C->bAiming?260.f:IsSprinting()?(C->OperatorRole==ENRRole::Scout?610.f:C->OperatorRole==ENRRole::Assault?550.f:580.f):420.f;
 if(bWantsQuiet&&IsMovingOnGround())Speed=FMath::Min(Speed,C->bIsCrouched?105.f:150.f);
 if(LeanInput!=0||FMath::Abs(LeanAmount)>.05f)Speed=FMath::Min(Speed,220.f);
 if(C->bReloading)Speed=FMath::Min(Speed,320.f);
 if(C->bCarryingDisk)Speed*=.92f;
 return MovementMode==MOVE_None?0:Speed;
}
void UNRCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{Super::UpdateFromCompressedFlags(Flags);bWantsSprint=(Flags&FSavedMove_Character::FLAG_Custom_0)!=0;bWantsQuiet=(Flags&FSavedMove_Character::FLAG_Custom_1)!=0;
 const bool Left=(Flags&FSavedMove_Character::FLAG_Custom_2)!=0,Right=(Flags&FSavedMove_Character::FLAG_Custom_3)!=0;LeanInput=Left==Right?0:Left?-1:1;}
FNetworkPredictionData_Client* UNRCharacterMovement::GetPredictionData_Client()const
{if(!ClientPredictionData)const_cast<UNRCharacterMovement*>(this)->ClientPredictionData=new FNRPredictionData(*this);return ClientPredictionData;}
void UNRCharacterMovement::OnMovementUpdated(float Dt,const FVector& OldLocation,const FVector& OldVelocity)
{Super::OnMovementUpdated(Dt,OldLocation,OldVelocity);if(auto* C=Cast<ANRCharacter>(CharacterOwner)){if(C->GetLocalRole()!=ROLE_SimulatedProxy){C->ConstrainPreparationMovement();C->UpdateTacticalStance(Dt);}if(C->HasAuthority())C->UpdateFootsteps(Dt,true);}}
