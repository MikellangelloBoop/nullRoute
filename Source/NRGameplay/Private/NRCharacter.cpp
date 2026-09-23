#include "NRCharacter.h"
#include "NRLocomotion.h"
#include "NRTacticalDoor.h"
#include "NRPreparationZone.h"
#include "Engine/DamageEvents.h"
#include "NRCharacterMovement.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundBase.h"
#include "NRCombatFX.h"
#include "NRHUD.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "NRBallisticsSubsystem.h"
#include "NRAttributes.h"
#include "NRAbilities.h"
#include "NRNode.h"
#include "NRGraphSubsystem.h"
#include "NRGameMode.h"
#include "NRObjective.h"
#include "NRActivity.h"
#include "NRStructure.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationInvokerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "NRAlgorithms.h"

ANRCharacter::ANRCharacter(const FObjectInitializer& O):Super(O.SetDefaultSubobjectClass<UNRCharacterMovement>(ACharacter::CharacterMovementComponentName)){
 BaseEyeHeight=64;CrouchedEyeHeight=40;
 GetMesh()->SetRelativeLocation(FVector(0,0,-88));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
 bReplicates=true;bReplicateUsingRegisteredSubObjectList=true;PrimaryActorTick.bCanEverTick=true;GetCapsuleComponent()->InitCapsuleSize(34,88);

 LeanHitbox=CreateDefaultSubobject<USphereComponent>(TEXT("LeanHead"));LeanHitbox->SetupAttachment(GetCapsuleComponent());LeanHitbox->SetSphereRadius(14);LeanHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);LeanHitbox->SetCollisionObjectType(ECC_Pawn);LeanHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);LeanHitbox->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);LeanHitbox->SetGenerateOverlapEvents(false);LeanHitbox->SetCanEverAffectNavigation(false);
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));Camera->SetupAttachment(GetCapsuleComponent());Camera->SetRelativeLocation(FVector(0,0,64));Camera->bUsePawnControlRotation=false;
 Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));Body->SetupAttachment(GetCapsuleComponent());Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Body->SetRelativeScale3D(FVector(.6,.6,1.5));Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->SetOwnerNoSee(true);
 Weapon=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));Weapon->SetupAttachment(Camera);Weapon->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Weapon->SetRelativeLocation(FVector(45,20,-18));Weapon->SetRelativeScale3D(FVector(.65,.1,.13));Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);Weapon->SetOnlyOwnerSee(true);
 ASC=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));ASC->SetIsReplicated(true);ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
 Attributes=CreateDefaultSubobject<UNRAttributes>(TEXT("Attributes"));Invoker=CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
}
UAbilitySystemComponent* ANRCharacter::GetAbilitySystemComponent()const{return ASC;}
void ANRCharacter::InitASC(){ASC->InitAbilityActorInfo(this,this);if(HasAuthority()&&!bGranted){bGranted=true;ASC->GiveAbility(FGameplayAbilitySpec(UGA_BuildNode::StaticClass(),1));ASC->GiveAbility(FGameplayAbilitySpec(UGA_PacketSniffer::StaticClass(),1));ASC->GiveAbility(FGameplayAbilitySpec(UGA_DirectionalBreach::StaticClass(),1));ASC->GiveAbility(FGameplayAbilitySpec(UGA_FieldTreatment::StaticClass(),1));}}
void ANRCharacter::PossessedBy(AController* C){Super::PossessedBy(C);InitASC();}
void ANRCharacter::OnRep_Controller(){Super::OnRep_Controller();InitASC();}
void ANRCharacter::BeginPlay()
{
    Super::BeginPlay();InitASC();
    if(GetNetMode()==NM_DedicatedServer){PrimaryActorTick.bStartWithTickEnabled=false;PrimaryActorTick.bAllowTickOnDedicatedServer=false;SetActorTickEnabled(false);GetMesh()->SetComponentTickEnabled(false);}
    else InitializeVisuals();
    OnRep_Role();
}
void ANRCharacter::EndPlay(EEndPlayReason::Type R){GetWorldTimerManager().ClearAllTimersForObject(this);Super::EndPlay(R);}
void ANRCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME_CONDITION(ANRCharacter,PracticeHits,COND_OwnerOnly);DOREPLIFETIME_CONDITION(ANRCharacter,ReplicatedLean,COND_SimulatedOnly);DOREPLIFETIME_CONDITION(ANRCharacter,DroneKills,COND_OwnerOnly);DOREPLIFETIME(ANRCharacter,PrimaryWeapon);DOREPLIFETIME(ANRCharacter,PrimaryAssembly);DOREPLIFETIME(ANRCharacter,PistolAssembly);DOREPLIFETIME(ANRCharacter,EquippedWeapon);DOREPLIFETIME_CONDITION(ANRCharacter,ReserveAmmo,COND_OwnerOnly);DOREPLIFETIME(ANRCharacter,WeaponSkin);DOREPLIFETIME(ANRCharacter,Team);DOREPLIFETIME(ANRCharacter,bAiming);DOREPLIFETIME(ANRCharacter,Kills);DOREPLIFETIME(ANRCharacter,Deaths);DOREPLIFETIME(ANRCharacter,OperatorRole);DOREPLIFETIME(ANRCharacter,bCarryingDisk);DOREPLIFETIME(ANRCharacter,bDead);
 DOREPLIFETIME_CONDITION(ANRCharacter,Ammo,COND_OwnerOnly);DOREPLIFETIME(ANRCharacter,bReloading);DOREPLIFETIME(ANRCharacter,ReloadStartedAt);DOREPLIFETIME_CONDITION(ANRCharacter,AbilityReadyTime,COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ANRCharacter,SpoofUntil,COND_OwnerOnly);DOREPLIFETIME_CONDITION(ANRCharacter,ScanResult,COND_OwnerOnly);DOREPLIFETIME_CONDITION(ANRCharacter,PrintingNode,COND_OwnerOnly);}
