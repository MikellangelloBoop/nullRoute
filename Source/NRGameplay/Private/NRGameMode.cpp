#include "NRGameMode.h"
#include "NRServerBrowser.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameSession.h"
#include "NRPreparationZone.h"
#include "NRBallisticsSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NRCharacter.h"
#include "NRAttributes.h"
#include "NRNode.h"
#include "NRObjective.h"
#include "NRActivity.h"
#include "NRTacticalDoor.h"
#include "NRStructure.h"
#include "NRCollapseVolume.h"
#include "Navigation/NavLinkProxy.h"
#include "NRDroneDirector.h"
#include "NRHUD.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
void ANRGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRGameState,ConnectedPlayers);DOREPLIFETIME(ANRGameState,MinimumPlayers);DOREPLIFETIME(ANRGameState,MaximumPlayers);DOREPLIFETIME(ANRGameState,Phase);DOREPLIFETIME(ANRGameState,PhaseEnd);DOREPLIFETIME(ANRGameState,Round);DOREPLIFETIME(ANRGameState,EchelonScore);DOREPLIFETIME(ANRGameState,SyndicateScore);DOREPLIFETIME(ANRGameState,Team0Power);DOREPLIFETIME(ANRGameState,Team1Power);DOREPLIFETIME(ANRGameState,AttackTeam);DOREPLIFETIME(ANRGameState,AttackerFaction);DOREPLIFETIME(ANRGameState,DefenderFaction);DOREPLIFETIME(ANRGameState,ElaraControl);DOREPLIFETIME(ANRGameState,Announcement);}
bool ANRGameState::CanPrepareAt(uint8 Team,const FVector& P,float Clearance)const
{
 if(Team>1||P.ContainsNaN()||!FMath::IsFinite(P.X)||!FMath::IsFinite(P.Y)||!FMath::IsFinite(P.Z))return false;
 if(Phase!=ENRPhase::Preparation)return true;
 return Team==AttackTeam?P.X<=PreparationBoundaryX-Clearance:P.X>=PreparationBoundaryX+Clearance;
}
bool ANRGameState::SpendPower(uint8 Team,int32 Amount){if(!HasAuthority()||Team>1)return false;int32& Pool=Team==0?Team0Power:Team1Power;if(Amount>Pool)return false;Pool=FMath::Clamp(Pool-Amount,0,100);return true;}
ANRGameMode::ANRGameMode(){GameStateClass=ANRGameState::StaticClass();DefaultPawnClass=ANRCharacter::StaticClass();HUDClass=ANRHUD::StaticClass();}
void ANRGameMode::InitGame(const FString& Map,const FString& Options,FString& Error){Super::InitGame(Map,Options,Error);TrainingTeam=uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options,TEXT("TrainingTeam"),1),0,1));Seed=UGameplayStatics::GetIntOption(Options,TEXT("Seed"),137);bTraining=UGameplayStatics::GetIntOption(Options,TEXT("Training"),1)!=0;
 PreparationSeconds=FMath::Clamp(float(UGameplayStatics::GetIntOption(Options,TEXT("PrepSeconds"),int32(PreparationSeconds))),5.f,120.f);
 MaximumPlayers=FMath::Clamp(UGameplayStatics::GetIntOption(Options,TEXT("MaxPlayers"),10),2,10);MinimumPlayers=FMath::Clamp(UGameplayStatics::GetIntOption(Options,TEXT("MinPlayers"),2),2,MaximumPlayers);
 const FString Name=UGameplayStatics::ParseOption(Options,TEXT("ServerName"));if(!Name.IsEmpty())ServerName=Name.Left(48);if(GameSession)GameSession->MaxPlayers=MaximumPlayers;}
