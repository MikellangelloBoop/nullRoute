#include "NRTacticalDoor.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRCombatFX.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANRTacticalDoor::ANRTacticalDoor()
{
 ConsoleCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("ConsoleCollision"));SetRootComponent(ConsoleCollision);
 ConsoleCollision->SetBoxExtent({30,30,55});ConsoleCollision->SetCollisionProfileName(TEXT("BlockAll"));
 GateCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("GateCollision"));GateCollision->SetupAttachment(ConsoleCollision);
 GateCollision->SetCollisionProfileName(TEXT("BlockAll"));GateCollision->SetCanEverAffectNavigation(true);
 ConsoleMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));ConsoleMesh->SetupAttachment(ConsoleCollision);
 Panel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shutter"));Panel->SetupAttachment(ConsoleCollision);
 for(auto* C:{ConsoleMesh.Get(),Panel.Get()}){C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);}
}
void ANRTacticalDoor::BeginPlay()
{
 Super::BeginPlay();GateCollision->SetBoxExtent(GateExtent);GateCollision->SetRelativeLocation(GateOffset);
 if(GetNetMode()!=NM_DedicatedServer)
 {
  ConsoleMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_TacticalConsole")));ConsoleMesh->SetRelativeLocation({0,0,-90});
  Panel->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube")));
  Panel->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Graphite")));
  Panel->SetRelativeScale3D(GateExtent*2/100);
 }
 OpenAmount=bOpen?1:0;OnRep_Door();
}
void ANRTacticalDoor::EndPlay(EEndPlayReason::Type R){GetWorldTimerManager().ClearAllTimersForObject(this);Super::EndPlay(R);}
void ANRTacticalDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRTacticalDoor,bOpen);DOREPLIFETIME(ANRTacticalDoor,ReadyAt);}
bool ANRTacticalDoor::Occupied()const
{
 TArray<FOverlapResult> Hits;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRShutter),false,this);
 GetWorld()->OverlapMultiByObjectType(Hits,GetActorTransform().TransformPosition(GateOffset),GetActorQuat(),FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeBox(GateExtent+FVector(40,0,0)),Q);
 for(const auto& H:Hits)if(const auto* P=Cast<ANRCharacter>(H.GetActor()))if(P->IsAlive())return true;
 return false;
}
bool ANRTacticalDoor::Interact(ANRCharacter* P)
{
 const auto* GS=GetWorld()->GetGameState<ANRGameState>();
 if(!HasAuthority()||!P||!P->IsAlive()||!GS||(GS->Phase!=ENRPhase::Combat&&GS->Phase!=ENRPhase::Preparation)||!GS->CanPrepareAt(P->Team,GetActorLocation())||!GS->CanPrepareAt(P->Team,P->GetActorLocation())||GS->GetServerWorldTimeSeconds()<ReadyAt)return false;
 if(FVector::DistSquared(P->GetPawnViewLocation(),GetActorLocation())>250*250)return false;
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRDoorUse),false,P);
 if(GetWorld()->LineTraceSingleByChannel(Hit,P->GetPawnViewLocation(),GetActorLocation(),ECC_Visibility,Q)&&Hit.GetActor()!=this)return false;
 if(bOpen&&Occupied())return false;
 WakeForMutation();bOpen=!bOpen;ReadyAt=GS->GetServerWorldTimeSeconds()+6;OnRep_Door();ForceNetUpdate();return true;
}
void ANRTacticalDoor::OnRep_Door()
{
 if(bOpen)GateCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 if(GetNetMode()!=NM_DedicatedServer)
 {
  ConsoleMesh->SetMaterial(3,LoadObject<UMaterialInterface>(nullptr,bOpen?TEXT("/Game/Art/Materials/M_CyanLight"):TEXT("/Game/Art/Materials/M_AmberLight")));
  if(FMath::Abs(OpenAmount-(bOpen?1:0))>.01f)if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_Equip"),GetActorLocation(),.5f,false);
 }
 AnimateDoor();if(FMath::Abs(OpenAmount-(bOpen?1:0))>.001f)GetWorldTimerManager().SetTimer(MotionTimer,this,&ANRTacticalDoor::AnimateDoor,.033f,true);
}
void ANRTacticalDoor::AnimateDoor()
{
 // A bounded timer runs only during the 0.7s movement. Closing aborts if a player enters the opening.
 if(HasAuthority()&&!bOpen&&OpenAmount>0&&Occupied()){bOpen=true;ForceNetUpdate();}
 OpenAmount=FMath::FInterpConstantTo(OpenAmount,bOpen?1.f:0.f,.033f,1.45f);
 Panel->SetRelativeLocation(GateOffset+FVector(0,0,OpenAmount*(GateExtent.Z*2+12)));
 const bool Closed=!bOpen&&OpenAmount<.001f;
 GateCollision->SetCollisionEnabled(Closed?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 if(FMath::Abs(OpenAmount-(bOpen?1:0))<.001f){GetWorldTimerManager().ClearTimer(MotionTimer);FinishMutation();}
}
void ANRTacticalDoor::ResetDoor(){if(!HasAuthority())return;WakeForMutation();ReadyAt=0;bOpen=true;OnRep_Door();ForceNetUpdate();}
FString ANRTacticalDoor::Prompt()const
{
 const auto* GS=GetWorld()->GetGameState<ANRGameState>();const float Left=GS?ReadyAt-GS->GetServerWorldTimeSeconds():0;
 if(Left>0)return FString::Printf(TEXT("ЗАСЛОНКА / ПРИВОД ОСТЫНЕТ ЧЕРЕЗ %.0f С"),FMath::CeilToFloat(Left));
 return bOpen?TEXT("F  ЗАКРЫТЬ ЗАСЛОНКУ / ИЗМЕНИТЬ МАРШРУТ"):TEXT("F  ОТКРЫТЬ ЗАСЛОНКУ / ПРОХОД");
}