void ANRCharacter::SetupPlayerInputComponent(UInputComponent* I){Super::SetupPlayerInputComponent(I);
 I->BindAction("Ping",IE_Pressed,this,&ANRCharacter::PingPressed);I->BindAction("QuietWalk",IE_Pressed,this,&ANRCharacter::QuietPressed);I->BindAction("QuietWalk",IE_Released,this,&ANRCharacter::QuietReleased);
 I->BindAction("LeanLeft",IE_Pressed,this,&ANRCharacter::LeanLeftPressed);I->BindAction("LeanLeft",IE_Released,this,&ANRCharacter::LeanLeftReleased);I->BindAction("LeanRight",IE_Pressed,this,&ANRCharacter::LeanRightPressed);I->BindAction("LeanRight",IE_Released,this,&ANRCharacter::LeanRightReleased);I->BindAction("Workbench",IE_Pressed,this,&ANRCharacter::ToggleWorkbench).bExecuteWhenPaused=true;I->BindAction("Primary",IE_Pressed,this,&ANRCharacter::PrimaryPressed);I->BindAction("Pistol",IE_Pressed,this,&ANRCharacter::PistolPressed);I->BindAction("Knife",IE_Pressed,this,&ANRCharacter::KnifePressed);I->BindAction("Inspect",IE_Pressed,this,&ANRCharacter::InspectPressed);I->BindAction("Graph",IE_Pressed,this,&ANRCharacter::ToggleGraph);I->BindAction("Aim",IE_Pressed,this,&ANRCharacter::AimPressed);I->BindAction("Aim",IE_Released,this,&ANRCharacter::AimReleased);I->BindAction("Menu",IE_Pressed,this,&ANRCharacter::ToggleMenu).bExecuteWhenPaused=true;I->BindAxis("MoveForward",this,&ANRCharacter::Forward);I->BindAxis("MoveRight",this,&ANRCharacter::Right);I->BindAxis("Turn",this,&ANRCharacter::Turn);I->BindAxis("LookUp",this,&ANRCharacter::Look);
 I->BindAction("Fire",IE_Pressed,this,&ANRCharacter::FirePressed);I->BindAction("Fire",IE_Released,this,&ANRCharacter::FireReleased);I->BindAction("Ability",IE_Pressed,this,&ANRCharacter::AbilityPressed);I->BindAction("Interact",IE_Pressed,this,&ANRCharacter::InteractPressed);I->BindAction("Reload",IE_Pressed,this,&ANRCharacter::ReloadPressed);I->BindAction("Role",IE_Pressed,this,&ANRCharacter::RolePressed);I->BindAction("Link",IE_Pressed,this,&ANRCharacter::LinkPressed);I->BindAction("Operation",IE_Pressed,this,&ANRCharacter::OperationPressed);I->BindAction("Jump",IE_Pressed,this,&ANRCharacter::JumpPressed);I->BindAction("Sprint",IE_Pressed,this,&ANRCharacter::SprintPressed);I->BindAction("Sprint",IE_Released,this,&ANRCharacter::SprintReleased);I->BindAction("Crouch",IE_Pressed,this,&ANRCharacter::CrouchPressed);I->BindAction("Crouch",IE_Released,this,&ANRCharacter::CrouchReleased);I->BindAction("Jump",IE_Released,this,&ACharacter::StopJumping);}
