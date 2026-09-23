#include "NRCharacter.h"
#include "NRCharacterMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

bool ANRCharacter::IsQuietWalking()const
{
 const auto* M=CastChecked<UNRCharacterMovement>(GetCharacterMovement());
 // A player braking from a sprint does not get silent footsteps while still moving fast.
 return IsAlive()&&M->bWantsQuiet&&M->IsMovingOnGround()&&GetVelocity().Size2D()<=(bIsCrouched?120:165);
}
void ANRCharacter::QuietPressed(){if(IsAlive()&&!IsUIBlocking())CastChecked<UNRCharacterMovement>(GetCharacterMovement())->bWantsQuiet=true;}
void ANRCharacter::QuietReleased(){CastChecked<UNRCharacterMovement>(GetCharacterMovement())->bWantsQuiet=false;}
void ANRCharacter::LeanLeftPressed(){if(IsAlive()&&!IsUIBlocking()){bLeanLeftHeld=true;RefreshLeanInput();}}
void ANRCharacter::LeanLeftReleased(){bLeanLeftHeld=false;RefreshLeanInput();}
void ANRCharacter::LeanRightPressed(){if(IsAlive()&&!IsUIBlocking()){bLeanRightHeld=true;RefreshLeanInput();}}
void ANRCharacter::LeanRightReleased(){bLeanRightHeld=false;RefreshLeanInput();}
void ANRCharacter::RefreshLeanInput(){CastChecked<UNRCharacterMovement>(GetCharacterMovement())->LeanInput=bLeanLeftHeld==bLeanRightHeld?0:bLeanLeftHeld?-1:1;}
float ANRCharacter::GetLeanAmount()const{return GetLocalRole()==ROLE_SimulatedProxy?RemoteLean:CastChecked<UNRCharacterMovement>(GetCharacterMovement())->LeanAmount;}
FVector ANRCharacter::LeanOffset(float Value)const
{
 return FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y)*(Value*32.f)+FVector(0,0,-FMath::Abs(Value)*4.f);
}
float ANRCharacter::ConstrainLean(float Value)const
{
 if(!IsAlive()||FMath::Abs(Value)<.001f)return 0;
 const FVector Base=Super::GetPawnViewLocation(),End=Base+LeanOffset(Value);
 FCollisionQueryParams Q(SCENE_QUERY_STAT(NRLean),false,this);FHitResult Hit;float Fraction=1;
 // Visibility and bullet blockers both constrain the eye. Thin cover cannot be bypassed by moving the camera.
 for(ECollisionChannel Channel:{ECC_Visibility,ECC_GameTraceChannel1})
  if(GetWorld()->SweepSingleByChannel(Hit,Base,End,FQuat::Identity,Channel,FCollisionShape::MakeSphere(14),Q))Fraction=FMath::Min(Fraction,Hit.bStartPenetrating?0.f:FMath::Max(0.f,Hit.Time-.04f));
 return Value*Fraction;
}
FVector ANRCharacter::GetPawnViewLocation()const
{
 // Every server weapon/ability trace uses this same constrained origin. Never accept a client eye position.
 return Super::GetPawnViewLocation()+LeanOffset(ConstrainLean(GetLeanAmount()));
}
void ANRCharacter::UpdateTacticalStance(float Dt)
{
 auto* M=CastChecked<UNRCharacterMovement>(GetCharacterMovement());
 const float Target=IsAlive()&&M->IsMovingOnGround()?float(FMath::Clamp(int32(M->LeanInput),-1,1)):0.f;
 M->LeanAmount=ConstrainLean(FMath::FInterpConstantTo(M->LeanAmount,Target,Dt,6.f));
 if(HasAuthority())ReplicatedLean=int8(FMath::RoundToInt(M->LeanAmount*100));
 LeanHitbox->SetWorldLocation(GetPawnViewLocation());
 LeanHitbox->SetCollisionEnabled(IsAlive()&&FMath::Abs(M->LeanAmount)>.05f?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
}
void ANRCharacter::UpdateStanceVisuals(float Dt)
{
 if(GetLocalRole()==ROLE_SimulatedProxy)
 {
  RemoteLean=FMath::FInterpTo(RemoteLean,ReplicatedLean/100.f,Dt,15);
  LeanHitbox->SetWorldLocation(GetPawnViewLocation());LeanHitbox->SetCollisionEnabled(IsAlive()&&FMath::Abs(RemoteLean)>.05f?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 // Upper-body lean is evaluated by NRLocomotion. Keep the feet and the mesh smoothing transform intact.
 GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
}
FRotator ANRCharacter::LeanViewRotation()const
{
 // UE Rotator positive roll tilts camera up toward local +Y (right).
 // Share this with Tick: eye displacement, camera and exposed head must agree.
 FRotator R=GetControlRotation();R.Roll=GetLeanAmount()*10.f;return R;
}
void ANRCharacter::CalcCamera(float Dt,FMinimalViewInfo& Out)
{
 Camera->SetWorldRotation(LeanViewRotation());Super::CalcCamera(Dt,Out);
}
void ANRCharacter::ResetTacticalStance()
{
 auto* M=CastChecked<UNRCharacterMovement>(GetCharacterMovement());M->bWantsQuiet=false;M->LeanInput=0;M->LeanAmount=0;
 ReplicatedLean=0;RemoteLean=0;bLeanLeftHeld=bLeanRightHeld=false;LeanHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
