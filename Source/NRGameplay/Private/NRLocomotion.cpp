#include "NRLocomotion.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
struct FNRPose : FAnimNode_Base
{
 FAnimNode_BlendSpacePlayer_Standalone Rifle, Pistol, Crouch;
 UAnimSequence *Fall=nullptr,*Reload=nullptr,*PistolReload=nullptr,*Unarmed=nullptr,*JumpStart=nullptr;
 float Speed=0,Direction=0,PistolWeight=0,CrouchWeight=0,AirWeight=0,ReloadWeight=0,KnifeWeight=0;
 float Pitch=0,Lean=0,Recoil=0,Slash=0,ReloadTime=0,FallTime=0,AirTime=0,Land=0;
 bool bPistol=false;
 virtual void Initialize_AnyThread(const FAnimationInitializeContext& C) override
 { Rifle.Initialize_AnyThread(C);Pistol.Initialize_AnyThread(C);Crouch.Initialize_AnyThread(C); }
 virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& C) override
 { Rifle.CacheBones_AnyThread(C);Pistol.CacheBones_AnyThread(C);Crouch.CacheBones_AnyThread(C); }
 virtual void Update_AnyThread(const FAnimationUpdateContext& C) override
 {
  for(auto* Node:{&Rifle,&Pistol,&Crouch})
  {
   Node->SetPosition(FVector(Direction,Speed,0));
   // The blendspace already contains walk and jog speeds; only sprint needs time scaling.
   Node->SetPlayRate(FMath::Clamp(Speed/450.f,1.f,1.45f));Node->Update_AnyThread(C);
  }
 }
 static void Sample(UAnimSequence* Clip,float Time,FPoseContext& Out)
 {
  Out.ResetToRefPose();if(!Clip)return;
  FAnimationPoseData Data(Out);Clip->GetAnimationPose(Data,FAnimExtractContext(FMath::Clamp(Time,0.f,Clip->GetPlayLength()),false));
 }
 static void Blend(FPoseContext& Out,FPoseContext& Other,float Weight)
 {
  if(Weight<.001f)return;FAnimationPoseData A(Out),B(Other);
  FAnimationRuntime::BlendTwoPosesTogetherInPlace(A,B,1.f-FMath::Clamp(Weight,0.f,1.f));
 }
 static FCompactPoseBoneIndex Bone(const FCompactPose& Pose,FName Name)
 {
  const auto& Bones=Pose.GetBoneContainer();return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetReferenceSkeleton().FindBoneIndex(Name)));
 }
 static void UpperBlend(FPoseContext& Out,const FPoseContext& Other,float Alpha,FName Root)
 {
  if(Alpha<.001f)return;const auto RootBone=Bone(Out.Pose,Root);if(RootBone.GetInt()==INDEX_NONE)return;
  for(auto I:Out.Pose.ForEachBoneIndex())
  {
   auto Parent=I;while(Parent.GetInt()!=INDEX_NONE&&Parent!=RootBone)Parent=Out.Pose.GetParentBoneIndex(Parent);
   if(Parent==RootBone){FTransform T;T.Blend(Out.Pose[I],Other.Pose[I],Alpha);Out.Pose[I]=T;}
  }
 }
 // Rotate around a component-space axis but keep the modification local so all children follow.
 static void Rotate(FPoseContext& Out,FName Name,FVector Axis,float Degrees)
 {
  const auto I=Bone(Out.Pose,Name);if(I.GetInt()==INDEX_NONE||FMath::Abs(Degrees)<.001f)return;
  FQuat ParentRotation=FQuat::Identity;
  for(auto P=Out.Pose.GetParentBoneIndex(I);P.GetInt()!=INDEX_NONE;P=Out.Pose.GetParentBoneIndex(P))ParentRotation=Out.Pose[P].GetRotation()*ParentRotation;
  const FQuat Local(ParentRotation.UnrotateVector(Axis),FMath::DegreesToRadians(Degrees));
  Out.Pose[I].SetRotation((Local*Out.Pose[I].GetRotation()).GetNormalized());
 }
 virtual void Evaluate_AnyThread(FPoseContext& Out) override
 {
  Rifle.Evaluate_AnyThread(Out);FPoseContext Other(Out);
  if(PistolWeight>.001f){Pistol.Evaluate_AnyThread(Other);Blend(Out,Other,PistolWeight);}
  if(CrouchWeight>.001f){Crouch.Evaluate_AnyThread(Other);Blend(Out,Other,CrouchWeight);}
  if(AirWeight>.001f){Sample(JumpStart&&AirTime<JumpStart->GetPlayLength()?JumpStart:Fall,JumpStart&&AirTime<JumpStart->GetPlayLength()?AirTime:FallTime,Other);Blend(Out,Other,AirWeight);}
  // The knife leaves the support hand free. Legs retain the directional locomotion pose.
  if(KnifeWeight>.001f){Sample(Unarmed,0,Other);UpperBlend(Out,Other,KnifeWeight,TEXT("clavicle_l"));}
  if(ReloadWeight>.001f)
  {
   auto* Clip=bPistol?PistolReload:Reload;Sample(Clip,ReloadTime*(Clip?Clip->GetPlayLength():0),Other);
   UpperBlend(Out,Other,ReloadWeight,TEXT("spine_01")); // Reload while moving never freezes the legs.
  }
  const float GripPitch=FMath::Lerp(14.2f,11.2f,PistolWeight)*(1.f-ReloadWeight);
  const float Aim=Pitch*(1.f-.7f*ReloadWeight)-GripPitch+Recoil*2;
  for(const auto& Pair:{TPair<FName,float>(TEXT("spine_01"),.30f),{TEXT("spine_03"),.35f},{TEXT("spine_04"),.35f}})
  {Rotate(Out,Pair.Key,FVector::ForwardVector,Aim*Pair.Value);Rotate(Out,Pair.Key,FVector::RightVector,-Lean*18*Pair.Value);Rotate(Out,Pair.Key,FVector::UpVector,-FMath::Lerp(6.5f,2.1f,PistolWeight)*(1.f-ReloadWeight)*Pair.Value);}
  Rotate(Out,TEXT("head"),FVector::ForwardVector,GripPitch*.6f);
  Rotate(Out,TEXT("upperarm_r"),FVector::UpVector,Slash*58);
  Rotate(Out,TEXT("lowerarm_r"),FVector::ForwardVector,Slash*25);
  const auto Pelvis=Bone(Out.Pose,TEXT("pelvis"));if(Pelvis.GetInt()!=INDEX_NONE)Out.Pose[Pelvis].AddToTranslation(FVector(0,0,-Land*3));
  Out.Pose.NormalizeRotations();
 }
};