bool ANRCharacter::IsAlive()const{return !bDead&&Attributes->GetHealth()>0;}
bool ANRCharacter::CanAct()const{auto* S=GetWorld()->GetGameState<ANRGameState>();return IsAlive()&&S&&(S->Phase==ENRPhase::Combat||S->Phase==ENRPhase::Preparation);}
void ANRCharacter::Forward(float V){if(IsAlive()&&!IsUIBlocking())AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V);}
void ANRCharacter::Right(float V){if(IsAlive()&&!IsUIBlocking())AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);}
void ANRCharacter::Turn(float V){AddControllerYawInput(V);}void ANRCharacter::Look(float V){AddControllerPitchInput(V);}
void ANRCharacter::FirePressed(){if(!IsUIBlocking()){
 InspectStart=-100;SprintReleased();if(EquippedWeapon!=2&&Ammo==0&&GetWorld()->GetTimeSeconds()-LastDryFire>.3f){LastDryFire=GetWorld()->GetTimeSeconds();if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_DryFire"),GetActorLocation(),.35f,true);}
 ServerFire(true);}}void ANRCharacter::FireReleased(){ServerFire(false);}void ANRCharacter::AbilityPressed(){if(!IsUIBlocking())ServerAbility();}void ANRCharacter::InteractPressed(){if(!IsUIBlocking())ServerInteract();}void ANRCharacter::ReloadPressed(){if(!IsUIBlocking())ServerReload();}void ANRCharacter::RolePressed(){if(!IsUIBlocking())ServerChangeRole();}void ANRCharacter::LinkPressed(){if(!IsUIBlocking())ServerLink();}void ANRCharacter::OperationPressed(){if(!IsUIBlocking())ServerOperation();}
void ANRCharacter::ServerFire_Implementation(bool Pressed){if(!Pressed){GetWorldTimerManager().ClearTimer(FireTimer);return;}if(!CanAct()||GetWorldTimerManager().IsTimerActive(FireTimer))return;CastChecked<UNRCharacterMovement>(GetCharacterMovement())->bWantsSprint=false;FireShot();if(!WeaponStats(EquippedWeapon).bAutomatic)return;GetWorldTimerManager().SetTimer(FireTimer,this,&ANRCharacter::FireShot,WeaponStats(EquippedWeapon).Interval,true);}
bool ANRCharacter::TraceView(FHitResult& H,float Range)const{FVector O;FRotator R;GetActorEyesViewPoint(O,R);FCollisionQueryParams P(SCENE_QUERY_STAT(NRAim),false,this);return GetWorld()->LineTraceSingleByChannel(H,O,O+R.Vector()*Range,ECC_Visibility,P);}
void ANRCharacter::FireShot(){if(EquippedWeapon==2){MeleeStrike();return;}auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();const float Now=GetWorld()->GetTimeSeconds();const float Interval=WeaponStats(EquippedWeapon).Interval;
 if(Now<WeaponReadyAt||!CanAct()||!GM||bReloading||IsSprinting()||Ammo<=0||Now-LastShot<Interval-.005f)return;LastShot=Now;--Ammo;
 FVector O;FRotator R;GetActorEyesViewPoint(O,R);FRandomStream ShotRandom(++ShotSequence*196613u);const auto Stats=WeaponStats(EquippedWeapon);const float Spread=(bAiming?Stats.AimSpread:(GetVelocity().SizeSquared2D()>100?1.1f:.55f)*Stats.HipSpreadScale);FVector D=ShotRandom.VRandCone(R.Vector(),FMath::DegreesToRadians(Spread));FCollisionQueryParams P(SCENE_QUERY_STAT(NRWeapon),true,this);P.bReturnPhysicalMaterial=true;
 FHitResult Hit;FVector End=O+D*15000.f;float Damage=Stats.Damage;
 // One ammo unit per shell; bounded pellet count and directions are entirely server-derived.
 for(int Pellet=0;Pellet<Stats.Pellets;++Pellet)
 {
  const FVector PelletDirection=Stats.Pellets>1?ShotRandom.VRandCone(R.Vector(),FMath::DegreesToRadians(Stats.PelletCone*(bAiming?.7f:1.f))):D;
  GetWorld()->GetSubsystem<UNRBallisticsSubsystem>()->Fire(this,O,PelletDirection,Damage,Stats.Speed);
 }
 MulticastShot(O,O+D*15000.f,EquippedWeapon==1?3:PrimaryKind()==ENRPrimaryWeapon::Forge12?4:PrimaryKind()==ENRPrimaryWeapon::Lancer60?5:PrimaryKind()==ENRPrimaryWeapon::Spectre?1:PrimaryKind()==ENRPrimaryWeapon::BR74?2:0);if(GM->IsCombat())ReportWeaponNoise(Assembly(EquippedWeapon).Muzzle==ENRMuzzle::Suppressor);
}
void ANRCharacter::MulticastShot_Implementation(FVector_NetQuantize Start,FVector_NetQuantize End,uint8 WeaponType)
{
    if(GetNetMode()==NM_DedicatedServer)return;
    LastPresentedShot=GetWorld()->GetTimeSeconds();
    const FVector Muzzle=MuzzlePosition();
    if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Shot(Muzzle,(End-Start).GetSafeNormal(),WeaponStats(EquippedWeapon).Speed,this,WeaponType,Assembly(EquippedWeapon).Muzzle==ENRMuzzle::Suppressor);
    if(IsLocallyControlled())
    {
        const float Recoil=WeaponStats(EquippedWeapon).RecoilScale;Kick=FMath::Min(Kick+Recoil,3.f);
        AddControllerPitchInput((EquippedWeapon==0&&PrimaryKind()==ENRPrimaryWeapon::Spectre?-.9f:bAiming?-.22f:-.4f)*Recoil);
    }
}
void ANRCharacter::MulticastImpact_Implementation(FVector_NetQuantize Position,FVector_NetQuantizeNormal Normal,bool Organic)
{
    if(GetNetMode()!=NM_DedicatedServer)if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Impact(Position,Normal,Organic);
}
void ANRCharacter::MulticastBreach_Implementation(FVector_NetQuantize Position)
{
 if(GetNetMode()!=NM_DedicatedServer)if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Breach(Position);
}
void ANRCharacter::ClientHitFeedback_Implementation(bool Killed)
{
    LastHitTime=GetWorld()->GetTimeSeconds();bLastHitKilled=Killed;if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(Killed?TEXT("S_KillConfirm"):TEXT("S_HitConfirm"),GetActorLocation(),Killed?.45f:.24f,true);
}
void ANRCharacter::ServerReload_Implementation(){if(EquippedWeapon==2||GetWorld()->GetTimeSeconds()<WeaponReadyAt||!CanAct()||bReloading||Ammo>=MagazineCapacity()||ReserveAmmo<=0)return;bReloading=true;ReloadStartedAt=GetWorld()->GetGameState<ANRGameState>()->GetServerWorldTimeSeconds();ForceNetUpdate();bAiming=false;Say(1);GetWorldTimerManager().ClearTimer(FireTimer);GetWorldTimerManager().SetTimer(ReloadTimer,this,&ANRCharacter::FinishReload,WeaponStats(EquippedWeapon).ReloadSeconds,false);}
void ANRCharacter::FinishReload(){
 if(HasAuthority()&&CanAct()){const int32 Transfer=FMath::Min(MagazineCapacity()-Ammo,ReserveAmmo);Ammo+=Transfer;ReserveAmmo-=Transfer;}
 bReloading=false;
}
void ANRCharacter::RefillLoadout(){if(HasAuthority()){
 GetWorldTimerManager().ClearTimer(ReloadTimer);GetWorldTimerManager().ClearTimer(FireTimer);GetWorldTimerManager().ClearTimer(MeleeTimer);
 bReloading=false;EquippedWeapon=0;PrimaryAmmo=WeaponStats(0).Capacity;PrimaryReserve=WeaponStats(0).Reserve;
 PistolAmmo=WeaponStats(1).Capacity;PistolReserve=36;Ammo=PrimaryAmmo;ReserveAmmo=PrimaryReserve;WeaponReadyAt=0;LastEquip=-100;OnRep_Equipped();}}
void ANRCharacter::ServerAbility_Implementation(){const float Now=GetWorld()->GetTimeSeconds();if(!CanAct()||Now<AbilityReadyTime||Now-LastRequest<.15f)return;LastRequest=Now;
 TSubclassOf<UGameplayAbility> Class=OperatorRole==ENRRole::Engineer?UGA_BuildNode::StaticClass():OperatorRole==ENRRole::Scout?UGA_PacketSniffer::StaticClass():OperatorRole==ENRRole::Medic?UGA_FieldTreatment::StaticClass():UGA_DirectionalBreach::StaticClass();ASC->TryActivateAbilityByClass(Class);}
bool ANRCharacter::BuildNode(){if(!HasAuthority()||!CanAct()||OperatorRole!=ENRRole::Engineer||(PrintingNode&&PrintingNode->NodeState==ENRNodeState::Printing))return false;
 auto* GS=GetWorld()->GetGameState<ANRGameState>();FHitResult H;if(!GS||!TraceView(H,600)||H.ImpactNormal.Z<.65f)return false;
 FVector Pos=H.ImpactPoint+FVector(0,0,65);if(!GS->CanPrepareAt(Team,Pos,100)||!GS->CanPrepareAt(Team,GetActorLocation()))return false;FCollisionQueryParams P;P.AddIgnoredActor(this);
 if(GetWorld()->OverlapBlockingTestByChannel(Pos,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(43,43,60)),P)||Attributes->GetHardwareTokens()<80)return false;
 if(!GS->SpendPower(Team,10))return false;
 auto* Node=GetWorld()->SpawnActorDeferred<ANodeBase>(ANodeBase::StaticClass(),FTransform(Pos),this,this,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 if(!Node){GS->SpendPower(Team,-10);return false;}Attributes->SetHardwareTokens(Attributes->GetHardwareTokens()-80);Node->Initialize(Team,ENRNodeOp::Sensor);Node->FinishSpawning(FTransform(Pos));Node->StartPrinting();PrintingNode=Node;Say(2);AbilityReadyTime=GetWorld()->GetTimeSeconds()+3.f;return true;
}
bool ANRCharacter::ScanNode(){auto* State=GetWorld()->GetGameState<ANRGameState>();if(!State||State->Phase!=ENRPhase::Combat)return false;if(!HasAuthority()||OperatorRole!=ENRRole::Scout||!CanAct())return false;FHitResult H;if(!TraceView(H,1500))return false;auto* N=Cast<ANodeBase>(H.GetActor());if(!N||N->Team==Team||N->NodeState==ENRNodeState::Destroyed)return false;
 Say(2);ScanResult.NodeId=N->NodeID;ScanResult.Team=N->Team;ScanResult.Operation=N->Operation;ScanResult.Delay=N->TriggerDelay;ScanResult.LinkCount=N->OutputPins.Num();ScanResult.ExpiresAt=GetWorld()->GetTimeSeconds()+8;SpoofUntil=GetWorld()->GetTimeSeconds()+6;AbilityReadyTime=GetWorld()->GetTimeSeconds()+5;
 TWeakObjectPtr<ANodeBase> Target=N;TWeakObjectPtr<ANRCharacter> Self=this;
 GetWorldTimerManager().SetTimer(CaptureTimer,[Self,Target](){auto* C=Self.Get();auto* Node=Target.Get();if(!C||!Node||!C->CanAct()||C->OperatorRole!=ENRRole::Scout)return;FHitResult Hit;if(C->TraceView(Hit,1500)&&Hit.GetActor()==Node)Node->Capture(C);},3.f,false);return true;
}
bool ANRCharacter::Breach(){if(!HasAuthority()||OperatorRole!=ENRRole::Assault||!CanAct()||Attributes->GetHardwareTokens()<150)return false;FHitResult H;if(!TraceView(H,250))return false;auto* S=Cast<ANRStructure>(H.GetActor());if(!S||S->bCollapsed)return false;
 const auto* State=GetWorld()->GetGameState<ANRGameState>();
 if(State&&State->Phase==ENRPhase::Preparation&&(Team==State->AttackTeam||!State->CanPrepareAt(Team,H.ImpactPoint)))return false;
 Say(2);Attributes->SetHardwareTokens(Attributes->GetHardwareTokens()-150);AbilityReadyTime=GetWorld()->GetTimeSeconds()+8;const FVector Origin=H.ImpactPoint,Direction=-H.ImpactNormal;TWeakObjectPtr<ANRStructure> Target=S;FTimerHandle Fuse;
 const int32 Round=GetWorld()->GetGameState<ANRGameState>()->Round;TWeakObjectPtr<ANRCharacter> Self=this;
 GetWorldTimerManager().SetTimer(Fuse,[Target,Origin,Direction,Round,Self](){if(auto* Structure=Target.Get()){const auto* GS=Structure->GetWorld()->GetGameState<ANRGameState>();if(!GS||GS->Round!=Round)return;Structure->BreachServer(Origin,Direction);if(Self.IsValid())Self->MulticastBreach(Origin);}},1.2f,false);return true;
}
void ANRCharacter::ServerInteract_Implementation(){if(!CanAct()||GetWorld()->GetTimeSeconds()-LastRequest<.15f)return;LastRequest=GetWorld()->GetTimeSeconds();FHitResult H;if(!TraceView(H,250))return;if(auto* Door=Cast<ANRTacticalDoor>(H.GetActor()))Door->Interact(this);else if(auto* Prep=Cast<ANRPreparationProp>(H.GetActor()))Prep->Interact(this);else if(auto* A=Cast<ANRActivity>(H.GetActor()))A->Interact(this);else if(auto* O=Cast<ANRObjective>(H.GetActor()))O->Interact(this);else if(auto* N=Cast<ANodeBase>(H.GetActor())){if(N->Team==255){N->Capture(this);}else if(N->Team==Team){N->WakeForMutation();N->NodeState=ENRNodeState::Active;N->FinishMutation();}}}
void ANRCharacter::ServerLink_Implementation(){if(!CanAct()||OperatorRole!=ENRRole::Engineer)return;FHitResult H;if(!TraceView(H,1500))return;auto* N=Cast<ANodeBase>(H.GetActor());if(!N||N->Team!=Team)return;
 if(LinkFrom.IsValid()){GetWorld()->GetSubsystem<UNRGraphSubsystem>()->Connect(LinkFrom.Get(),N,Team);LinkFrom.Reset();}else LinkFrom=N;}
void ANRCharacter::ServerOperation_Implementation(){if(!CanAct()||OperatorRole!=ENRRole::Engineer)return;FHitResult H;if(!TraceView(H,600))return;auto* N=Cast<ANodeBase>(H.GetActor());if(!N||N->Team!=Team)return;N->WakeForMutation();N->Operation=ENRNodeOp((uint8(N->Operation)+1)%7);++N->Revision;N->FinishMutation();}
void ANRCharacter::ServerChangeRole_Implementation(){auto* GS=GetWorld()->GetGameState<ANRGameState>();if(GS&&GS->Phase==ENRPhase::Preparation)SetRole(ENRRole((uint8(OperatorRole)+1)%4));}
void ANRCharacter::SetRole(ENRRole V){if(!HasAuthority()||uint8(V)>3)return;OperatorRole=V;auto* GS=GetWorld()->GetGameState<ANRGameState>();if(GS&&GS->Phase==ENRPhase::Preparation)RefillLoadout();OnRep_Role();OnRep_Team();Say(0);}
void ANRCharacter::OnRep_Role()
{
    if(!bVisualsReady)return;
    if(IsLocallyControlled())WorkshopStatus=NRWeaponModules::WeaponDescription(PrimaryKind());
    const TCHAR* Paths[]={TEXT("/Game/Art/Meshes/SM_MX9.SM_MX9"),TEXT("/Game/Art/Meshes/SM_Spectre.SM_Spectre"),TEXT("/Game/Art/Meshes/SM_AR7.SM_AR7"),TEXT("/Game/Art/Meshes/SM_MX9.SM_MX9")};
    auto* Rifle=LoadObject<UStaticMesh>(nullptr,EquippedWeapon==1?TEXT("/Game/Weapons/Pistol/Meshes/SM_Pistol.SM_Pistol"):EquippedWeapon==2?TEXT("/Game/Art/Meshes/SM_RouteKnife.SM_RouteKnife"):NRWeaponModules::Mesh(PrimaryKind()));
    if(Weapon->GetStaticMesh()!=Rifle)EquipPresentedAt=GetWorld()->GetTimeSeconds();
    Weapon->SetStaticMesh(Rifle);WorldWeapon->SetStaticMesh(Rifle);
    Weapon->SetRelativeScale3D(FVector(1));WorldWeapon->SetRelativeScale3D(FVector(1));
    TArray<UStaticMeshComponent*> Parts;GetComponents(Parts);for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("AMSPack")))Part->SetVisibility(!bDead&&OperatorRole==ENRRole::Engineer);
    Scope->SetVisibility(false);for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("RifleDetail")))Part->SetVisibility(!bDead&&EquippedWeapon==0);OnRep_Skin();RefreshModuleVisuals();RefreshAppearance();
}
void ANRCharacter::GiveCredits(int32 Amount){if(HasAuthority())Attributes->SetHardwareTokens(FMath::Clamp(Attributes->GetHardwareTokens()+Amount,0.f,9999.f));}
float ANRCharacter::TakeDamage(float Amount,FDamageEvent const& Event,AController* Killer,AActor* DamageCauser){if(!HasAuthority()||!IsAlive()||!FMath::IsFinite(Amount)||Amount<=0)return 0;const auto* GS=GetWorld()->GetGameState<ANRGameState>();if(GS&&GS->Phase==ENRPhase::Preparation)return 0;if(Event.IsOfType(FPointDamageEvent::ClassID))ClientDamageDirection(GetActorLocation()-static_cast<const FPointDamageEvent&>(Event).ShotDirection*1000);else if(DamageCauser)ClientDamageDirection(DamageCauser->GetActorLocation());float Absorb=FMath::Min(Attributes->GetArmorDurability(),Amount*.35f);ASC->ApplyModToAttribute(UNRAttributes::GetArmorDurabilityAttribute(),EGameplayModOp::Additive,-Absorb);ASC->ApplyModToAttribute(UNRAttributes::GetHealthAttribute(),EGameplayModOp::Additive,-(Amount-Absorb));
 if(Attributes->GetHealth()<=0){bDead=true;++Deaths;OnRep_Dead();GetCharacterMovement()->DisableMovement();GetWorldTimerManager().ClearAllTimersForObject(this);GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);if(auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>())GM->PlayerKilled(this,Killer);}return Amount;}
