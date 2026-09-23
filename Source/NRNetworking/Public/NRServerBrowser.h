#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "HAL/PlatformProcess.h"
#include "NRServerBrowser.generated.h"
class FSocket;
class FInternetAddr;
class APlayerController;

struct NRNETWORKING_API FNRServerEntry
{
 FString Address,Name,Map=TEXT("NR_Arcology"),Phase=TEXT("Нет ответа");
 int32 Players=0,Capacity=10,Ping=-1;
 bool bOnline=false,bCompatible=false,bFavorite=false,bOfficial=false;
 double LastSeen=0;
};

// Persists across client travel. UDP queries are bounded, non-blocking and never run in actor ticks.
UCLASS() class NRNETWORKING_API UNRServerBrowser : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 static bool ParseAddress(const FString& Input,FString& Host,int32& Port,FString& Normalized);
 void Refresh();
 bool AddFavorite(const FString& Address);
 void RemoveFavorite(const FString& Address);
 bool Join(APlayerController* Player,const FString& Address);
 bool StartLocalServer();
 void StopLocalServer();
 bool HasLocalServer();
 void Advertise(const FString& Name,const FString& Map,int32 Port,int32 Players,int32 Capacity,const FString& Phase);
 void StopAdvertising();
 const TArray<FNRServerEntry>& GetServers()const{return Servers;}
 FString Status=TEXT("Обновите список или введите IP:порт.");
 uint32 Revision=0;
 bool IsSearching()const{return SearchUntil>FPlatformTime::Seconds();}
 static constexpr int32 Protocol=3;
private:
 friend class UNRSmokeSubsystem;
 struct FPending {double Started=0;FString Address;int32 Port=0;};
 bool Tick(float Dt);
 bool EnsureQuerySocket();
 void Probe(const FString& Address);
 void SendProbe(const TSharedRef<FInternetAddr>& Destination,const FString& Address,int32 Port,uint32 Generation);
 void ReadReplies();
 void ReadRequests();
 void SaveFavorites();
 void NetworkFailure(UWorld* World,class UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Error);
 void TravelFailure(UWorld* World,ETravelFailure::Type Type,const FString& Error);
 FNRServerEntry& FindOrAdd(const FString& Address);
 FSocket* QuerySocket=nullptr;
 FSocket* AdvertSocket=nullptr;
 TArray<FNRServerEntry> Servers;
 TMap<FString,FPending> Pending;
 TMap<FString,double> LastRequest;
 TArray<FString> Favorites;
 FTSTicker::FDelegateHandle TickHandle;
 FProcHandle LocalServerProcess;
 FDelegateHandle NetworkHandle,TravelHandle;
 FString ServerName,ServerMap,ServerPhase;
 int32 ServerPort=0,ServerPlayers=0,ServerCapacity=10,ReplyCount=0;
 double SearchUntil=0,LastRefresh=-100,ReplyWindow=0,ConnectionStarted=0;
 bool bConnecting=false;
 bool bTestNoPersistence=false;
 uint32 ScanGeneration=0;
};
