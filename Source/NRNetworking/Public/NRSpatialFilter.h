#pragma once
#include "CoreMinimal.h"
#include "Iris/ReplicationSystem/Filtering/NetObjectGridFilter.h"
#include "NRSpatialFilter.generated.h"
// The gameplay visibility service publishes conservative occlusion decisions.
UCLASS(Transient)
class NRNETWORKING_API UNRSpatialFilter:public UNetObjectGridWorldLocFilter {
 GENERATED_BODY()
public:
 void SetHidden(UE::Net::FNetRefHandle Handle,uint32 ConnectionId,bool bHidden);
 virtual void Filter(FNetObjectFilteringParams& Params) override;
 virtual void RemoveConnection(uint32 ConnectionId) override;
 virtual void RemoveObject(UE::Net::FInternalNetRefIndex Index,const FNetObjectFilteringInfo& Info) override;
private:
 TMap<uint32,TSet<uint32>> Hidden;
};