void ANRCharacter::ResetForRound(){if(!HasAuthority())return;ResetTacticalStance();PracticeHits=0;DroneKills=0;LastPing=-100;GetWorldTimerManager().ClearAllTimersForObject(this);GetWorldTimerManager().ClearTimer(CaptureTimer);LastShot=-100;LastRequest=-100;GetCharacterMovement()->MaxWalkSpeed=420;bAiming=false;bDead=false;bCarryingDisk=false;bReloading=false;RefillLoadout();UnCrouch();CastChecked<UNRCharacterMovement>(GetCharacterMovement())->bWantsSprint=false;AbilityReadyTime=0;SpoofUntil=0;PrintingNode=nullptr;LinkFrom.Reset();Attributes->SetHealth(100);Attributes->SetArmorDurability(100);GiveCredits(200);GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);GetCharacterMovement()->SetMovementMode(MOVE_Walking);OnRep_Dead();}

void ANRCharacter::OnRep_Dead()
{
    if(bDead&&!bPresentedDeath){PresentDeath();bPresentedDeath=true;}else if(!bDead)bPresentedDeath=false;
    if(bDead)ResetTacticalStance();
    GetCapsuleComponent()->SetCollisionEnabled(bDead?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
    if(bDead)GetCharacterMovement()->DisableMovement();else {GetCharacterMovement()->SetMovementMode(MOVE_Walking);GetCharacterMovement()->MaxWalkSpeed=420;}
    if(bVisualsReady){GetMesh()->SetVisibility(!bDead,true);ViewArms->SetVisibility(!bDead,true);Weapon->SetVisibility(!bDead,true);WorldWeapon->SetVisibility(!bDead,true);
        for(const auto& Part:ViewSleeves)Part->SetVisibility(!bDead);for(const auto& Part:ViewCuffs)Part->SetVisibility(!bDead);OnRep_Role();}
    Body->SetVisibility(false);
}

void ANRCharacter::InitializeVisuals()
{
    Body->SetVisibility(false);
    auto* Model=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Meshes/SKM_OperatorBody.SKM_OperatorBody"));
    GetMesh()->SetSkeletalMesh(Model);GetMesh()->SetRelativeLocation(FVector(0,0,-88));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetCastShadow(true);GetMesh()->SetCastHiddenShadow(true);
    GetMesh()->SetOwnerNoSee(true);GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetMesh()->SetCanEverAffectNavigation(false);
    GetMesh()->PlayAnimation(LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Art/Animations/BS_Operator2D.BS_Operator2D")),true);
    GetMesh()->TickAnimation(0,false);GetMesh()->RefreshBoneTransforms();
    auto* Helmet=NewObject<UStaticMeshComponent>(this);Helmet->SetupAttachment(GetMesh());
    Helmet->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_OperatorHelmet.SM_OperatorHelmet")));
    Helmet->AttachToComponent(GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,TEXT("NR_Helmet"));
    Helmet->ComponentTags.Add(TEXT("LegacyArmor"));Helmet->ComponentTags.Add(TEXT("FactionAccent"));Helmet->SetOwnerNoSee(true);Helmet->SetCastShadow(true);Helmet->SetCastHiddenShadow(true);
    Helmet->SetCollisionEnabled(ECollisionEnabled::NoCollision);Helmet->SetCanEverAffectNavigation(false);Helmet->RegisterComponent();
    for(int Part=0;Part<2;++Part)
    {
        auto* Rig=NewObject<UStaticMeshComponent>(this);Rig->SetupAttachment(GetMesh());
        Rig->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Part==0?TEXT("/Game/Art/Meshes/SM_AMSBackpack.SM_AMSBackpack"):TEXT("/Game/Art/Meshes/SM_ChestRig.SM_ChestRig")));
        Rig->SetRelativeLocation(Part==0?FVector(0,-21,127):FVector(0,15,131));Rig->SetRelativeRotation(Part==0?FRotator::ZeroRotator:FRotator(0,180,0));
        Rig->ComponentTags.Add(TEXT("LegacyArmor"));Rig->ComponentTags.Add(TEXT("FactionAccent"));if(Part==0)Rig->ComponentTags.Add(TEXT("AMSPack"));Rig->SetOwnerNoSee(true);Rig->SetCastShadow(true);Rig->SetCastHiddenShadow(true);Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rig->SetCanEverAffectNavigation(false);Rig->RegisterComponent();Rig->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepWorldTransform,TEXT("spine_04"));
    }
    auto* Rifle=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_AR7.SM_AR7"));
    Weapon->SetStaticMesh(Rifle);Weapon->SetRelativeLocation(FVector(35,15,-20));Weapon->SetRelativeRotation(FRotator(0,-90,0));Weapon->SetCastShadow(false);
    Weapon->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    WorldWeapon=NewObject<UStaticMeshComponent>(this,TEXT("WorldWeapon"));WorldWeapon->SetupAttachment(GetMesh(),TEXT("HandGrip_R"));
    WorldWeapon->SetStaticMesh(Rifle);WorldWeapon->SetCastShadow(true);WorldWeapon->SetCastHiddenShadow(true);WorldWeapon->SetOwnerNoSee(true);WorldWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);WorldWeapon->SetCanEverAffectNavigation(false);WorldWeapon->RegisterComponent();
    ViewArms=NewObject<USkeletalMeshComponent>(this,TEXT("ViewArms"));ViewArms->SetupAttachment(Camera);auto* ArmsModel=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Meshes/SKM_OperatorArms.SKM_OperatorArms"));ViewArms->SetSkeletalMesh(ArmsModel?ArmsModel:Model);
    ViewArms->SetRelativeLocation(FVector(30,0,-158));ViewArms->SetRelativeRotation(FRotator(0,-90,0));
    ViewArms->SetOnlyOwnerSee(true);ViewArms->SetCastShadow(false);ViewArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);ViewArms->SetCanEverAffectNavigation(false);
    ViewArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);ViewArms->RegisterComponent();
    ViewArms->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Art/Animations/A_FP_Ready.A_FP_Ready")),true);
    Weapon->AttachToComponent(ViewArms,FAttachmentTransformRules::SnapToTargetNotIncludingScale,TEXT("HandGrip_R"));Weapon->SetRelativeLocation(FVector::ZeroVector);Weapon->SetRelativeRotation(FRotator::ZeroRotator);
    ViewArms->HideBoneByName(TEXT("neck_01"),PBO_None);ViewArms->HideBoneByName(TEXT("thigh_l"),PBO_None);ViewArms->HideBoneByName(TEXT("thigh_r"),PBO_None);
    Scope=NewObject<UStaticMeshComponent>(this,TEXT("Optic"));Scope->SetupAttachment(Weapon);
    Scope->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_Suppressor.SM_Suppressor")));
    Scope->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Graphite.M_Graphite")));
    Scope->SetRelativeLocation(FVector(0,56,4));Scope->SetRelativeRotation(FRotator::ZeroRotator);Scope->SetRelativeScale3D(FVector(1));
    Scope->SetOnlyOwnerSee(true);Scope->SetCastShadow(false);Scope->SetCollisionEnabled(ECollisionEnabled::NoCollision);Scope->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);Scope->RegisterComponent();
    for(int I=0;I<2;++I)for(int Part=0;Part<2;++Part)
    {
        auto* Piece=NewObject<UStaticMeshComponent>(this);Piece->SetupAttachment(Camera);
        Piece->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Part==0?TEXT("/Game/Art/Meshes/SM_FPSleeve.SM_FPSleeve"):TEXT("/Game/Art/Meshes/SM_FPCuff.SM_FPCuff")));
        Piece->SetOnlyOwnerSee(true);Piece->SetCastShadow(false);Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);Piece->SetCanEverAffectNavigation(false);
        Piece->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);Piece->RegisterComponent();
        (Part==0?ViewSleeves:ViewCuffs).Add(Piece);
    }
    for(int I=0;I<2;++I)
    {
        auto* Detail=NewObject<UStaticMeshComponent>(this);Detail->SetupAttachment(I==0?Weapon.Get():WorldWeapon.Get());
        Detail->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_RifleHardware.SM_RifleHardware")));
        Detail->ComponentTags.Add(TEXT("RifleDetail"));Detail->SetOnlyOwnerSee(I==0);Detail->SetOwnerNoSee(I!=0);Detail->SetCastShadow(I!=0);Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);Detail->SetCanEverAffectNavigation(false);
        if(I==0)Detail->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);Detail->RegisterComponent();
    }
    BuildPreview=NewObject<UStaticMeshComponent>(this,TEXT("PrintPreview"));BuildPreview->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_Terminal.SM_Terminal")));
    BuildPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);BuildPreview->SetCanEverAffectNavigation(false);BuildPreview->SetCastShadow(false);BuildPreview->SetOnlyOwnerSee(true);BuildPreview->RegisterComponent();BuildPreview->SetVisibility(false);for(int32 I=0;I<BuildPreview->GetNumMaterials();++I)BuildPreview->SetMaterial(I,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Polymer.M_Polymer")));
    Camera->SetFieldOfView(92);Camera->SetEnableFirstPersonScale(true);Camera->SetFirstPersonScale(.25f);
    Camera->SetEnableFirstPersonFieldOfView(true);Camera->SetFirstPersonFieldOfView(80);
    // A small camera fill lights only the local weapon/arms on channel 1. It never lights the arena,
    // casts shadows, replicates, or exists on a dedicated server.
    TArray<UPrimitiveComponent*> ViewParts;GetComponents(ViewParts);
    for(auto* Part:ViewParts)if(Part->bOnlyOwnerSee&&Part!=BuildPreview)Part->SetLightingChannels(true,true,false);
    ViewFill=NewObject<UPointLightComponent>(this,TEXT("ViewFill"));ViewFill->SetupAttachment(Camera);
    ViewFill->SetRelativeLocation(FVector(8,-22,20));ViewFill->SetIntensityUnits(ELightUnits::Candelas);ViewFill->SetIntensity(.6f);
    ViewFill->SetAttenuationRadius(200);ViewFill->SetLightColor(FLinearColor(.82f,.92f,1.f));ViewFill->SetCastShadows(false);
    ViewFill->SetLightingChannels(false,true,false);ViewFill->SetVisibility(IsLocallyControlled());ViewFill->RegisterComponent();
    bVisualsReady=true;OnRep_Team();OnRep_Role();
}

