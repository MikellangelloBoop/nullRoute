#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NRFactions.generated.h"

UENUM(BlueprintType)
enum class ENRFaction : uint8 { Chronos, Police, Rebel, Ascended, RustHounds };

USTRUCT(BlueprintType)
struct NRGAMEPLAY_API FNRMapFactions
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,Config) FName Map=TEXT("NR_Arcology");
 UPROPERTY(EditAnywhere,Config) ENRFaction Attackers=ENRFaction::Rebel;
 UPROPERTY(EditAnywhere,Config) ENRFaction Defenders=ENRFaction::Chronos;
};

// The server selects a pair for the loaded map; clients receive it in GameState.
UCLASS(Config=Game,DefaultConfig,meta=(DisplayName="Null Route / Map factions"))
class NRGAMEPLAY_API UNRFactionSettings : public UDeveloperSettings
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Config,Category="Factions") TArray<FNRMapFactions> Maps;
 FNRMapFactions Resolve(const FString& MapPackage)const;
};
namespace NRFactions
{
 constexpr uint8 Count=5;
 NRGAMEPLAY_API FNRMapFactions WithOptions(FNRMapFactions Base,const FString& Options);
 NRGAMEPLAY_API const TCHAR* Key(ENRFaction Faction);
 NRGAMEPLAY_API const TCHAR* Name(ENRFaction Faction);
}
