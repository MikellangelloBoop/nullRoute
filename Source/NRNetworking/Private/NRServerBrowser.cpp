#include "NRServerBrowser.h"
#include "Common/UdpSocketBuilder.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
 void CloseSocket(FSocket*& Socket){if(Socket){Socket->Close();ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);Socket=nullptr;}}
 FString Clean(const FString& Text,int32 Limit){FString Out;for(TCHAR C:Text.Left(Limit))if(!FChar::IsControl(C))Out.AppendChar(C);return Out;}
}
bool UNRServerBrowser::ParseAddress(const FString& Input,FString& Host,int32& Port,FString& Normalized)
{
 FString Value=Input.TrimStartAndEnd().ToLower();Port=7777;
 if(Value.IsEmpty()||Value.Len()>128)return false;
 int32 Colon=INDEX_NONE;
 if(Value.FindChar(':',Colon))
 {
  const FString Digits=Value.Mid(Colon+1);if(Digits.IsEmpty()||Digits.Len()>5)return false;
  for(TCHAR C:Digits)if(C<'0'||C>'9')return false;Port=FCString::Atoi(*Digits);Value=Value.Left(Colon);
 }
 if(Port<1||Port>65534||Value.IsEmpty()||Value.StartsWith(TEXT("."))||Value.EndsWith(TEXT(".")))return false;
 TArray<FString> Labels;Value.ParseIntoArray(Labels,TEXT("."),false);
 for(const FString& Label:Labels){if(Label.IsEmpty()||Label.Len()>63||Label.StartsWith(TEXT("-"))||Label.EndsWith(TEXT("-")))return false;for(TCHAR C:Label)if(!((C>='a'&&C<='z')||(C>='0'&&C<='9')||C=='-'))return false;}
 bool Numeric=true;for(TCHAR C:Value)Numeric&=(C>='0'&&C<='9')||C=='.';
 if(Numeric){FIPv4Address IP;if(!FIPv4Address::Parse(Value,IP)||IP==FIPv4Address::Any||IP==FIPv4Address::LanBroadcast||IP.A>=224)return false;}
 Host=Value;Normalized=FString::Printf(TEXT("%s:%d"),*Host,Port);return true;
}
void UNRServerBrowser::Initialize(FSubsystemCollectionBase& Collection)
{
 Super::Initialize(Collection);
 if(GConfig)
 {
  TArray<FString> Saved;GConfig->GetArray(TEXT("NullRoute.Servers"),TEXT("Favorites"),Saved,GGameUserSettingsIni);
  TArray<FString> Official;GConfig->GetArray(TEXT("NullRoute.Servers"),TEXT("OfficialEndpoints"),Official,GGameIni);
  for(const FString& Address:Saved){FString Host,Normalized;int32 Port;if(Favorites.Num()<32&&ParseAddress(Address,Host,Port,Normalized)){Favorites.AddUnique(Normalized);FindOrAdd(Normalized).bFavorite=true;}}
  for(const FString& Address:Official){FString Host,Normalized;int32 Port;if(Servers.Num()<48&&ParseAddress(Address,Host,Port,Normalized)){auto& Row=FindOrAdd(Normalized);Row.bOfficial=true;Row.Name=TEXT("Null Route / выделенный сервер");}}
 }
 TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UNRServerBrowser::Tick),.1f);
 if(GEngine){NetworkHandle=GEngine->OnNetworkFailure().AddUObject(this,&UNRServerBrowser::NetworkFailure);TravelHandle=GEngine->OnTravelFailure().AddUObject(this,&UNRServerBrowser::TravelFailure);}
}
void UNRServerBrowser::Deinitialize()
{
 FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);++ScanGeneration;CloseSocket(QuerySocket);StopAdvertising();
 if(GEngine){GEngine->OnNetworkFailure().Remove(NetworkHandle);GEngine->OnTravelFailure().Remove(TravelHandle);}
 // Only the child created by this game instance is stopped; externally managed servers are untouched.
 StopLocalServer();Super::Deinitialize();
}
FNRServerEntry& UNRServerBrowser::FindOrAdd(const FString& Address)
{
 if(auto* Existing=Servers.FindByPredicate([&](const FNRServerEntry& E){return E.Address==Address;}))return *Existing;
 FNRServerEntry New;New.Address=Address;New.Name=Address;Servers.Add(New);return Servers.Last();
}
bool UNRServerBrowser::EnsureQuerySocket()
{
 if(QuerySocket)return true;QuerySocket=FUdpSocketBuilder(TEXT("NRServerBrowser")).AsNonBlocking().WithBroadcast().BoundToPort(0).WithReceiveBufferSize(32768);
 if(!QuerySocket){Status=TEXT("Не удалось открыть UDP-поиск серверов.");++Revision;}return QuerySocket!=nullptr;
}
void UNRServerBrowser::Refresh()
{
 const double Now=FPlatformTime::Seconds();if(Now-LastRefresh<1||!EnsureQuerySocket())return;
 LastRefresh=Now;SearchUntil=Now+4;++ScanGeneration;Pending.Reset();
 Servers.RemoveAll([](const FNRServerEntry& E){return !E.bFavorite&&!E.bOfficial;});
 for(auto& Row:Servers){Row.bOnline=false;Row.bCompatible=false;Row.Ping=-1;Row.Phase=TEXT("Поиск…");}
 Status=TEXT("Поиск в LAN и проверка сохранённых серверов…");++Revision;
 auto* Sockets=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
 auto Broadcast=Sockets->CreateInternetAddr();Broadcast->SetBroadcastAddress();Broadcast->SetPort(7778);SendProbe(Broadcast,TEXT(""),7777,ScanGeneration);
 Probe(TEXT("127.0.0.1:7777"));TArray<FString> Addresses;for(const auto& Row:Servers)Addresses.Add(Row.Address);for(const auto& Address:Addresses)Probe(Address);
}
void UNRServerBrowser::Probe(const FString& Address)
{
 FString Host,Normalized;int32 Port;if(!ParseAddress(Address,Host,Port,Normalized)||!QuerySocket)return;
 auto* Sockets=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);auto IP=Sockets->CreateInternetAddr();bool Valid=false;IP->SetIp(*Host,Valid);
 if(Valid){IP->SetPort(Port+1);SendProbe(IP,Normalized,Port,ScanGeneration);return;}
 const uint32 Generation=ScanGeneration;TWeakObjectPtr<UNRServerBrowser> Self=this;
 // DNS runs outside the game thread. Replies from a superseded scan are discarded.
 Sockets->GetAddressInfoAsync([Self,Normalized,Port,Generation](FAddressInfoResult Result)
 {
  if(Result.Results.IsEmpty())return;auto IP=Result.Results[0].Address;IP->SetPort(Port+1);
  AsyncTask(ENamedThreads::GameThread,[Self,Normalized,Port,Generation,IP](){if(auto* Browser=Self.Get())Browser->SendProbe(IP,Normalized,Port,Generation);});
 },*Host,nullptr,EAddressInfoFlags::Default,FNetworkProtocolTypes::IPv4,SOCKTYPE_Datagram);
}
void UNRServerBrowser::SendProbe(const TSharedRef<FInternetAddr>& IP,const FString& Address,int32 Port,uint32 Generation)
{
 if(!QuerySocket||Generation!=ScanGeneration||Pending.Num()>=64)return;
 const FString Nonce=FGuid::NewGuid().ToString(EGuidFormats::Digits);Pending.Add(Nonce,{FPlatformTime::Seconds(),Address,Port});
 const FTCHARToUTF8 Data(*(TEXT("NRQ2 ")+Nonce));int32 Sent=0;QuerySocket->SendTo(reinterpret_cast<const uint8*>(Data.Get()),Data.Length(),Sent,*IP);
}
void UNRServerBrowser::ReadReplies()
{
 if(!QuerySocket)return;auto* Sockets=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);uint32 Size=0;
 for(int Budget=0;Budget<32&&QuerySocket->HasPendingData(Size);++Budget)
 {
  uint8 Bytes[1025];int32 Read=0;auto Sender=Sockets->CreateInternetAddr();if(!QuerySocket->RecvFrom(Bytes,1024,Read,*Sender)||Read<=0||Size>1024)continue;Bytes[Read]=0;
  TSharedPtr<FJsonObject> Json;const FString Payload=UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes));if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Payload),Json)||!Json)continue;
  FString Magic,Nonce,Name,Map,Phase;double Version=0,Port=0,Players=0,Capacity=0;
  if(!Json->TryGetStringField(TEXT("magic"),Magic)||Magic!=TEXT("NULLROUTE")||!Json->TryGetStringField(TEXT("nonce"),Nonce)||!Json->TryGetNumberField(TEXT("protocol"),Version)||!Json->TryGetNumberField(TEXT("port"),Port)||!Json->TryGetNumberField(TEXT("players"),Players)||!Json->TryGetNumberField(TEXT("capacity"),Capacity)||!Json->TryGetStringField(TEXT("name"),Name)||!Json->TryGetStringField(TEXT("map"),Map)||!Json->TryGetStringField(TEXT("phase"),Phase))continue;
  const FPending* Request=Pending.Find(Nonce);const double Now=FPlatformTime::Seconds();
  if(!Request||Now-Request->Started>4||Port!=Request->Port||Port<1||Port>65534||Players<0||Players>10||Capacity<2||Capacity>10||Players>Capacity)continue;
  const FString Address=Request->Address.IsEmpty()?FString::Printf(TEXT("%s:%d"),*Sender->ToString(false),int32(Port)):Request->Address;
  if(Servers.Num()>=64&&!Servers.ContainsByPredicate([&](const auto& E){return E.Address==Address;}))continue;
  auto& Row=FindOrAdd(Address);Row.Name=Clean(Name,48);Row.Map=Clean(Map,48);Row.Phase=Clean(Phase,40);Row.Players=int32(Players);Row.Capacity=int32(Capacity);Row.Ping=FMath::Clamp(int32((Now-Request->Started)*1000),0,9999);Row.LastSeen=Now;Row.bOnline=true;Row.bCompatible=Version==Protocol;Row.bFavorite=Favorites.Contains(Address);++Revision;
 }
}
void UNRServerBrowser::Advertise(const FString& Name,const FString& Map,int32 Port,int32 Players,int32 Capacity,const FString& Phase)
{
 if(Port<1||Port>65534)return;if(ServerPort!=Port)StopAdvertising();ServerPort=Port;ServerName=Clean(Name,48);ServerMap=Clean(Map,48);ServerPhase=Clean(Phase,40);ServerPlayers=FMath::Clamp(Players,0,10);ServerCapacity=FMath::Clamp(Capacity,2,10);
 if(!AdvertSocket){AdvertSocket=FUdpSocketBuilder(TEXT("NRServerQuery")).AsNonBlocking().BoundToPort(Port+1).WithReceiveBufferSize(32768);UE_LOG(LogTemp,Display,TEXT("NR_SERVER_QUERY %s port %d"),AdvertSocket?TEXT("READY"):TEXT("FAILED"),Port+1);}
}
void UNRServerBrowser::StopAdvertising(){CloseSocket(AdvertSocket);ServerPort=0;LastRequest.Reset();}
void UNRServerBrowser::ReadRequests()
{
 if(!AdvertSocket)return;uint32 Size=0;const double Now=FPlatformTime::Seconds();
 if(Now-ReplyWindow>=1){ReplyWindow=Now;ReplyCount=0;LastRequest.Reset();}
 auto* Sockets=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
 for(int Budget=0;Budget<32&&AdvertSocket->HasPendingData(Size);++Budget)
 {
  uint8 Bytes[65];int32 Read=0;auto Sender=Sockets->CreateInternetAddr();if(!AdvertSocket->RecvFrom(Bytes,64,Read,*Sender)||Read!=37||Size!=37)continue;Bytes[Read]=0;
  const FString Packet=UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes));if(!Packet.StartsWith(TEXT("NRQ2 ")))continue;const FString Nonce=Packet.Mid(5);
  bool Valid=Nonce.Len()==32;for(TCHAR C:Nonce)Valid&=FChar::IsHexDigit(C);if(!Valid)continue;
  // Bound work and amplification: at most 32 small replies/s globally, 5/s per source IP.
  const FString Source=Sender->ToString(false);if(ReplyCount>=32)continue;if(const double* Last=LastRequest.Find(Source))if(Now-*Last<.2)continue;LastRequest.Add(Source,Now);++ReplyCount;
  auto Json=MakeShared<FJsonObject>();Json->SetStringField(TEXT("magic"),TEXT("NULLROUTE"));Json->SetStringField(TEXT("nonce"),Nonce);Json->SetNumberField(TEXT("protocol"),Protocol);Json->SetStringField(TEXT("name"),ServerName);Json->SetStringField(TEXT("map"),ServerMap);Json->SetStringField(TEXT("phase"),ServerPhase);Json->SetNumberField(TEXT("port"),ServerPort);Json->SetNumberField(TEXT("players"),ServerPlayers);Json->SetNumberField(TEXT("capacity"),ServerCapacity);
  FString Payload;FJsonSerializer::Serialize(Json,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload));const FTCHARToUTF8 Data(*Payload);int32 Sent=0;if(Data.Length()<=768)AdvertSocket->SendTo(reinterpret_cast<const uint8*>(Data.Get()),Data.Length(),Sent,*Sender);
 }
}
bool UNRServerBrowser::Tick(float)
{
 ReadRequests();ReadReplies();const double Now=FPlatformTime::Seconds();
 if(SearchUntil>0&&Now>=SearchUntil){SearchUntil=0;int Count=0;for(auto& Row:Servers){Count+=Row.bOnline;if(!Row.bOnline)Row.Phase=TEXT("Нет ответа");}Status=FString::Printf(TEXT("Доступно серверов: %d. Повторный поиск — кнопкой «Обновить»."),Count);++Revision;}
 if(bConnecting&&GetWorld()&&GetWorld()->GetNetMode()==NM_Client&&GetWorld()->GetFirstPlayerController()&&GetWorld()->GetFirstPlayerController()->GetPawn()){bConnecting=false;Status=TEXT("Подключено.");++Revision;}
 if(bConnecting&&Now-ConnectionStarted>35){bConnecting=false;Status=TEXT("Сервер не ответил. Проверьте адрес и UDP-порт.");++Revision;}
 return true;
}
void UNRServerBrowser::SaveFavorites(){if(GConfig&&!bTestNoPersistence){GConfig->SetArray(TEXT("NullRoute.Servers"),TEXT("Favorites"),Favorites,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);}}
bool UNRServerBrowser::AddFavorite(const FString& Address)
{
 FString Host,Normal;int32 Port;if(!ParseAddress(Address,Host,Port,Normal)||(!Favorites.Contains(Normal)&&Favorites.Num()>=32)){Status=TEXT("Нужен корректный IPv4 или домен:порт. Максимум 32 адреса.");++Revision;return false;}
 Favorites.AddUnique(Normal);FindOrAdd(Normal).bFavorite=true;SaveFavorites();++Revision;return true;
}
void UNRServerBrowser::RemoveFavorite(const FString& Address){Favorites.Remove(Address);if(auto* Row=Servers.FindByPredicate([&](const auto& E){return E.Address==Address;}))Row->bFavorite=false;SaveFavorites();++Revision;}
bool UNRServerBrowser::Join(APlayerController* Player,const FString& Address)
{
 FString Host,Normal;int32 Port;if(!Player||!ParseAddress(Address,Host,Port,Normal)){Status=TEXT("Некорректный адрес. Пример: 201.51.19.38:7777");++Revision;return false;}
 if(const auto* Row=Servers.FindByPredicate([&](const auto& E){return E.Address==Normal;}))if(Row->bOnline&&(!Row->bCompatible||Row->Players>=Row->Capacity)){Status=Row->bCompatible?TEXT("Сервер заполнен."):TEXT("Другая версия игры. Обновите клиент и сервер.");++Revision;return false;}
 Status=TEXT("Подключение к ")+Normal+TEXT("…");bConnecting=true;ConnectionStarted=FPlatformTime::Seconds();++Revision;
 UGameplayStatics::SetGamePaused(Player,false);Player->ClientTravel(Normal,TRAVEL_Absolute);return true;
}
void UNRServerBrowser::NetworkFailure(UWorld* World,UNetDriver*,ENetworkFailure::Type,const FString& Error)
{if(World&&World->GetGameInstance()!=GetGameInstance())return;bConnecting=false;Status=TEXT("Соединение потеряно: ")+Clean(Error,160);++Revision;}
void UNRServerBrowser::TravelFailure(UWorld* World,ETravelFailure::Type,const FString& Error)
{if(World&&World->GetGameInstance()!=GetGameInstance())return;bConnecting=false;Status=TEXT("Не удалось загрузить сервер: ")+Clean(Error,160);++Revision;}
bool UNRServerBrowser::HasLocalServer(){return LocalServerProcess.IsValid()&&FPlatformProcess::IsProcRunning(LocalServerProcess);}
bool UNRServerBrowser::StartLocalServer()
{
 if(HasLocalServer()){Status=TEXT("Ваш локальный сервер уже запущен.");++Revision;return false;}
 if(LocalServerProcess.IsValid())FPlatformProcess::CloseProc(LocalServerProcess);
 FString Executable=FPlatformProcess::ExecutablePath(),Prefix;
#if WITH_EDITOR
 Executable=FPaths::Combine(FPlatformProcess::BaseDir(),TEXT("UnrealEditor-Cmd.exe"));Prefix=FString::Printf(TEXT("\"%s\" "),*FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
#endif
 const FString Arguments=Prefix+TEXT("/Game/Maps/NR_Switchyard?Training=0?MinPlayers=2?MaxPlayers=10?ServerName=Local -server -nullrhi -nosound -unattended -port=7777 -abslog=\"")+FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("LocalMatchServer.log"))+TEXT("\"");
 uint32 Pid=0;LocalServerProcess=FPlatformProcess::CreateProc(*Executable,*Arguments,true,true,true,&Pid,0,nullptr,nullptr);
 Status=LocalServerProcess.IsValid()?TEXT("Сервер запускается. Через несколько секунд обновите список; адрес 127.0.0.1:7777."):TEXT("Не удалось запустить сервер. Смотрите Saved/LocalMatchServer.log.");++Revision;return LocalServerProcess.IsValid();
}
void UNRServerBrowser::StopLocalServer()
{if(LocalServerProcess.IsValid()){if(FPlatformProcess::IsProcRunning(LocalServerProcess))FPlatformProcess::TerminateProc(LocalServerProcess,false);FPlatformProcess::CloseProc(LocalServerProcess);Status=TEXT("Ваш локальный сервер остановлен.");++Revision;}}