void ANRCharacter::Tick(float Dt)
{
    Super::Tick(Dt);if(!bVisualsReady)return;
    if(ViewFill)ViewFill->SetVisibility(IsLocallyControlled()&&!bDead);
    if(CachedFaction!=GetFaction()||CachedAppearanceRole!=OperatorRole||CachedAppearanceTeam!=Team)RefreshAppearance();
    UpdateStanceVisuals(Dt);

    // Native pose graph handles directional motion, upper-body reload and remote aim.
    if(!IsLocallyControlled())return;
    RestoreWeaponModules();
    if(!bSkinLoaded){bSkinLoaded=true;int32 Saved=0;GConfig->GetInt(TEXT("NullRoute.Cosmetics"),TEXT("WeaponSkin"),Saved,GGameUserSettingsIni);SelectSkin(uint8(FMath::Clamp(Saved,0,3)));}
    UpdateFootsteps(Dt,false);
    SprintAlpha=FMath::FInterpTo(SprintAlpha,IsSprinting()?1.f:0.f,Dt,9);
    LandingDip=FMath::FInterpTo(LandingDip,0.f,Dt,13);
    const FVector Offset=LeanOffset(ConstrainLean(GetLeanAmount()));FVector CameraPos=GetActorTransform().InverseTransformVectorNoScale(Offset);CameraPos.Z=FMath::FInterpTo(Camera->GetRelativeLocation().Z,(bIsCrouched?40.f:64.f)+Offset.Z,Dt,16);Camera->SetRelativeLocation(CameraPos);
    Camera->SetWorldRotation(LeanViewRotation());
    Scope->SetVisibility(false);
    UpdateHealthFeedback();
    if(bReloading!=bVisualReloading)
    {
        if(bReloading)if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_Reload"),GetActorLocation(),.5f,true);
        PlayReadyAnimation();
        if(bReloading)if(auto* A=ViewArms->GetSingleNodeInstance())if(auto* Clip=Cast<UAnimSequence>(A->GetCurrentAsset()))A->SetPlayRate(Clip->GetPlayLength()/WeaponStats(EquippedWeapon).ReloadSeconds);
        ReloadVisualTime=bReloading?FMath::Max(0.f,GetWorld()->GetGameState<ANRGameState>()->GetServerWorldTimeSeconds()-ReloadStartedAt):0;
        if(bReloading)if(auto* A=ViewArms->GetSingleNodeInstance())if(auto* Clip=Cast<UAnimSequence>(A->GetCurrentAsset()))A->SetPosition(FMath::Clamp(ReloadVisualTime/WeaponStats(EquippedWeapon).ReloadSeconds,0.f,1.f)*Clip->GetPlayLength(),false);
        bVisualReloading=bReloading;
    }
    Kick=FMath::FInterpTo(Kick,0,Dt,12);
    const float Speed=GetVelocity().Size2D();const float Time=GetWorld()->GetTimeSeconds();
    const float EquipDip=1-FMath::SmoothStep(0.f,.35f,Time-EquipPresentedAt);
    const float Bob=FMath::Sin(Time*(IsSprinting()?14:10))*FMath::Min(Speed/420.f,1.3f)*(bAiming?.035f:bIsCrouched?.16f:.38f);
    // One local cosmetic transform; server ballistics always use the authoritative eye ray.
    AimAlpha=FMath::FInterpTo(AimAlpha,bAiming&&!bReloading?1.f:0.f,Dt,WeaponStats(EquippedWeapon).AimRate);
    if(bReloading)ReloadVisualTime+=Dt;
    const float ReloadTilt=bReloading?FMath::Sin(FMath::Clamp(ReloadVisualTime/WeaponStats(EquippedWeapon).ReloadSeconds,0.f,1.f)*PI):0;
    const FRotator Control=GetControlRotation();
    const FRotator Delta=(Control-PreviousViewRotation).GetNormalized();PreviousViewRotation=Control;
    Sway=FMath::Vector2DInterpTo(Sway,FVector2D(FMath::Clamp(Delta.Yaw/FMath::Max(Dt,.001f)/60.f,-3.f,3.f),FMath::Clamp(Delta.Pitch/FMath::Max(Dt,.001f)/60.f,-3.f,3.f)),Dt,9);
    const float Inspect=(!bReloading&&!bAiming&&!IsSprinting())?FMath::Sin(FMath::Clamp((Time-InspectStart)/2.f,0.f,1.f)*PI):0.f;
    const FTransform Grip=ViewArms->GetSocketTransform(TEXT("HandGrip_R"),RTS_Component);
    const float Slash=EquippedWeapon==2?FMath::Sin(FMath::Clamp((Time-MeleeVisualAt)/.48f,0.f,1.f)*PI):0;
    const FQuat GunRotation=FRotator(-Kick*.7f-ReloadTilt*3-SprintAlpha*14+Slash*26,(EquippedWeapon==2?-130.f:-90.f)+Sway.X*.35f*(1-AimAlpha)-SprintAlpha*9+Inspect*48-Slash*55,(EquippedWeapon==2?-45.f:0.f)+ReloadTilt*12+SprintAlpha*15-Inspect*23+Slash*35).Quaternion();
    const FQuat HandsRotation=GunRotation*Grip.GetRotation().Inverse();
    const FVector GunBase=EquippedWeapon==0?FMath::Lerp(FVector(55,17,-23),FVector(38,0,-15.8),AimAlpha):FMath::Lerp(FVector(49,14,-18),FVector(38,0,-12.6),AimAlpha);
    const FVector GunPosition=GunBase-FVector(0,0,EquippedWeapon<2&&Assembly(EquippedWeapon).Optic!=ENROptic::Iron?4*AimAlpha:0)+FVector(-Kick*1.7f,-Sway.X*.25f*(1-AimAlpha),Bob*(1-AimAlpha)-Kick*.3f-ReloadTilt*4-SprintAlpha*6-LandingDip+Inspect*5-EquipDip*12);
    ViewArms->SetRelativeRotation(HandsRotation);
    ViewArms->SetRelativeLocation(GunPosition-HandsRotation.RotateVector(Grip.GetLocation()));
    for(int I=0;I<2;++I)
    {
        const FVector Wrist=Camera->GetComponentTransform().InverseTransformPosition(ViewArms->GetBoneLocation(I==0?TEXT("hand_l"):TEXT("hand_r")));
        const FVector Elbow=I==0?FVector(5,-26,-39):FVector(5,29,-39);
        const FVector Arm=Wrist-Elbow;const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Arm.GetSafeNormal());
        ViewSleeves[I]->SetRelativeTransform(FTransform(Rotation,Elbow,FVector(1,1,Arm.Size()/100.f)));
        ViewCuffs[I]->SetRelativeTransform(FTransform(Rotation,Wrist-Arm.GetSafeNormal()*2.2f));
    }
    Camera->SetFieldOfView(FMath::Lerp(92.f+SprintAlpha*5,WeaponStats(EquippedWeapon).AimFOV,AimAlpha));
    Camera->SetFirstPersonFieldOfView(FMath::Lerp(80.f,78.f,AimAlpha));
    bool Show=false;
    if(OperatorRole==ENRRole::Engineer&&IsAlive()&&!bAiming&&(!PrintingNode||PrintingNode->NodeState!=ENRNodeState::Printing))
    {
        FHitResult H;if(TraceView(H,600)&&H.ImpactNormal.Z>.65f)
        {
            const FVector Pos=H.ImpactPoint+FVector(0,0,65);FCollisionQueryParams Q;Q.AddIgnoredActor(this);
            if(!GetWorld()->OverlapBlockingTestByChannel(Pos,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(43,43,60)),Q))
            {BuildPreview->SetWorldLocation(Pos);Show=true;}
        }
    }
    BuildPreview->SetVisibility(Show);
}

