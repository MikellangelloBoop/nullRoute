#pragma once
#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "NRTypes.h"
#include "NRWeaponModules.h"
#include "NRFactions.h"
#include "NRCharacter.generated.h"
class USkeletalMeshComponent;class UPointLightComponent;
class UAbilitySystemComponent;class UNRAttributes;class UCameraComponent;class UStaticMeshComponent;
class UNavigationInvokerComponent;class ANodeBase;class ANRStructure;
UCLASS() class NRGAMEPLAY_API ANRCharacter:public ACharacter,public IAbilitySystemInterface {
 GENERATED_BODY()
public:
 ANRCharacter(const FObjectInitializer& ObjectInitializer=FObjectInitializer::Get());
 int32 MagazineCapacity()const{return WeaponStats(EquippedWeapon).Capacity;}
 FNRWeaponStats WeaponStats(uint8 Slot)const;
 const FNRWeaponAssembly& Assembly(uint8 Slot)const{return Slot==1?PistolAssembly:PrimaryAssembly;}
 void ConfigureWeapon(uint8 Slot,FNRWeaponAssembly Build);
 bool CanCustomize()const;
 void ToggleWorkbench();
 FString WorkshopStatus;
 UPROPERTY(ReplicatedUsing=OnRep_Modules) FNRWeaponAssembly PrimaryAssembly;
 UPROPERTY(ReplicatedUsing=OnRep_Modules) FNRWeaponAssembly PistolAssembly;
 int32 ReserveCapacity()const{return WeaponStats(EquippedWeapon).Reserve;}
 ENRPrimaryWeapon PrimaryKind()const{return NRWeaponModules::Effective(OperatorRole,PrimaryWeapon);}
 UPROPERTY(ReplicatedUsing=OnRep_Role) ENRPrimaryWeapon PrimaryWeapon=ENRPrimaryWeapon::RoleDefault;
 void ChoosePrimary(ENRPrimaryWeapon Choice);
 UFUNCTION(Server,Reliable)void ServerChoosePrimary(ENRPrimaryWeapon Choice);
 UFUNCTION(Client,Reliable) void ClientPrimaryResult(ENRPrimaryWeapon Choice,bool Accepted);
 double LastPrimaryRequest=-100;

 // Only the active magazine/reserve is replicated to its owner; swaps conserve both banks.
 UPROPERTY(ReplicatedUsing=OnRep_Equipped) uint8 EquippedWeapon=0;
 void SelectWeapon(uint8 Slot);
 void AwardSupply();
 void RefillAmmunition();
 void ConstrainPreparationMovement();
 UPROPERTY(Replicated) int32 PracticeHits=0;
 void Say(uint8 Cue);
 UFUNCTION(Client,Reliable)void ClientDroneFeedback(uint8 Kind,bool Killed,int32 Reward);
 UFUNCTION(Client,Reliable)void ClientReceivePing(FVector_NetQuantize Position,bool Danger);
 void ReportWeaponNoise(bool Suppressed);
 FVector TacticalPing=FVector::ZeroVector;float PingUntil=0;bool bDangerPing=false;
 FString CombatNotice;float CombatNoticeUntil=0;
 UPROPERTY(Replicated)int32 DroneKills=0;
 FString Subtitle;
 float SubtitleUntil=0;
 bool IsSprinting()const;
 bool IsQuietWalking()const;
 float GetLeanAmount()const;
 void UpdateTacticalStance(float Dt);
 virtual FVector GetPawnViewLocation()const override;
 virtual void CalcCamera(float DeltaTime,struct FMinimalViewInfo& OutResult)override;
 UPROPERTY(Replicated) int8 ReplicatedLean=0;
 void UpdateFootsteps(float Dt,bool Server);
 ENRFaction GetFaction()const;
 void RefreshAppearance();
 bool UsesMetaHuman()const{return bUsingMetaHuman;}
 class UAnimationAsset* AnimationForBody(class UAnimationAsset* Source)const;
 void SelectSkin(uint8 Index);
 virtual void Landed(const FHitResult& Hit)override;
 virtual void OnStartCrouch(float HalfHeightAdjust,float ScaledHalfHeightAdjust)override;
 virtual void OnEndCrouch(float HalfHeightAdjust,float ScaledHalfHeightAdjust)override;
 UPROPERTY(Replicated) int32 ReserveAmmo=90;
 UPROPERTY(ReplicatedUsing=OnRep_Skin) uint8 WeaponSkin=0;
 friend class UNRSmokeSubsystem;
 UFUNCTION(NetMulticast,Unreliable)void MulticastBreach(FVector_NetQuantize Position);
 virtual void Tick(float DeltaTime) override;
 void SelectClass(ENRRole NewRole);
 void ToggleMenu();
 void ToggleGraph();
 void ConnectNodes(ANodeBase* From,ANodeBase* To);
 void ProgramNode(ANodeBase* Target,ENRNodeOp Operation);
 UFUNCTION(NetMulticast,Unreliable) void MulticastImpact(FVector_NetQuantize Position,FVector_NetQuantizeNormal Normal,bool Organic);
 UFUNCTION(Client,Unreliable) void ClientHitFeedback(bool Killed);
 UFUNCTION(NetMulticast,Unreliable)void MulticastRicochet(FVector_NetQuantize Origin,FVector_NetQuantizeNormal Direction,float Speed);
 UFUNCTION(Client,Unreliable)void ClientDamageDirection(FVector_NetQuantize Source);
 float DamageSourceYaw=0;
 float ReloadFraction()const{return bReloading?FMath::Clamp(ReloadVisualTime/WeaponStats(EquippedWeapon).ReloadSeconds,0.f,1.f):0.f;}
 float LastHitTime=-100,LastDamageTime=-100; bool bLastHitKilled=false;
 FVector MuzzlePosition() const;
 virtual UAbilitySystemComponent* GetAbilitySystemComponent()const override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input)override;
 virtual void PossessedBy(AController* NewController)override;
 virtual void OnRep_Controller()override;
 virtual float TakeDamage(float Amount,FDamageEvent const& Event,AController* Instigator,AActor* Causer)override;
 bool BuildNode();bool ScanNode();bool Breach();bool TreatWounded();
 UFUNCTION(Client,Reliable)void ClientTreatment(float Restored);
 void ResetForRound();void SetRole(ENRRole Value);void GiveCredits(int32 Amount);
 bool IsAlive()const;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> ASC;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UNRAttributes> Attributes;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY(Transient) TObjectPtr<UPointLightComponent> ViewFill;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Weapon;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UNavigationInvokerComponent> Invoker;
 UPROPERTY(ReplicatedUsing=OnRep_Team) uint8 Team=0;
 UPROPERTY(Replicated) bool bAiming=false;
 UPROPERTY(Replicated) int32 Kills=0;
 UPROPERTY(Replicated) int32 Deaths=0;
 UPROPERTY(ReplicatedUsing=OnRep_Role) ENRRole OperatorRole=ENRRole::Engineer;
 UPROPERTY(Replicated) bool bCarryingDisk=false;
 UPROPERTY(ReplicatedUsing=OnRep_Dead) bool bDead=false;
 UPROPERTY(Replicated) int32 Ammo=30;
 UPROPERTY(Replicated) bool bReloading=false;
 UPROPERTY(Replicated) float ReloadStartedAt=0;
 // Cosmetic cues use the existing multicast; no per-bone replication.
 float LastPresentedShot=-100,LastPresentedMelee=-100;
 float EquipPresentedAt=-100;
 UPROPERTY(Replicated) float AbilityReadyTime=0;
 UPROPERTY(Replicated) float SpoofUntil=0;
 UPROPERTY(Replicated) FNRScanResult ScanResult;
 UPROPERTY(Replicated) TObjectPtr<ANodeBase> PrintingNode;
