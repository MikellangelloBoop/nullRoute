#include "NRGraphSubsystem.h"
#include "NRNode.h"
#include "NRCharacter.h"
#include "NRAlgorithms.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
bool UNRGraphSubsystem::ShouldCreateSubsystem(UObject* Outer)const{const UWorld* W=Cast<UWorld>(Outer);return W&&W->IsGameWorld();}
void UNRGraphSubsystem::RegisterNode(ANodeBase* N){Nodes.AddUnique(N);++Revision;}
void UNRGraphSubsystem::RemoveNode(ANodeBase* N){Nodes.Remove(N);++Revision;}
ANodeBase* UNRGraphSubsystem::Find(FGuid Id)const{for(const auto& W:Nodes)if(ANodeBase* N=W.Get())if(N->NodeID==Id)return N;return nullptr;}
bool UNRGraphSubsystem::Connect(ANodeBase* A,ANodeBase* B,uint8 Team){
 if(!A||!B||!A->HasAuthority()||A==B||A->Team!=Team||B->Team!=Team||A->OutputPins.Num()>=8||Nodes.Num()>256)return false;
 if(A->NodeState==ENRNodeState::Destroyed||B->NodeState==ENRNodeState::Destroyed||FVector::DistSquared(A->GetActorLocation(),B->GetActorLocation())>2500.f*2500.f)return false;
 TMap<FGuid,TArray<FGuid>> Graph;for(const auto& W:Nodes)if(auto* N=W.Get())for(const auto& E:N->OutputPins)Graph.FindOrAdd(N->NodeID).Add(E.Target);
 for(const auto& E:A->OutputPins)if(E.Target==B->NodeID)return false;
 if(NR::Reachable(Graph,B->NodeID,A->NodeID))return false;
 A->WakeForMutation();FNRNodeLink L;L.Target=B->NodeID;A->OutputPins.Add(L);++A->Revision;A->FinishMutation();++Revision;return true;
}
void UNRGraphSubsystem::Cascade(ANodeBase* Root){
 if(!Root||!Root->HasAuthority()||!Root->IsBlackNode())return;
 TMap<FGuid,TArray<FGuid>> Graph;
 // Trojan propagates over the connected infrastructure domain in both directions.
 for(const auto& W:Nodes)if(auto* N=W.Get())for(const auto& E:N->OutputPins){Graph.FindOrAdd(N->NodeID).Add(E.Target);Graph.FindOrAdd(E.Target).Add(N->NodeID);}
 const auto Victims=NR::Closure(Graph,Root->NodeID);for(const auto& Id:Victims)if(auto* N=Find(Id))N->NetExecute();++Revision;
}
void UNRGraphSubsystem::Tick(float Dt){
 if(GetWorld()->GetNetMode()==NM_Client)return;Accum+=Dt;if(Accum<.1f)return;Accum=0;
 TMap<FGuid,TArray<bool>> Inputs;TMap<FGuid,bool> Previous;
 TArray<ANRCharacter*> Players;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->IsAlive())Players.Add(*It);
 for(const auto& W:Nodes)if(auto* N=W.Get()){Previous.Add(N->NodeID,N->bOutput);for(const auto& E:N->OutputPins)Inputs.FindOrAdd(E.Target).Add(N->bOutput);}
 for(const auto& W:Nodes){auto* N=W.Get();if(!N||N->NodeState!=ENRNodeState::Active)continue;
 const auto* Values=Inputs.Find(N->NodeID);bool Any=false,All=Values&&Values->Num()>0;
 if(Values)for(bool V:*Values){Any|=V;All&=V;}bool Result=Any;
 switch(N->Operation){
 case ENRNodeOp::Sensor:Result=false;for(auto* C:Players)if(C->Team!=N->Team&&C->SpoofUntil<GetWorld()->GetTimeSeconds()&&FVector::DistSquared(C->GetActorLocation(),N->GetActorLocation())<900.f*900.f){Result=true;break;}break;
 case ENRNodeOp::And:Result=All;break;case ENRNodeOp::Or:Result=Any;break;case ENRNodeOp::Invert:Result=!Any;break;
 case ENRNodeOp::Delay:if(!Any){DelayStarted.Remove(N->NodeID);Result=false;}else{double& Since=DelayStarted.FindOrAdd(N->NodeID,GetWorld()->GetTimeSeconds());Result=GetWorld()->GetTimeSeconds()-Since>=N->TriggerDelay;}break;
 case ENRNodeOp::Turret:if(Any){for(auto* C:Players)if(C->Team!=N->Team&&FVector::DistSquared(C->GetActorLocation(),N->GetActorLocation())<1400.f*1400.f){FHitResult Hit;FCollisionQueryParams P;P.AddIgnoredActor(N);if(GetWorld()->LineTraceSingleByChannel(Hit,N->GetActorLocation()+FVector(0,0,90),C->GetActorLocation(),ECC_Visibility,P)&&Hit.GetActor()==C){UGameplayStatics::ApplyDamage(C,3.f,nullptr,N,nullptr);break;}}}break;
 default:break;}N->SetOutput(Result);
 }
}
