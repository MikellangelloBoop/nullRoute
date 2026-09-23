#include "NRSpatialFilter.h"
#include "Iris/ReplicationSystem/NetRefHandle.h"
void UNRSpatialFilter::SetHidden(UE::Net::FNetRefHandle Handle,uint32 C,bool bHide){
 const uint32 I=GetObjectIndex(Handle);if(!I)return;
 if(bHide)Hidden.FindOrAdd(C).Add(I);else if(auto* S=Hidden.Find(C))S->Remove(I);
}
void UNRSpatialFilter::Filter(FNetObjectFilteringParams& P){Super::Filter(P);
 if(const auto* S=Hidden.Find(P.ConnectionId))for(uint32 I:*S)
 if(I<P.OutAllowedObjects.GetNumBits()&&GetFilteredObjects().IsBitSet(I))P.OutAllowedObjects.ClearBit(I);
}
void UNRSpatialFilter::RemoveConnection(uint32 C){Hidden.Remove(C);Super::RemoveConnection(C);}
void UNRSpatialFilter::RemoveObject(UE::Net::FInternalNetRefIndex I,const FNetObjectFilteringInfo& Info){
 for(auto& P:Hidden)P.Value.Remove(I);Super::RemoveObject(I,Info);
}
