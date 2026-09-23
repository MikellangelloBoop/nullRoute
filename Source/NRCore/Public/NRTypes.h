#pragma once
#include "CoreMinimal.h"
#include "NRTypes.generated.h"
UENUM() enum class ENRRole:uint8 { Engineer, Scout, Assault, Medic };
UENUM() enum class ENRPhase:uint8 { Waiting, Preparation, Combat, Resolution, MatchOver };
UENUM() enum class ENRNodeState:uint8 { Printing, Dormant, Active, Compromised, Destroyed };
UENUM() enum class ENRNodeOp:uint8 { Sensor, And, Or, Invert, Delay, Turret, Relay };
UENUM() enum class ENRPrintPhase:uint8 { None, Polymer, FilamentChange, Reinforcement };
USTRUCT() struct NRCORE_API FNRNodeLink {
 GENERATED_BODY()
 UPROPERTY() FGuid Target;
 UPROPERTY() uint8 SourcePort=0;
 UPROPERTY() uint8 TargetPort=0;
};
USTRUCT() struct NRCORE_API FNRScanResult {
 GENERATED_BODY()
 UPROPERTY() FGuid NodeId;
 UPROPERTY() uint8 Team=255;
 UPROPERTY() ENRNodeOp Operation=ENRNodeOp::Relay;
 UPROPERTY() float Delay=0;
 UPROPERTY() int32 LinkCount=0;
 UPROPERTY() float ExpiresAt=0;
};