struct FNRLocomotionProxy : FAnimInstanceProxy
{
 FNRPose Pose;bool WasAir=false;
 FNRLocomotionProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
 virtual FAnimNode_Base* GetCustomRootNode() override {return &Pose;}
 virtual void Initialize(UAnimInstance* Instance) override
 {
  auto* Anim=CastChecked<UNRLocomotion>(Instance);Anim->LoadClips();
  Pose.Rifle.SetBlendSpace(Cast<UBlendSpace>(Anim->Clips[0]));Pose.Pistol.SetBlendSpace(Cast<UBlendSpace>(Anim->Clips[1]));Pose.Crouch.SetBlendSpace(Cast<UBlendSpace>(Anim->Clips[2]));
  Pose.Fall=Cast<UAnimSequence>(Anim->Clips[3]);Pose.Reload=Cast<UAnimSequence>(Anim->Clips[4]);Pose.PistolReload=Cast<UAnimSequence>(Anim->Clips[5]);Pose.Unarmed=Cast<UAnimSequence>(Anim->Clips[6]);Pose.JumpStart=Cast<UAnimSequence>(Anim->Clips[7]);
  FAnimInstanceProxy::Initialize(Instance);
 }
 virtual void PreUpdate(UAnimInstance* Instance,float Dt) override
 {
  FAnimInstanceProxy::PreUpdate(Instance,Dt);auto* C=Cast<ANRCharacter>(Instance->TryGetPawnOwner());if(!C)return;
  // Copy replicated gameplay state on the game thread; worker evaluation never reads actors.
  const FVector V=C->GetActorRotation().UnrotateVector(C->GetVelocity());
  Pose.Speed=FMath::FInterpTo(Pose.Speed,float(V.Size2D()),Dt,14);
  if(V.SizeSquared2D()>16)Pose.Direction=FRotator::NormalizeAxis(FMath::FixedTurn(Pose.Direction,FMath::RadiansToDegrees(FMath::Atan2(V.Y,V.X)),Dt*900));
  const bool Air=C->GetCharacterMovement()->IsFalling();
  auto Smooth=[Dt](float& Value,float Target,float Rate=10){Value=FMath::FInterpTo(Value,Target,Dt,Rate);};
  Smooth(Pose.PistolWeight,C->EquippedWeapon!=0?1:0);Smooth(Pose.KnifeWeight,C->EquippedWeapon==2?1:0);
  Smooth(Pose.CrouchWeight,C->bIsCrouched?1:0);Smooth(Pose.AirWeight,Air?1:0);
  Smooth(Pose.ReloadWeight,C->bReloading?1:0,15);Smooth(Pose.Lean,C->GetLeanAmount(),16);
  Smooth(Pose.Pitch,FMath::Clamp(FRotator::NormalizeAxis(C->GetBaseAimRotation().Pitch),-65.f,65.f),18);
  Pose.bPistol=C->EquippedWeapon==1;const auto* GS=C->GetWorld()->GetGameState<ANRGameState>();
  const float Now=C->GetWorld()->GetTimeSeconds();const float ServerNow=GS?GS->GetServerWorldTimeSeconds():Now;
  Pose.ReloadTime=FMath::Clamp((ServerNow-C->ReloadStartedAt)/C->WeaponStats(C->EquippedWeapon).ReloadSeconds,0.f,1.f);
  Pose.Recoil=FMath::Exp(-FMath::Max(0.f,Now-C->LastPresentedShot)*22);
  Pose.Slash=C->EquippedWeapon==2?FMath::Sin(FMath::Clamp((Now-C->LastPresentedMelee)/.48f,0.f,1.f)*PI):0;
  Pose.AirTime=Air?Pose.AirTime+Dt:0;
  if(!Air&&WasAir)Pose.Land=1;Smooth(Pose.Land,0,10);WasAir=Air;
  Pose.FallTime=Air&&Pose.Fall?FMath::Fmod(Pose.FallTime+Dt,Pose.Fall->GetPlayLength()):0;
 }
};
}

void UNRLocomotion::LoadClips()
{
 Clips.Reset();auto* C=Cast<ANRCharacter>(TryGetPawnOwner());
 for(const TCHAR* Path:{TEXT("/Game/Art/Animations/BS_Operator2D"),TEXT("/Game/Art/Animations/BS_OperatorPistol"),TEXT("/Game/Art/Animations/BS_OperatorCrouch"),
  TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jump/MM_Rifle_Jump_Fall_Loop"),TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload"),TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload"),TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"),TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jump/MM_Rifle_Jump_Start")})
 {
  auto* Clip=LoadObject<UAnimationAsset>(nullptr,Path);if(C)Clip=C->AnimationForBody(Clip);Clips.Add(Clip);
 }
}
FAnimInstanceProxy* UNRLocomotion::CreateAnimInstanceProxy(){return new FNRLocomotionProxy(this);}
void UNRLocomotion::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
