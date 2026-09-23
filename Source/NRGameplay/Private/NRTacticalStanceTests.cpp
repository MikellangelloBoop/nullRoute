#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRCharacterMovement.h"
#include "NRCombatFX.h"
#include "NRGameMode.h"
#include "NRBallisticsSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/InputSettings.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void UNRSmokeSubsystem::RunStanceTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;auto* M=CastChecked<UNRCharacterMovement>(C->GetCharacterMovement());
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_STANCE %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(Stage==0&&T>3)
 {
  Stage=1;GetWorld()->GetAuthGameMode<ANRGameMode>()->BeginCombat();C->ResetForRound();C->TeleportTo(FVector(-1850,700,90),FRotator::ZeroRotator);PC->SetControlRotation(FRotator::ZeroRotator);M->StopMovementImmediately();M->SetMovementMode(MOVE_Walking);
  TArray<FInputActionKeyMapping> Keys;const auto* Input=GetDefault<UInputSettings>();
  auto Bound=[&](FName Action,FKey Key){Keys.Reset();Input->GetActionMappingByName(Action,Keys);return Keys.Num()==1&&Keys[0].Key==Key;};
  Check(Bound("QuietWalk",EKeys::LeftAlt)&&Bound("LeanLeft",EKeys::Q)&&Bound("LeanRight",EKeys::E)&&Bound("Ability",EKeys::Z)&&Bound("Interact",EKeys::F)&&Bound("Link",EKeys::X),TEXT("Alt QE Z F X bindings have no action conflicts"));
  C->QuietPressed();Check(M->GetMaxSpeed()==150&&C->IsQuietWalking(),TEXT("Alt enables slow quiet walk"));
  M->Velocity=FVector(580,0,0);Check(!C->IsQuietWalking(),TEXT("sprint braking cannot become instantly silent"));M->StopMovementImmediately();
  C->bIsCrouched=true;Check(M->GetMaxSpeed()==105,TEXT("quiet crouch speed limited"));C->bIsCrouched=false;
  M->bWantsSprint=true;Check(!M->IsSprinting()&&M->GetMaxSpeed()==150,TEXT("quiet walk takes priority over sprint"));M->bWantsSprint=false;
  C->bAiming=true;Check(M->GetMaxSpeed()==150,TEXT("quiet walk works with ADS"));C->bAiming=false;
  C->QuietReleased();Check(M->GetMaxSpeed()==420,TEXT("Alt release restores normal speed"));
  M->UpdateFromCompressedFlags(FSavedMove_Character::FLAG_Custom_1|FSavedMove_Character::FLAG_Custom_2);
  Check(M->bWantsQuiet&&M->LeanInput==-1,TEXT("movement packets decode quiet and left lean"));
  M->UpdateFromCompressedFlags(FSavedMove_Character::FLAG_Custom_2|FSavedMove_Character::FLAG_Custom_3);Check(M->LeanInput==0,TEXT("opposing lean flags are neutral"));
  C->ResetTacticalStance();C->LeanLeftPressed();C->LeanRightPressed();Check(M->LeanInput==0,TEXT("holding both lean keys centres stance"));C->LeanLeftReleased();Check(M->LeanInput==1,TEXT("releasing one key keeps other lean"));
  C->UpdateTacticalStance(1);FVector Eye;FRotator Aim;C->GetActorEyesViewPoint(Eye,Aim);
  Check(M->LeanAmount>.99f&&FMath::IsNearlyEqual(float(Eye.Y-C->GetActorLocation().Y),32.f,.1f),TEXT("server eye moves with full right lean"));
  auto* Data=static_cast<FNetworkPredictionData_Client_Character*>(M->GetPredictionData_Client());auto Move=Data->AllocateNewMove();Move->SetMoveFor(C,.016f,FVector::ZeroVector,*Data);M->LeanInput=0;M->LeanAmount=0;Move->PrepMoveFor(C);
  Check(M->LeanInput==1&&M->LeanAmount>.99f&&(Move->GetCompressedFlags()&FSavedMove_Character::FLAG_Custom_3),TEXT("saved move replays lean intent and transition"));
  PC->SetControlRotation(FRotator(-12,0,0));FMinimalViewInfo View;C->CalcCamera(.016f,View);
  Check(FMath::IsNearlyEqual(View.Rotation.Roll,10.f,.1f)&&FMath::IsNearlyEqual(View.Rotation.Pitch,-12.f,.1f),TEXT("camera roll preserves mouse pitch"));
  // Test geometry, not just a sign: screen up and the server eye lean to the same side at every yaw.
  bool LeftCorrect=true,RightCorrect=true,BodyCorrect=true;
  const FVector SavedLocation=C->GetActorLocation();C->TeleportTo(FVector(0,5000,90),FRotator::ZeroRotator);M->SetMovementMode(MOVE_Walking);
  for(float Yaw:{0.f,90.f,180.f,270.f})for(int Side:{-1,1})
  {
   C->SetActorRotation(FRotator(0,Yaw,0));PC->SetControlRotation(FRotator(-12,Yaw,0));
   C->ResetTacticalStance();if(Side<0)C->LeanLeftPressed();else C->LeanRightPressed();C->UpdateTacticalStance(1);
   C->Tick(.016f);const FVector TickUp=C->Camera->GetUpVector();C->CalcCamera(.016f,View);
   const FVector Right=FRotationMatrix(FRotator(0,Yaw,0)).GetUnitAxis(EAxis::Y);
   const float CameraSide=FVector::DotProduct(FRotationMatrix(View.Rotation).GetUnitAxis(EAxis::Z),Right)*Side;
   const float EyeSide=FVector::DotProduct(C->GetPawnViewLocation()-C->GetActorLocation(),Right)*Side;
   const bool OK=CameraSide>.15f&&EyeSide>31.f&&FVector::DotProduct(TickUp,Right)*Side>.15f&&FMath::IsNearlyEqual(View.Rotation.Pitch,-12.f,.1f);
   if(Side<0)LeftCorrect&=OK;else RightCorrect&=OK;
   const float SavedLean=M->LeanAmount;M->LeanAmount=0;C->GetMesh()->TickAnimation(.25f,false);C->GetMesh()->RefreshBoneTransforms();
   const FVector UprightHead=C->GetMesh()->GetBoneLocation(TEXT("head"));M->LeanAmount=SavedLean;C->GetMesh()->TickAnimation(.25f,false);C->GetMesh()->RefreshBoneTransforms();
   BodyCorrect&=FVector::DotProduct(C->GetMesh()->GetBoneLocation(TEXT("head"))-UprightHead,Right)*Side>9.f;
  }
  Check(LeftCorrect,TEXT("Q tilts camera and server eye left at four yaw angles"));
  Check(RightCorrect,TEXT("E tilts camera and server eye right at four yaw angles"));
  Check(BodyCorrect,TEXT("third person body leans toward camera displacement"));
  C->TeleportTo(SavedLocation,FRotator::ZeroRotator);M->SetMovementMode(MOVE_Walking);PC->SetControlRotation(FRotator(-12,0,0));C->ResetTacticalStance();C->LeanRightPressed();C->UpdateTacticalStance(1);C->GetActorEyesViewPoint(Eye,Aim);
  FHitResult Hit;const FVector Exposed=Eye+FVector(0,8,0);
  GetWorld()->LineTraceSingleByChannel(Hit,Exposed+FVector(70,0,0),Exposed-FVector(70,0,0),ECC_GameTraceChannel1);
  Check(Hit.GetComponent()==C->LeanHitbox,TEXT("leaning head outside capsule can be shot"));
  auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Combat;C->WeaponReadyAt=0;C->LastShot=-100;
  auto* B=GetWorld()->GetSubsystem<UNRBallisticsSubsystem>();B->Bullets.Reset();C->FireShot();
  Check(B->Bullets.Num()==1&&FVector::Dist(B->Bullets.Last().Position,C->GetPawnViewLocation())<1,TEXT("live projectile starts at server lean eye"));B->Bullets.Reset();GS->Phase=ENRPhase::Preparation;
  auto* Wall=GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(80,2,80));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);Box->RegisterComponent();Wall->SetActorLocation(Eye+FVector(0,-3,0));
  C->UpdateTacticalStance(1);Check(M->LeanAmount>=0&&M->LeanAmount<.6f&&C->GetPawnViewLocation().Y<Wall->GetActorLocation().Y-14,TEXT("bullet-only wall blocks camera and firing origin"));Wall->Destroy();
  C->ResetTacticalStance();C->LeanLeftPressed();C->UpdateTacticalStance(.05f);Check(M->LeanAmount<0&&M->LeanAmount>-.5f,TEXT("lean begins with smooth transition"));C->UpdateTacticalStance(1);Check(M->LeanAmount<-.99f,TEXT("full left lean"));
  M->SetMovementMode(MOVE_Falling);C->UpdateTacticalStance(1);Check(FMath::IsNearlyZero(M->LeanAmount)&&C->LeanHitbox->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("airborne state centres lean and removes exposed hitbox"));M->SetMovementMode(MOVE_Walking);
  C->QuietPressed();C->bDead=true;C->OnRep_Dead();Check(!M->bWantsQuiet&&M->LeanInput==0,TEXT("death clears held stance"));C->ResetForRound();
  auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>();FX->Sound(TEXT("S_Step"),C->GetActorLocation(),.035f,false,true);
  Check(FX->QuietAttenuation&&FX->FootAttenuation&&FX->QuietAttenuation->Attenuation.FalloffDistance==450&&FX->QuietAttenuation->Attenuation.FalloffDistance<FX->FootAttenuation->Attenuation.FalloffDistance,TEXT("quiet footsteps have shorter audible range"));
  PC->SetControlRotation(FRotator::ZeroRotator);C->QuietPressed();
 }
 if(Stage==1){C->AddMovementInput(FVector::ForwardVector);if(T>4.5f){Stage=2;Check(C->GetVelocity().Size2D()>140&&C->GetVelocity().Size2D()<155,TEXT("actual walking simulation respects quiet speed"));C->ResetForRound();Check(!M->bWantsQuiet&&M->LeanInput==0&&M->LeanAmount==0,TEXT("round reset clears tactical stance"));UE_LOG(LogTemp,Display,TEXT("NR_STANCE_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}}
}

void UNRSmokeSubsystem::RunStanceNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)
 {
  if(Stage==0&&T>1)if(auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>()){Stage=1;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);}
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)
  {
   auto* M=CastChecked<UNRCharacterMovement>(It->GetCharacterMovement());
   if(!(NetworkStage&1)&&M->bWantsQuiet&&It->GetVelocity().Size2D()>135&&It->GetVelocity().Size2D()<155){NetworkStage|=1;UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE_SERVER quiet speed : PASS"));}
   if(!(NetworkStage&2)&&M->LeanAmount<-.95f&&It->GetVelocity().Size2D()<1&&It->LeanHitbox->GetCollisionEnabled()==ECollisionEnabled::QueryOnly){NetworkStage|=2;UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE_SERVER stationary lean and exposed head : PASS"));}
   if(!(NetworkStage&4)&&M->LeanAmount>.95f&&It->ReplicatedLean>95){NetworkStage|=4;UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE_SERVER right lean replication : PASS"));}
  }
  return;
 }
 if(!C||!PC||C->HasAuthority())return;
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(FParse::Param(FCommandLine::Get(),TEXT("NRStanceObserver")))
 {
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(*It!=C){if(It->GetLeanAmount()<-.9f)NetworkStage|=1;if(It->GetLeanAmount()>.9f)NetworkStage|=2;}
  if(NetworkStage==3){Check(true,TEXT("observer receives both lean directions"));UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE_OBSERVER PASSED"));FPlatformMisc::RequestExitWithStatus(false,0);}
  else if(T>25){Check(false,TEXT("observer lean timeout"));FPlatformMisc::RequestExitWithStatus(false,1);}return;
 }
 if(Stage==0&&T>3){Stage=1;C->QuietPressed();}
 if(Stage==1)
 {
  C->AddMovementInput(C->GetActorForwardVector());const float Speed=C->GetVelocity().Size2D();
  // Measure the whole interval: a client may reach solid cover before its final sample.
  if(Speed>135&&Speed<155)NetworkStage|=8;if(Speed>155)NetworkStage|=16;
  if(T>4.3f){Stage=2;UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE final quiet speed %.1f"),Speed);Check((NetworkStage&8)&&!(NetworkStage&16),TEXT("owner predicted quiet movement"));C->LeanLeftPressed();}
 }
 if(Stage==2&&T>9){Stage=3;Check(C->GetLeanAmount()<-.95f&&C->GetVelocity().Size2D()<1,TEXT("owner stationary left lean"));C->LeanLeftReleased();C->LeanRightPressed();}
 if(Stage==3&&T>14){Stage=4;Check(C->GetLeanAmount()>.95f,TEXT("owner right lean"));C->LeanRightReleased();C->QuietReleased();}
 if(Stage==4&&T>15){Stage=5;Check(FMath::Abs(C->GetLeanAmount())<.01f&&!C->IsQuietWalking(),TEXT("release restores centre and normal footsteps"));UE_LOG(LogTemp,Display,TEXT("NR_NETSTANCE_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);}
}

void UNRSmokeSubsystem::RunStanceVisual(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;
 if(Stage==0&&T>3){Stage=1;C->SetRole(ENRRole::Assault);C->TeleportTo(FVector(-1850,-920,110),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(-3,22,0));C->QuietPressed();C->LeanLeftPressed();}
 if(Stage==1&&T>7){Stage=2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/StanceLeft.png"),true,false);}
 if(Stage==2&&T>8){Stage=3;C->LeanLeftReleased();C->LeanRightPressed();C->AimPressed();}
 if(Stage==3&&T>10){Stage=4;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/StanceRightADS.png"),true,false);}
 if(Stage==4&&T>12){Stage=5;UE_LOG(LogTemp,Display,TEXT("NR_STANCE_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0);}
}