FVector ANRCharacter::MuzzlePosition() const
{
    if(bVisualsReady)
    {
        if(IsLocallyControlled())return Weapon->GetComponentTransform().TransformPosition(FVector(0,(EquippedWeapon==1?23:51)+(EquippedWeapon<2&&Assembly(EquippedWeapon).Muzzle==ENRMuzzle::Suppressor?16:EquippedWeapon<2&&Assembly(EquippedWeapon).Muzzle==ENRMuzzle::Compensator?5:0),4));
        return WorldWeapon->GetComponentTransform().TransformPosition(FVector(0,(EquippedWeapon==1?23:51)+(EquippedWeapon<2&&Assembly(EquippedWeapon).Muzzle==ENRMuzzle::Suppressor?16:EquippedWeapon<2&&Assembly(EquippedWeapon).Muzzle==ENRMuzzle::Compensator?5:0),4));
    }
    return GetActorLocation()+GetControlRotation().Vector()*60+FVector(0,0,64);
}
void ANRCharacter::OnRep_Team()
{
    if(!bVisualsReady)return;
    RefreshAppearance();
}
void ANRCharacter::AimPressed(){if(EquippedWeapon!=2&&IsAlive()&&!bReloading&&!IsUIBlocking()){SprintReleased();bAiming=true;GetCharacterMovement()->MaxWalkSpeed=280;ServerAim(true);}}
void ANRCharacter::AimReleased(){bAiming=false;GetCharacterMovement()->MaxWalkSpeed=420;ServerAim(false);}
void ANRCharacter::ServerAim_Implementation(bool Aiming){bAiming=Aiming&&EquippedWeapon!=2&&IsAlive()&&!bReloading;GetCharacterMovement()->MaxWalkSpeed=bAiming?280:420;}
void ANRCharacter::SelectClass(ENRRole NewRole){ServerSelectClass(NewRole);}
void ANRCharacter::ServerSelectClass_Implementation(ENRRole NewRole)
{
    auto* GS=GetWorld()->GetGameState<ANRGameState>();
    if(uint8(NewRole)<=3&&GS&&GS->Phase==ENRPhase::Preparation){SetRole(NewRole);}
}
void ANRCharacter::ToggleMenu()
{
    FireReleased();AimReleased();SprintReleased();StopJumping();ResetTacticalStance();
    if(auto* PC=Cast<APlayerController>(Controller))if(auto* HUD=Cast<ANRHUD>(PC->GetHUD()))HUD->ToggleMenu();
}