void ANRGameMode::StartPlay(){const auto Pair=NRFactions::WithOptions(GetDefault<UNRFactionSettings>()->Resolve(GetWorld()->GetMapName()),OptionsString);auto* State=GetGameState<ANRGameState>();State->AttackerFaction=Pair.Attackers;State->DefenderFaction=Pair.Defenders;State->MinimumPlayers=MinimumPlayers;State->MaximumPlayers=MaximumPlayers;Super::StartPlay();PreparationZone=GetWorld()->SpawnActor<ANRPreparationZone>(FVector(ANRGameState::PreparationBoundaryX,0,500),FRotator::ZeroRotator);GetWorldTimerManager().SetTimer(MatchTimer,this,&ANRGameMode::UpdateMatch,1.f,true);if(bTraining)BeginRound();UpdateServerListing();}
void ANRGameMode::PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& Id,FString& Error)
{
 Super::PreLogin(Options,Address,Id,Error);if(!bTraining&&GetNumPlayers()>=MaximumPlayers)Error=TEXT("SERVER_FULL");
}
bool ANRGameMode::HasEnoughPlayers()const
{
 if(bTraining)return true;int32 Count[2]={0,0};for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->Controller&&It->Team<2)++Count[It->Team];
 return Count[0]>0&&Count[1]>0&&Count[0]+Count[1]>=MinimumPlayers;
}
void ANRGameMode::PostLogin(APlayerController* PC)
{
 Super::PostLogin(PC);
 if(!bTraining&&GetNumPlayers()>MaximumPlayers){if(GameSession)GameSession->KickPlayer(PC,FText::FromString(TEXT("SERVER_FULL")));return;}
 if(!PC->GetPawn())RestartPlayer(PC);
 if(auto* C=Cast<ANRCharacter>(PC->GetPawn()))
 {
  int32 Count[2]={0,0};for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(*It!=C&&It->Controller&&It->Team<2)++Count[It->Team];
  C->Team=bTraining?TrainingTeam:Count[0]<=Count[1]?0:1;C->SetRole(C->OperatorRole);RestartPlayer(PC);
  const auto Phase=GetGameState<ANRGameState>()->Phase;
  // Reconnecting during combat cannot grant an extra life. The next round restores this pawn.
  if(!bTraining&&(Phase==ENRPhase::Combat||Phase==ENRPhase::Resolution||Phase==ENRPhase::MatchOver)){C->bDead=true;C->GetCharacterMovement()->DisableMovement();C->SetActorEnableCollision(false);C->SetActorHiddenInGame(true);C->ForceNetUpdate();}
 }
 if(!bTraining&&GetGameState<ANRGameState>()->Phase==ENRPhase::Waiting&&HasEnoughPlayers())BeginRound();UpdateServerListing();
}
void ANRGameMode::Logout(AController* Exiting)
{
 auto* Leaving=Exiting?Cast<ANRCharacter>(Exiting->GetPawn()):nullptr;
 if(Leaving){Leaving->bCarryingDisk=false;for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(!It->bExtraction&&It->bTaken){bool Carrier=false;for(TActorIterator<ANRCharacter> P(GetWorld());P;++P)Carrier|=*P!=Leaving&&P->bCarryingDisk;if(!Carrier)It->DropAt(Leaving->GetActorLocation()-FVector(0,0,48));}}
 Super::Logout(Exiting);if(bTraining)return;
 int32 Teams[2]={0,0};for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(*It!=Leaving&&It->Controller&&It->Team<2)++Teams[It->Team];
 auto* S=GetGameState<ANRGameState>();
 if(Teams[0]+Teams[1]==0||((S->Phase==ENRPhase::Preparation||S->Phase==ENRPhase::Waiting)&&(Teams[0]==0||Teams[1]==0)))
 {GetWorldTimerManager().ClearTimer(PhaseTimer);S->Phase=ENRPhase::Waiting;S->PhaseEnd=0;S->Announcement=TEXT("WAITING FOR PLAYERS");if(PreparationZone)PreparationZone->SetPreparing(true);S->ForceNetUpdate();}
 else if(S->Phase==ENRPhase::Combat&&(Teams[0]==0||Teams[1]==0))ResolveRound(Teams[0]>0?0:1,TEXT("TEAM DISCONNECTED"));UpdateServerListing();
}
void ANRGameMode::RestartPlayer(AController* C)
{
 if(!C->GetPawn())Super::RestartPlayer(C);auto* P=Cast<ANRCharacter>(C->GetPawn());if(!P)return;
 const auto* GS=GetGameState<ANRGameState>();
 const FName Tag=GS&&P->Team==GS->AttackTeam?TEXT("Team1"):TEXT("Team0");FCollisionQueryParams Q;Q.AddIgnoredActor(P);
 for(TActorIterator<APlayerStart> It(GetWorld());It;++It)if(It->PlayerStartTag==Tag)
 {
  // Try a bounded grid around the side's spawn: five teammates never share a capsule.
  for(int Slot=0;Slot<15;++Slot)
  {
   const FVector At=It->GetActorLocation()+FVector((Slot/5)*110,(Slot%5-2)*130,0);
   if(GS&&!GS->CanPrepareAt(P->Team,At))continue;
   if(GetWorld()->OverlapBlockingTestByChannel(At,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q))continue;
   if(P->TeleportTo(At,It->GetActorRotation())){C->SetControlRotation(It->GetActorRotation());P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);return;}
  }
 }
}
bool ANRGameMode::IsCombat()const{const auto* S=GetGameState<ANRGameState>();return S&&S->Phase==ENRPhase::Combat;}
void ANRGameMode::BeginRound(){if(!bTraining&&!HasEnoughPlayers()){auto* State=GetGameState<ANRGameState>();State->Phase=ENRPhase::Waiting;State->PhaseEnd=0;State->ForceNetUpdate();return;}for(TActorIterator<ANRCollapseVolume> It(GetWorld());It;++It)It->Destroy();for(TActorIterator<ANavLinkProxy> It(GetWorld());It;++It)It->Destroy();auto* S=GetGameState<ANRGameState>();++S->Round;S->AttackTeam=bTraining||S->Round<=6?1:0;S->Phase=ENRPhase::Preparation;S->PhaseEnd=S->GetServerWorldTimeSeconds()+FMath::Clamp(PreparationSeconds,5.f,120.f);S->Announcement=TEXT("PREPARATION // DEFEND THE SITE / PLAN THE ATTACK");if(PreparationZone)PreparationZone->SetPreparing(true);GetWorld()->GetSubsystem<UNRBallisticsSubsystem>()->ClearProjectiles();S->ForceNetUpdate();S->Team0Power=S->Team1Power=100;
 TArray<ANodeBase*> Nodes;for(TActorIterator<ANodeBase> It(GetWorld());It;++It)Nodes.Add(*It);for(auto* N:Nodes)N->Destroy();SpawnBUP();
 for(TActorIterator<ANRCharacter> It(GetWorld());It;++It){It->SetActorHiddenInGame(false);It->SetActorEnableCollision(true);It->ResetForRound();if(It->Controller)RestartPlayer(It->Controller);}
 for(TActorIterator<ANRObjective> It(GetWorld());It;++It)It->ResetObjective();for(TActorIterator<ANRActivity> It(GetWorld());It;++It)It->ResetActivity();
 for(TActorIterator<ANRTacticalDoor> It(GetWorld());It;++It)It->ResetDoor();
 for(TActorIterator<ANRStructure> It(GetWorld());It;++It){It->ResetForRound();}
 for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)It->ResetWave();
 GetWorldTimerManager().SetTimer(PhaseTimer,this,&ANRGameMode::BeginCombat,FMath::Clamp(PreparationSeconds,5.f,120.f),false);}