protected:
 virtual void BeginPlay()override;
 virtual void EndPlay(EEndPlayReason::Type Reason)override;
private:
 UPROPERTY() TObjectPtr<USkeletalMeshComponent> ViewArms;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ViewSleeves;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ViewCuffs;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> WorldWeapon;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Scope;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> BuildPreview;
 void PingPressed();
 UFUNCTION(Server,Reliable)void ServerPing();
 float LastPing=-100;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ArmorParts;
 bool bUsingMetaHuman=false;
 ENRFaction CachedFaction=ENRFaction::Chronos;
 ENRRole CachedAppearanceRole=ENRRole::Engineer;
 uint8 CachedAppearanceTeam=255;
 void InitializeVisuals();
 void PresentDeath();bool bPresentedDeath=false;
 void QuietPressed();void QuietReleased();
 void LeanLeftPressed();void LeanLeftReleased();void LeanRightPressed();void LeanRightReleased();
 void RefreshLeanInput();void ResetTacticalStance();void UpdateStanceVisuals(float Dt);
 float ConstrainLean(float Value)const;
 FVector LeanOffset(float Value)const;
 FRotator LeanViewRotation()const;
 UPROPERTY() TObjectPtr<class USphereComponent> LeanHitbox;
 bool bLeanLeftHeld=false,bLeanRightHeld=false;
 float RemoteLean=0;
 void UpdateHealthFeedback();
 bool bHealthFeedbackReady=false;
 void RestoreWeaponModules();
 void RefreshModuleVisuals();
 UFUNCTION() void OnRep_Modules();
 UFUNCTION(Server,Reliable) void ServerConfigureWeapon(uint8 Slot,FNRWeaponAssembly Build);
 UFUNCTION(Client,Reliable) void ClientModuleResult(uint8 Slot,FNRWeaponAssembly Build,bool Accepted);
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ModuleVisuals;
 double LastModuleRequest[2]={-100,-100};
 bool bModulesRestored=false;
 void PrimaryPressed();void PistolPressed();void KnifePressed();
 UFUNCTION(Server,Reliable)void ServerEquip(uint8 Slot);
 UFUNCTION()void OnRep_Equipped();
 void MeleeStrike();void ResolveMelee();
 UFUNCTION(NetMulticast,Unreliable)void MulticastMelee();
 UFUNCTION(Client,Reliable)void ClientVoice(uint8 Cue,uint8 VoiceRole);
 UPROPERTY()TObjectPtr<class UAudioComponent> DialogueAudio;
 int32 PrimaryAmmo=30,PrimaryReserve=90,PistolAmmo=12,PistolReserve=36;
 float WeaponReadyAt=0,LastEquip=-100,MeleeVisualAt=-100,LastVoice=-100;
 FTimerHandle MeleeTimer;
 void PlayReadyAnimation();
 void AimPressed();void AimReleased();
 UFUNCTION(Server,Reliable) void ServerConnectNodes(ANodeBase* From,ANodeBase* To);
 UFUNCTION(Server,Reliable) void ServerProgramNode(ANodeBase* Target,ENRNodeOp Operation);
 UFUNCTION(Server,Reliable) void ServerAim(bool Aiming);
 UFUNCTION(Server,Reliable) void ServerSelectClass(ENRRole NewRole);
 UFUNCTION() void OnRep_Team();
 bool IsUIBlocking() const;
 float AimAlpha=0,ReloadVisualTime=0;
 FRotator PreviousViewRotation;
 FVector2D Sway=FVector2D::ZeroVector;
 float Kick=0,VisualAmmo=30,VisualHealth=100,StepTime=0;
 uint32 ShotSequence=0;
 uint8 WorldWeaponSlot=255;
 bool bVisualsReady=false,bVisualReloading=false,bWorldAirborne=false,bWorldReloading=false,bWorldCrouched=false;
 void Forward(float V);void Right(float V);void Turn(float V);void Look(float V);
 void FirePressed();void FireReleased();void AbilityPressed();void InteractPressed();void ReloadPressed();void RolePressed();void LinkPressed();void OperationPressed();
 UFUNCTION(Server,Reliable)void ServerFire(bool bPressed);
 UFUNCTION(Server,Reliable)void ServerAbility();
 UFUNCTION(Server,Reliable)void ServerInteract();
 UFUNCTION(Server,Reliable)void ServerReload();
 UFUNCTION(Server,Reliable)void ServerChangeRole();
 UFUNCTION(Server,Reliable)void ServerLink();
 UFUNCTION(Server,Reliable)void ServerOperation();
 UFUNCTION(NetMulticast,Unreliable)void MulticastShot(FVector_NetQuantize Start,FVector_NetQuantize End,uint8 WeaponType);
 UFUNCTION()void OnRep_Role();
 UFUNCTION()void OnRep_Dead();
 bool TraceView(FHitResult& Hit,float Range)const;
 bool CanAct()const;void FireShot();void FinishReload();void InitASC();
 FTimerHandle FireTimer,ReloadTimer,CaptureTimer;
 TWeakObjectPtr<ANodeBase> LinkFrom;
 void SprintPressed();void SprintReleased();void CrouchPressed();void CrouchReleased();void JumpPressed();
 void RefillLoadout();
 UFUNCTION(Server,Reliable)void ServerSelectSkin(uint8 Index);
 UFUNCTION()void OnRep_Skin();
 UFUNCTION(NetMulticast,Unreliable)void MulticastFootstep(FVector_NetQuantize Position,bool Metal,bool Quiet);
 void InspectPressed();
 float InspectStart=-100;
 float LocalStepDistance=0,ServerStepDistance=0,LandingDip=0,SprintAlpha=0,LastDryFire=-100;
 int32 FootstepIndex=0;
 bool bSkinLoaded=false;
 float LastSkinRequest=-100;
 float LastShot=-100;float LastRequest=-100;bool bGranted=false;
};

