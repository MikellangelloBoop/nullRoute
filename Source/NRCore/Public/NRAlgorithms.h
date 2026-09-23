#pragma once
#include "CoreMinimal.h"
namespace NR {
// Directed graph with bounded iterative traversal: no recursion on network data.
inline bool Reachable(const TMap<FGuid,TArray<FGuid>>& Graph,FGuid From,FGuid To) {
 TSet<FGuid> Seen;TArray<FGuid> Queue{From};
 for(int32 I=0;I<Queue.Num()&&I<4096;++I){const FGuid N=Queue[I];if(N==To)return true;
 if(Seen.Contains(N))continue;Seen.Add(N);
 if(const auto* Edges=Graph.Find(N))for(const FGuid& E:*Edges)if(!Seen.Contains(E))Queue.Add(E);}
 return false;
}
inline TArray<FGuid> Closure(const TMap<FGuid,TArray<FGuid>>& Graph,FGuid Root) {
 TSet<FGuid> Seen{Root};TArray<FGuid> Queue{Root};
 for(int32 I=0;I<Queue.Num()&&I<4096;++I)if(const auto* E=Graph.Find(Queue[I]))
 for(const FGuid& Id:*E)if(!Seen.Contains(Id)){Seen.Add(Id);Queue.Add(Id);}
 return Queue;
}
inline bool UpdateDebris(float SpeedSquared,float Delta,float& QuietSeconds) {
 QuietSeconds=SpeedSquared<25.f ? QuietSeconds+Delta:0.f;return QuietSeconds>=3.f;
}
inline bool IsFiniteVector(const FVector& P){return FMath::IsFinite(P.X)&&FMath::IsFinite(P.Y)&&FMath::IsFinite(P.Z);}
inline bool MetalRicochet(const FVector& Velocity,const FVector& Normal,int32 PreviousBounces,FVector& Reflected) {
 if(!IsFiniteVector(Velocity)||!IsFiniteVector(Normal)||PreviousBounces!=0||Velocity.SizeSquared()<18000.f*18000.f)return false;
 const FVector N=Normal.GetSafeNormal();if(N.IsNearlyZero())return false;
 const float Incidence=-FVector::DotProduct(Velocity.GetSafeNormal(),N);
 if(Incidence<=0||Incidence>=.28f)return false;
 Reflected=(Velocity-2*FVector::DotProduct(Velocity,N)*N)*.52f;return true;
}
}