void ANRGameMode::BeginCombat(){auto* S=GetGameState<ANRGameState>();S->Phase=ENRPhase::Combat;
 // Discard every practice projectile before opening the gate; retain all prepared nodes.
 GetWorld()->GetSubsystem<UNRBallisticsSubsystem>()->ClearProjectiles();
 if(PreparationZone)PreparationZone->SetPreparing(false);
 for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->IsAlive()){It->RefillAmmunition();It->AbilityReadyTime=0;It->ForceNetUpdate();}
 S->ForceNetUpdate();S->PhaseEnd=S->GetServerWorldTimeSeconds()+180;S->Announcement=TEXT("ELARA IS WAITING // RECOVER THE DISK");GetWorldTimerManager().SetTimer(PhaseTimer,[this](){auto* G=GetGameState<ANRGameState>();ResolveRound(1-G->AttackTeam,TEXT("TIME EXPIRED"));},180.f,false);for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)It->Say(5);}
void ANRGameMode::ResolveRound(uint8 Winner,const FString& Why){auto* S=GetGameState<ANRGameState>();if(S->Phase!=ENRPhase::Combat)return;GetWorldTimerManager().ClearTimer(PhaseTimer);if(Winner==0)++S->EchelonScore;else ++S->SyndicateScore;S->Announcement=FString(NRFactions::Name(S->FactionForTeam(Winner)))+TEXT(" // ")+Why;
 S->Phase=FMath::Max(S->EchelonScore,S->SyndicateScore)>=7?ENRPhase::MatchOver:ENRPhase::Resolution;S->PhaseEnd=S->GetServerWorldTimeSeconds()+(S->Phase==ENRPhase::MatchOver?15.f:7.f);
 if(S->Phase==ENRPhase::Resolution)GetWorldTimerManager().SetTimer(PhaseTimer,this,&ANRGameMode::BeginRound,7.f,false);else GetWorldTimerManager().SetTimer(PhaseTimer,this,&ANRGameMode::RestartMatch,15.f,false);}