void ANRCharacter::ToggleGraph(){FireReleased();AimReleased();SprintReleased();StopJumping();ResetTacticalStance();if(auto* PC=Cast<APlayerController>(Controller))if(auto* HUD=Cast<ANRHUD>(PC->GetHUD()))HUD->ToggleGraph();}
void ANRCharacter::ConnectNodes(ANodeBase* From,ANodeBase* To){ServerConnectNodes(From,To);}
void ANRCharacter::ProgramNode(ANodeBase* Target,ENRNodeOp Operation){ServerProgramNode(Target,Operation);}
void ANRCharacter::ServerConnectNodes_Implementation(ANodeBase* From,ANodeBase* To)
{
 if(!CanAct()||OperatorRole!=ENRRole::Engineer||!From||!To||FVector::DistSquared(GetActorLocation(),From->GetActorLocation())>2500.f*2500.f||GetWorld()->GetTimeSeconds()-LastRequest<.15f)return;
 LastRequest=GetWorld()->GetTimeSeconds();GetWorld()->GetSubsystem<UNRGraphSubsystem>()->Connect(From,To,Team);
}
void ANRCharacter::ServerProgramNode_Implementation(ANodeBase* Target,ENRNodeOp Operation)
{
 if(!CanAct()||OperatorRole!=ENRRole::Engineer||!Target||Target->Team!=Team||uint8(Operation)>6||Target->NodeState==ENRNodeState::Destroyed||FVector::DistSquared(GetActorLocation(),Target->GetActorLocation())>2500.f*2500.f||GetWorld()->GetTimeSeconds()-LastRequest<.15f)return;
 LastRequest=GetWorld()->GetTimeSeconds();Target->WakeForMutation();Target->Operation=Operation;++Target->Revision;Target->FinishMutation();
}

bool ANRCharacter::IsUIBlocking() const
{
 if(auto* PC=Cast<APlayerController>(Controller))if(auto* H=Cast<ANRHUD>(PC->GetHUD()))return H->IsMenuOpen();
 return false;
}

