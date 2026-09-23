#include "NRSmokeSubsystem.h"
#include "NRServerBrowser.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRHUD.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void UNRSmokeSubsystem::RunServerSystemTest(ANRCharacter* C,APlayerController* PC,float T)
{
 auto* B=GetWorld()->GetGameInstance()->GetSubsystem<UNRServerBrowser>();if(!B||!C||!PC)return;
 auto Check=[this](bool OK,const TCHAR* Label){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_SERVERTEST %s : %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(Stage==0&&T>2)
 {
  Stage=1;PreparationEpoch=T;B->bTestNoPersistence=true;B->Favorites.Reset();B->Servers.Reset();
  FString Host,Normal;int32 Port=0;
  Check(B->ParseAddress(TEXT("  Example.COM:7779  "),Host,Port,Normal)&&Normal==TEXT("example.com:7779")&&Port==7779,TEXT("hostname normalized without travel options"));
  Check(B->ParseAddress(TEXT("127.0.0.1"),Host,Port,Normal)&&Port==7777,TEXT("IPv4 default game port"));
  bool Reject=true;for(const TCHAR* Bad:{TEXT(""),TEXT("http://evil"),TEXT("127.0.0.1?listen"),TEXT("127.0.0.1:7777?Training=1"),TEXT("bad host"),TEXT("-bad.test"),TEXT("a..b"),TEXT("1.2.3.4:0"),TEXT("1.2.3.4:65535"),TEXT("999.1.1.1"),TEXT("0.0.0.0"),TEXT("255.255.255.255"),TEXT("224.0.0.1"),TEXT("::1")})Reject&=!B->ParseAddress(Bad,Host,Port,Normal);
  Check(Reject,TEXT("invalid ports injection and non-unicast endpoints rejected"));
  Check(B->AddFavorite(TEXT("127.0.0.1:17880"))&&B->AddFavorite(TEXT("127.0.0.1:17880"))&&B->Favorites.Num()==1,TEXT("favorites deduplicated"));
  for(int I=1;I<32;++I)B->AddFavorite(FString::Printf(TEXT("192.168.5.%d:7777"),I));
  Check(!B->AddFavorite(TEXT("192.168.6.1"))&&B->Favorites.Num()==32&&B->AddFavorite(TEXT("127.0.0.1:17880")),TEXT("favorite storage bounded with idempotent re-add"));
  B->RemoveFavorite(TEXT("192.168.5.1:7777"));Check(B->Favorites.Num()==31,TEXT("remove favorite"));
  B->Favorites.Reset();B->Servers.Reset();B->AddFavorite(TEXT("127.0.0.1:17880"));
  B->Advertise(TEXT("Тест Элары"),TEXT("NR_Arcology"),17880,2,3,TEXT("Подготовка"));B->LastRefresh=-100;B->Refresh();
  Check(B->AdvertSocket!=nullptr&&B->QuerySocket!=nullptr,TEXT("separate query and advertisement UDP sockets opened"));
 }
 if(Stage==1&&T>PreparationEpoch+1)
 {
  Stage=2;const auto* Row=B->Servers.FindByPredicate([](const auto& E){return E.Address==TEXT("127.0.0.1:17880");});
  Check(Row&&Row->bOnline&&Row->bCompatible&&Row->Players==2&&Row->Capacity==3&&Row->Ping>=0&&Row->Name==TEXT("Тест Элары")&&Row->Phase==TEXT("Подготовка"),TEXT("real UDP roundtrip preserves nonce protocol counts and UTF8"));
  auto& Full=B->FindOrAdd(TEXT("127.0.0.1:17882"));Full.bOnline=true;Full.bCompatible=true;Full.Players=Full.Capacity=2;
  Check(!B->Join(PC,Full.Address)&&!B->bConnecting,TEXT("full server blocked before travel"));
  Full.Players=0;Full.bCompatible=false;Check(!B->Join(PC,Full.Address),TEXT("protocol mismatch blocked"));
  Check(!B->Join(PC,TEXT("127.0.0.1?listen")),TEXT("direct connection rejects Unreal URL options"));
  B->StopAdvertising();Check(B->AdvertSocket==nullptr,TEXT("advertisement socket closes cleanly"));
  UE_LOG(LogTemp,Display,TEXT("NR_SERVERTEST_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
 }
 if(T>30)FPlatformMisc::RequestExitWithStatus(false,1);
}

void UNRSmokeSubsystem::RunServerJoinTest(ANRCharacter* C,APlayerController* PC,float T)
{
 auto* B=GetWorld()->GetGameInstance()->GetSubsystem<UNRServerBrowser>();if(!B||!PC)return;
 const bool Full=FParse::Param(FCommandLine::Get(),TEXT("NRExpectFull")),Late=FParse::Param(FCommandLine::Get(),TEXT("NRExpectLate"));
 if(GetWorld()->GetNetMode()==NM_Standalone)
 {
  if(Stage==0&&T>2){Stage=1;FString Address;FParse::Value(FCommandLine::Get(),TEXT("NRJoinAddress="),Address);if(!B->Join(PC,Address))FPlatformMisc::RequestExitWithStatus(false,1);}
  if(Full&&B->Status.Contains(TEXT("SERVER_FULL"))){UE_LOG(LogTemp,Display,TEXT("NR_SERVERJOIN FULL_REJECTED PASS"));FPlatformMisc::RequestExitWithStatus(false,0);}
  if(T>50){UE_LOG(LogTemp,Error,TEXT("NR_SERVERJOIN TIMEOUT %s"),*B->Status);FPlatformMisc::RequestExitWithStatus(false,1);}return;
 }
 if(Stage==0&&C&&T>3)
 {
  auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS)return;
  if(Full){UE_LOG(LogTemp,Error,TEXT("NR_SERVERJOIN FULL_SERVER_ACCEPTED FAIL"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
  const bool OK=C->Team<2&&GS->MaximumPlayers==(FParse::Param(FCommandLine::Get(),TEXT("NRRemoteTest"))?10:3)&&GS->MinimumPlayers==2&&(Late?(C->bDead&&GS->Phase==ENRPhase::Combat):!C->bDead);
  Stage=1;UE_LOG(LogTemp,Display,TEXT("NR_SERVERJOIN %s %s team %d phase %d"),Late?TEXT("LATE_QUEUED"):TEXT("CONNECTED"),OK?TEXT("PASS"):TEXT("FAIL"),C->Team,int(GS->Phase));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);
 }
 if(Stage==1&&FParse::Param(FCommandLine::Get(),TEXT("NRRemoteTest")))
 {
  auto* GS=GetWorld()->GetGameState<ANRGameState>();if(GS&&GS->Phase==ENRPhase::Combat&&GS->ConnectedPlayers>=2){Stage=2;UE_LOG(LogTemp,Display,TEXT("NR_PUBLIC_COMBAT PASS team %d players %d"),C->Team,GS->ConnectedPlayers);}
 }
}

void UNRSmokeSubsystem::RunServerVisual(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;
 if(Stage==0&&T>3){Stage=1;if(auto* HUD=Cast<ANRHUD>(PC->GetHUD()))HUD->ToggleServers();}
 if(Stage==1&&T>8){Stage=2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/ServerBrowser.png"),true,false);}
 if(Stage==2&&T>10)FPlatformMisc::RequestExitWithStatus(false,0);
}