void ANRGameMode::PlayerKilled(ANRCharacter* Victim,AController* Killer){if(auto* C=Killer?Cast<ANRCharacter>(Killer->GetPawn()):nullptr){C->GiveCredits(200);if(C!=Victim&&C->Team!=Victim->Team)++C->Kills;}
 if(Victim->bCarryingDisk){Victim->bCarryingDisk=false;for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(!It->bExtraction){It->DropAt(Victim->GetActorLocation()-FVector(0,0,48));break;}}
 if(!IsCombat())return;int32 Alive[2]={0,0};for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->IsAlive()&&It->Team<2)++Alive[It->Team];if(Alive[Victim->Team]==0)ResolveRound(1-Victim->Team,TEXT("TEAM ELIMINATED"));}
void ANRGameMode::ExtractDisk(ANRCharacter* P){if(P&&P->bCarryingDisk&&P->Team==GetGameState<ANRGameState>()->AttackTeam){P->Say(7);P->bCarryingDisk=false;P->GiveCredits(300);ResolveRound(P->Team,TEXT("ELARA EXTRACTED"));}}
void ANRGameMode::SpawnBUP(){FRandomStream R(Seed+GetGameState<ANRGameState>()->Round*7919);TArray<FVector> Anchors{{-700,-600,70},{700,600,70},{-600,600,70},{600,-600,70},{0,700,70},{0,-700,70}};
 if(GetWorld()->GetMapName().Contains(TEXT("Switchyard")))Anchors={{-450,1050,70},{350,1700,70},{-350,-1500,70},{450,-1800,70},{750,100,70},{-750,-100,70}};
 for(int32 I=Anchors.Num()-1;I>0;--I)Anchors.Swap(I,R.RandRange(0,I));for(int32 I=0;I<3;++I){auto* N=GetWorld()->SpawnActorDeferred<ANodeBase>(ANodeBase::StaticClass(),FTransform(Anchors[I]));N->Initialize(255,ENRNodeOp::Relay,true);N->NodeID=FGuid(Seed,GetGameState<ANRGameState>()->Round,I+1,0xB0F);N->FinishSpawning(FTransform(Anchors[I]));N->CompletePrinting();}}

void ANRGameMode::UpdateMatch(){UpdateServerListing();auto* S=GetGameState<ANRGameState>();int32 Counts[2]={0,0};for(TActorIterator<ANodeBase> It(GetWorld());It;++It)if(It->NodeState==ENRNodeState::Active&&It->Team<2)++Counts[It->Team];if(S->Phase==ENRPhase::Combat){S->Team0Power=FMath::Min(100,S->Team0Power+Counts[0]);S->Team1Power=FMath::Min(100,S->Team1Power+Counts[1]);}S->ElaraControl=Counts[0]+Counts[1]>0?float(Counts[1])/float(Counts[0]+Counts[1]):.5f;}


void ANRGameMode::UpdateServerListing()
{
 auto* S=GetGameState<ANRGameState>();if(!S)return;S->ConnectedPlayers=GetNumPlayers();
 if(GetNetMode()!=NM_DedicatedServer&&GetNetMode()!=NM_ListenServer)return;
 const TCHAR* Phase=S->Phase==ENRPhase::Waiting?TEXT("Ожидание игроков"):S->Phase==ENRPhase::Preparation?TEXT("Подготовка"):S->Phase==ENRPhase::Combat?TEXT("Раунд идёт"):S->Phase==ENRPhase::Resolution?TEXT("Смена раунда"):TEXT("Матч завершён");
 if(auto* Browser=GetGameInstance()->GetSubsystem<UNRServerBrowser>())Browser->Advertise(ServerName,GetWorld()->GetMapName(),GetWorld()->URL.Port,GetNumPlayers(),MaximumPlayers,Phase);
}
void ANRGameMode::RestartMatch()
{
 auto* S=GetGameState<ANRGameState>();S->Round=0;S->EchelonScore=0;S->SyndicateScore=0;
 for(TActorIterator<ANRCharacter> It(GetWorld());It;++It){It->Kills=0;It->Deaths=0;It->Attributes->SetHardwareTokens(600);}BeginRound();
}
