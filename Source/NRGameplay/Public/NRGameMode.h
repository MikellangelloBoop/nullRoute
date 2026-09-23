#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "NRTypes.h"
#include "NRFactions.h"
#include "NRGameMode.generated.h"
class ANRCharacter;
UCLASS()class NRGAMEPLAY_API ANRGameState:public AGameStateBase {
 GENERATED_BODY()
public:
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 UPROPERTY(Replicated)ENRPhase Phase=ENRPhase::Waiting;
 UPROPERTY(Replicated)float PhaseEnd=0;
 UPROPERTY(Replicated)int32 Round=0;
 UPROPERTY(Replicated)int32 ConnectedPlayers=0;
 UPROPERTY(Replicated)int32 MinimumPlayers=2;
 UPROPERTY(Replicated)int32 MaximumPlayers=10;
 UPROPERTY(Replicated)int32 EchelonScore=0;
 UPROPERTY(Replicated)int32 SyndicateScore=0;
 UPROPERTY(Replicated)int32 Team0Power=100;
 UPROPERTY(Replicated)int32 Team1Power=100;
 UPROPERTY(Replicated)uint8 AttackTeam=1;
 UPROPERTY(Replicated) ENRFaction AttackerFaction=ENRFaction::Rebel;
 UPROPERTY(Replicated) ENRFaction DefenderFaction=ENRFaction::Chronos;
 ENRFaction FactionForTeam(uint8 Team)const{return Team==AttackTeam?AttackerFaction:DefenderFaction;}
 UPROPERTY(Replicated)float ElaraControl=0.5f;
 UPROPERTY(Replicated)FString Announcement=TEXT("CONNECTING TO ECHELON");
 // Shared geometry rule: client prediction and server validation use the same boundary.
 static constexpr float PreparationBoundaryX=-1580.f;
 bool CanPrepareAt(uint8 Team,const FVector& Position,float Clearance=60.f)const;
 bool SpendPower(uint8 Team,int32 Amount);
};
UCLASS(Config=Game)class NRGAMEPLAY_API ANRGameMode:public AGameModeBase {
 GENERATED_BODY()
public:
 friend class UNRSmokeSubsystem;
 ANRGameMode();virtual void StartPlay()override;virtual void PostLogin(APlayerController* PC)override;
 virtual void InitGame(const FString& Map,const FString& Options,FString& Error)override;
 virtual void RestartPlayer(AController* Controller)override;
 virtual void PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& ErrorMessage)override;
 virtual void Logout(AController* Exiting)override;
 void PlayerKilled(ANRCharacter* Victim,AController* Killer);void ExtractDisk(ANRCharacter* Player);
 bool IsCombat()const;
 UPROPERTY(Config) float PreparationSeconds=45.f;
private:
 void BeginRound();void BeginCombat();void ResolveRound(uint8 Winner,const FString& Why);
 void UpdateMatch();void SpawnBUP();
 void UpdateServerListing();void RestartMatch();
 bool HasEnoughPlayers()const;
 int32 MinimumPlayers=2,MaximumPlayers=10;
 FString ServerName=TEXT("Null Route");
 UPROPERTY() TObjectPtr<class ANRPreparationZone> PreparationZone;
 FTimerHandle PhaseTimer,MatchTimer;
 uint8 TrainingTeam=1;int32 Seed=137;int32 AssignedPlayers=0;bool bTraining=true;
};
