#include "NRInterestSubsystem.h"
#include "NRSpatialFilter.h"
#include "Engine/NetDriver.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
bool UNRInterestSubsystem::ShouldCreateSubsystem(UObject* O)const{auto* W=Cast<UWorld>(O);return W&&W->IsGameWorld();}
void UNRInterestSubsystem::RegisterActor(AActor* A){Actors.AddUnique(A);}
void UNRInterestSubsystem::Tick(float Dt){if(GetWorld()->GetNetMode()==NM_Client||Actors.IsEmpty())return;Accum+=Dt;if(Accum<.1f)return;Accum=0;
 auto* Driver=GetWorld()->GetNetDriver();if(!Driver||Driver->ClientConnections.IsEmpty())return;
 auto* System=UE::Net::FReplicationSystemUtil::GetReplicationSystem(GetWorld());if(!System)return;
 auto* Filter=Cast<UNRSpatialFilter>(System->GetFilter(TEXT("NRSpatial")));auto* Bridge=Cast<UObjectReplicationBridge>(System->GetReplicationBridge());if(!Filter||!Bridge)return;
 const int32 Checks=FMath::Min(32,Actors.Num());for(int32 I=0;I<Checks;++I){Cursor%=Actors.Num();auto* Actor=Actors[Cursor++].Get();if(!Actor)continue;auto Handle=Bridge->GetReplicatedRefHandle(Actor);if(!Handle.IsValid())continue;
 for(const auto& Connection:Driver->ClientConnections){auto* PC=Connection?Connection->PlayerController.Get():nullptr;if(!PC)continue;FVector Eye;FRotator Rot;PC->GetPlayerViewPoint(Eye,Rot);const FVector Target=Actor->GetActorLocation();bool Hidden=false;
 // Always preserve collision/interaction state within 30m, including wallbangs.
 if(FVector::DistSquared(Eye,Target)>3000.f*3000.f){FCollisionQueryParams Q;Q.AddIgnoredActor(PC->GetPawn());FHitResult H;Hidden=GetWorld()->LineTraceSingleByChannel(H,Eye,Target,ECC_Visibility,Q)&&H.GetActor()!=Actor;}
 Filter->SetHidden(Handle,Connection->GetConnectionId(),Hidden);}}
 if(Cursor>=Actors.Num()){Actors.RemoveAll([](const auto& W){return !W.IsValid();});Cursor=0;}
}
