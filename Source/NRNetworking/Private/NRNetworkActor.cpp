#include "NRNetworkActor.h"
#include "NRInterestSubsystem.h"
#include "Engine/World.h"
ANRNetworkActor::ANRNetworkActor(){bReplicates=true;bReplicateUsingRegisteredSubObjectList=true;
 PrimaryActorTick.bCanEverTick=false;NetDormancy=DORM_Initial;SetReplicateMovement(false);NetCullDistanceSquared=250000000.f;}
void ANRNetworkActor::BeginPlay(){Super::BeginPlay();if(auto* Interest=GetWorld()->GetSubsystem<UNRInterestSubsystem>())Interest->RegisterActor(this);if(HasAuthority()&&!IsNetStartupActor()){SetNetDormancy(DORM_Awake);ForceNetUpdate();}}
void ANRNetworkActor::WakeForMutation(){if(HasAuthority())SetNetDormancy(DORM_Awake);}
void ANRNetworkActor::FinishMutation(){if(HasAuthority()){ForceNetUpdate();SetNetDormancy(DORM_DormantAll);}}