bool ANRCharacter::IsSprinting()const{return CastChecked<UNRCharacterMovement>(GetCharacterMovement())->IsSprinting();}
void ANRCharacter::SprintPressed(){if(IsAlive()&&!IsUIBlocking()){AimReleased();CastChecked<UNRCharacterMovement>(GetCharacterMovement())->bWantsSprint=true;}}
void ANRCharacter::SprintReleased(){CastChecked<UNRCharacterMovement>(GetCharacterMovement())->bWantsSprint=false;}
void ANRCharacter::CrouchPressed(){if(IsAlive()&&!IsUIBlocking()){SprintReleased();Crouch();}}
void ANRCharacter::CrouchReleased(){UnCrouch();}
void ANRCharacter::JumpPressed(){if(IsAlive()&&!IsUIBlocking()&&!bIsCrouched){bLeanLeftHeld=bLeanRightHeld=false;RefreshLeanInput();Jump();}}
void ANRCharacter::OnStartCrouch(float A,float B){Super::OnStartCrouch(A,B);if(IsLocallyControlled())Camera->AddRelativeLocation(FVector(0,0,B));}
void ANRCharacter::OnEndCrouch(float A,float B){Super::OnEndCrouch(A,B);if(IsLocallyControlled())Camera->AddRelativeLocation(FVector(0,0,-B));}
void ANRCharacter::Landed(const FHitResult& Hit)
{
 const float Fall=FMath::Abs(GetVelocity().Z);Super::Landed(Hit);
 if(IsLocallyControlled()&&Fall>220){LandingDip=FMath::Clamp(Fall/150.f,1.5f,6.f);if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_Land"),GetActorLocation(),FMath::Clamp(Fall/1100.f,.2f,.6f),true);}
}
void ANRCharacter::UpdateFootsteps(float Dt,bool Server)
{
 float& Distance=Server?ServerStepDistance:LocalStepDistance;
 if(!IsAlive()||!GetCharacterMovement()->IsMovingOnGround()||GetVelocity().SizeSquared2D()<2500){Distance=0;return;}
 const bool Silent=IsQuietWalking();Distance+=GetVelocity().Size2D()*Dt;const float Stride=Silent?155:bIsCrouched?165:IsSprinting()?195:180;
 if(Distance<Stride)return;Distance=FMath::Fmod(Distance,Stride);
 const FHitResult& Floor=GetCharacterMovement()->CurrentFloor.HitResult;
 const bool Metal=Floor.GetActor()&&Floor.GetActor()->ActorHasTag(TEXT("SurfaceMetal"));
 if(Server){MulticastFootstep(GetActorLocation(),Metal,Silent||bIsCrouched);return;}
 if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(FName(*FString::Printf(TEXT("S_Step%s%d"),Metal?TEXT("Metal"):TEXT("Concrete"),(FootstepIndex++%4)+1)),GetActorLocation(),Silent?.035f:bIsCrouched?.075f:IsSprinting()?.32f:.21f,true,Silent);
}
void ANRCharacter::MulticastFootstep_Implementation(FVector_NetQuantize P,bool Metal,bool Quiet)
{
 if(GetNetMode()==NM_DedicatedServer||IsLocallyControlled())return;
 if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(FName(*FString::Printf(TEXT("S_Step%s%d"),Metal?TEXT("Metal"):TEXT("Concrete"),(FootstepIndex++%4)+1)),P,Quiet?.035f:.32f,false,Quiet);
}
void ANRCharacter::SelectSkin(uint8 Index){if(Index>3)return;GConfig->SetInt(TEXT("NullRoute.Cosmetics"),TEXT("WeaponSkin"),Index,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);ServerSelectSkin(Index);}
void ANRCharacter::ServerSelectSkin_Implementation(uint8 Index)
{
 if(Index>3||GetWorld()->GetRealTimeSeconds()-LastSkinRequest<.15f)return;LastSkinRequest=GetWorld()->GetRealTimeSeconds();WeaponSkin=Index;OnRep_Skin();
}
void ANRCharacter::OnRep_Skin()
{
 if(!bVisualsReady)return;
 Weapon->EmptyOverrideMaterials();WorldWeapon->EmptyOverrideMaterials();if(EquippedWeapon!=0)return;
 if(OperatorRole==ENRRole::Assault){RefreshAppearance();return;}
 const TCHAR* Names[]={TEXT("M_SkinCarbon"),TEXT("M_SkinCeramic"),TEXT("M_SkinHazard"),TEXT("M_SkinPhantom")};
 if(auto* M=LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Art/Materials/"))+Names[FMath::Min(3,int(WeaponSkin))])))
 {for(int I=0;I<Weapon->GetNumMaterials();++I)Weapon->SetMaterial(I,M);for(int I=0;I<WorldWeapon->GetNumMaterials();++I)WorldWeapon->SetMaterial(I,M);}
}

void ANRCharacter::InspectPressed(){if(IsAlive()&&!IsUIBlocking()&&!bReloading){InspectStart=GetWorld()->GetTimeSeconds();}}

void ANRCharacter::ClientDamageDirection_Implementation(FVector_NetQuantize Source)
{DamageSourceYaw=(Source-GetActorLocation()).Rotation().Yaw;LastDamageTime=GetWorld()->GetTimeSeconds();}
void ANRCharacter::MulticastRicochet_Implementation(FVector_NetQuantize Origin,FVector_NetQuantizeNormal Direction,float Speed)
{if(GetNetMode()!=NM_DedicatedServer)if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Ricochet(Origin,Direction,Speed,this);}

void ANRCharacter::UpdateHealthFeedback()
{
 if(!IsLocallyControlled()||GetNetMode()==NM_DedicatedServer)return;
 const float Health=Attributes->GetHealth();
 // Read the replicated GAS health value. No extra unreliable RPC and no dependency on a damage causer.
 if(bHealthFeedbackReady&&Health<VisualHealth-.01f)
 {
  LastDamageTime=GetWorld()->GetTimeSeconds();
  if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->DamageFeedback(VisualHealth-Health,Health);
 }
 VisualHealth=Health;bHealthFeedbackReady=true;
}

void ANRCharacter::ConstrainPreparationMovement()
{
 const auto* S=GetWorld()->GetGameState<ANRGameState>();
 if(!S||S->Phase!=ENRPhase::Preparation||Team>1||!IsAlive()||GetLocalRole()==ROLE_SimulatedProxy)return;
 const float Margin=GetCapsuleComponent()->GetScaledCapsuleRadius()+14.f;
 FVector At=GetActorLocation();if(S->CanPrepareAt(Team,At,Margin))return;
 // Server-side backstop also stops climbing/jumping around the preparation wall.
 At.X=S->PreparationBoundaryX+(Team==S->AttackTeam?-Margin:Margin);
 SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
 GetCharacterMovement()->Velocity.X=0;
}
void ANRCharacter::RefillAmmunition()
{
 if(!HasAuthority()||!IsAlive())return;const uint8 Slot=EquippedWeapon;
 RefillLoadout();EquippedWeapon=Slot;
 Ammo=Slot==0?PrimaryAmmo:Slot==1?PistolAmmo:0;ReserveAmmo=Slot==0?PrimaryReserve:Slot==1?PistolReserve:0;
 LastShot=-100;WeaponReadyAt=0;OnRep_Equipped();ForceNetUpdate();
}
