#include "NRObjective.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
ANRObjective::ANRObjective(){SetReplicateMovement(true);Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));SetRootComponent(Mesh);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetRelativeScale3D(FVector(.55,.45,.15));Mesh->SetCollisionProfileName(TEXT("BlockAll"));}
void ANRObjective::BeginPlay(){Super::BeginPlay();HomePosition=GetActorLocation();}
void ANRObjective::DropAt(FVector Position){if(!HasAuthority())return;WakeForMutation();SetActorLocation(Position);bTaken=false;OnRep_Taken();FinishMutation();}
void ANRObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRObjective,bTaken);}
void ANRObjective::Interact(ANRCharacter* P){auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GM||!GS||!GM->IsCombat()||!P||!P->IsAlive()||P->Team!=GS->AttackTeam)return;
 if(bExtraction){if(P->bCarryingDisk)GM->ExtractDisk(P);}else if(!bTaken){WakeForMutation();bTaken=true;P->bCarryingDisk=true;P->Say(6);OnRep_Taken();FinishMutation();}}
void ANRObjective::ResetObjective(){if(!HasAuthority())return;WakeForMutation();SetActorLocation(HomePosition);bTaken=false;OnRep_Taken();FinishMutation();}
void ANRObjective::OnRep_Taken(){Mesh->SetVisibility(!bTaken);Mesh->SetCollisionEnabled(bTaken?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);}
