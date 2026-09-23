#include "NRGraphVisuals.h"
#include "NRGraphSubsystem.h"
#include "NRNode.h"
#include "NRCharacter.h"
#include "NRStructure.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
bool UNRGraphVisuals::ShouldCreateSubsystem(UObject* Outer)const{auto* W=Cast<UWorld>(Outer);return W&&W->IsGameWorld()&&!IsRunningDedicatedServer();}
void UNRGraphVisuals::Tick(float Dt){if(GetWorld()->GetNetMode()==NM_DedicatedServer)return;Accum+=Dt;if(Accum<.15f)return;Accum=0;
 auto* PC=GetWorld()->GetFirstPlayerController();auto* C=PC?Cast<ANRCharacter>(PC->GetPawn()):nullptr;auto* Graph=GetWorld()->GetSubsystem<UNRGraphSubsystem>();if(!C||!Graph)return;
 if(!Owner){Owner=GetWorld()->SpawnActor<AActor>();Lines=NewObject<UInstancedStaticMeshComponent>(Owner);Lines->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lines->SetCanEverAffectNavigation(false);Lines->RegisterComponent();Lines->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_SDFLine.M_SDFLine")));Visor=NewObject<UPostProcessComponent>(Owner);Visor->bUnbound=true;Visor->RegisterComponent();if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Visor.M_Visor")))Visor->AddOrUpdateBlendable(M,1.f);}
 uint32 Hash=0;for(const auto& W:Graph->GetNodes())if(auto* N=W.Get()){
 Hash=HashCombine(Hash,HashCombine(GetTypeHash(N->NodeID),N->Revision));
 const bool Reveal=N->Team!=C->Team&&C->ScanResult.NodeId==N->NodeID&&C->ScanResult.ExpiresAt>GetWorld()->GetTimeSeconds();N->Mesh->SetRenderCustomDepth(Reveal);N->Mesh->SetCustomDepthStencilValue(2);}
 for(TActorIterator<ANRStructure> It(GetWorld());It;++It){const bool Reveal=C->OperatorRole==ENRRole::Assault&&!It->bCollapsed&&FVector::DistSquared(C->GetActorLocation(),It->GetActorLocation())<1000.f*1000.f;It->GetGeometryCollectionComponent()->SetRenderCustomDepth(Reveal);It->GetGeometryCollectionComponent()->SetCustomDepthStencilValue(5);}
 if(Hash==LastHash)return;LastHash=Hash;Lines->ClearInstances();
 for(const auto& W:Graph->GetNodes())if(auto* N=W.Get())if(N->Team==C->Team&&N->NodeState!=ENRNodeState::Destroyed)for(const auto& E:N->OutputPins)if(auto* To=Graph->Find(E.Target)){
 const FVector A=N->GetActorLocation()+FVector(0,0,75),B=To->GetActorLocation()+FVector(0,0,75);FVector Prev=A;
 for(int32 I=1;I<=8;++I){float T=float(I)/8;FVector Next=FMath::Lerp(A,B,T)+FVector(0,0,60*FMath::Sin(T*PI));FVector D=Next-Prev;Lines->AddInstance(FTransform(D.Rotation(),(Next+Prev)*.5,FVector(D.Size()/100.f,.06,1)),true);Prev=Next;}}
}
