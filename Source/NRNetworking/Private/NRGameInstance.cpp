#include "NRGameInstance.h"
#include "NRServerBrowser.h"
#include "Misc/NetworkVersion.h"
#include "Engine/Engine.h"
#include "CoreGlobals.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UNRGameInstance::Init()
{
 // Reject old clients before their replicated character layout reaches Iris.
 FNetworkVersion::SetGameNetworkProtocolVersion(UNRServerBrowser::Protocol);
#if !WITH_EDITOR && !UE_SERVER && WITH_SERVER_CODE
 // Launcher Game targets ignore -server in LaunchEngineLoop. Set the process mode
 // before UGameEngine creates its viewport/local player and before loading the map.
 // This is a headless compatibility host; the native Server target needs no override.
 if(FParse::Param(FCommandLine::Get(),TEXT("server")))
 {
  if(!FParse::Param(FCommandLine::Get(),TEXT("nullrhi")))
  {UE_LOG(LogTemp,Fatal,TEXT("Null Route packaged server requires -nullrhi -nosound."));}
  GIsClient=false;GIsServer=true;
  // Game user settings otherwise retain the client's 144 FPS cap in this host.
  // SetByCode wins over graphics settings and keeps the headless simulation at 64 Hz.
  if(auto* Cap=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))Cap->Set(64.f,ECVF_SetByCode);
  if(auto* Context=GetWorldContext())Context->RunAsDedicated=true;
  UE_LOG(LogTemp,Display,TEXT("NR_HEADLESS_HOST enabled; no local player or viewport; max tick 64"));
 }
#endif
 Super::Init();
}
