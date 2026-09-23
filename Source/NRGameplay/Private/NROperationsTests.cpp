#include "NRSmokeSubsystem.h"
#include "NRCharacter.h"
#include "NRCharacterMovement.h"
#include "NRAttributes.h"
#include "NRDroneDirector.h"
#include "NRGameMode.h"
#include "NRBallisticsSubsystem.h"
#include "NRHUD.h"
#include "MassEntitySubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include <limits>

void UNRSmokeSubsystem::RunOperationsTest(ANRCharacter* C,APlayerController* PC,float T)
{
 if(T<3||!C||!PC)return;
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_OPERATIONS %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 ANRDroneDirector* Director=nullptr;for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It){Director=*It;break;}if(!Director){Check(false,TEXT("map contains Mass director"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();auto* Move=CastChecked<UNRCharacterMovement>(C->GetCharacterMovement());
 if(Stage==1)
 {
  if(T<7)return;Stage=2;int Moved=0;bool Clear=true;
  for(auto H:Director->Entities){const auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(H);Moved+=FVector::DistSquared(D.Position,D.Home)>25.f*25.f;Clear&=!GetWorld()->OverlapBlockingTestByChannel(D.Position,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(18));FHitResult Hit;GetWorld()->SweepSingleByChannel(Hit,D.Position,D.Position+FVector(0,0,1),FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(18));UE_LOG(LogTemp,Display,TEXT("NR_PATROL_DIAG pos=%s home=%s target=%s path=%d state=%d hit=%s component=%s"),*D.Position.ToCompactString(),*D.Home.ToCompactString(),*D.Target.ToCompactString(),D.bHasPath,int(D.State),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()));}
  Check(Moved>=3,TEXT("live Mass processors move patrols along navigation"));Check(Clear,TEXT("live drone movement keeps collision clearance"));
  UE_LOG(LogTemp,Display,TEXT("NR_OPERATIONS_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);return;
 }
 if(Stage>1)return;Stage=1;
 GetWorld()->GetTimerManager().ClearTimer(Director->WaveTimer);auto* GS=GetWorld()->GetGameState<ANRGameState>();GS->Phase=ENRPhase::Combat;
 Director->ResetWave();Check(Director->Entities.Num()==24&&Director->Views.Num()==24,TEXT("bounded 24 entity wave"));
 bool Types[3]={false,false,false};for(auto H:Director->Entities){auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(H);Types[D.Kind]=true;}
 Check(Types[0]&&Types[1]&&Types[2],TEXT("three distinct drone roles"));Check(Director->Bodies.Num()==3&&Director->Bodies[0]->GetStaticMesh()&&Director->Bodies[1]->GetStaticMesh()&&Director->Bodies[2]->GetStaticMesh(),TEXT("three authored drone models loaded"));
 auto Box=[&](FVector P,FVector Size){auto* A=GetWorld()->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);B->SetBoxExtent(Size);B->SetCollisionProfileName(TEXT("BlockAll"));B->RegisterComponent();A->SetActorLocation(P);return A;};
 auto* Floor=Box(FVector(0,5000,-10),FVector(1500,1500,10));C->TeleportTo(FVector(200,5000,88),FRotator::ZeroRotator);PC->SetControlRotation(FRotator(0,180,0));Move->StopMovementImmediately();Move->SetMovementMode(MOVE_Walking);
 auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(Director->Entities[0]);D.Position=FVector(-400,5000,110);D.Home=D.Target=D.Position;D.Yaw=0;
 Move->Velocity=FVector(150,0,0);C->QuietPressed();Check(ANRDroneDirector::HearingRadius(C)==140,TEXT("Alt reduces actual drone hearing radius"));C->QuietReleased();Check(ANRDroneDirector::HearingRadius(C)==750,TEXT("normal steps carry farther"));Move->StopMovementImmediately();Check(ANRDroneDirector::HearingRadius(C)==0,TEXT("stationary player makes no footsteps"));
 Check(Director->CanSee(D,C),TEXT("unobstructed player inside vision cone detected"));D.Yaw=180;Check(!Director->CanSee(D,C),TEXT("player behind drone stays outside vision cone"));D.Yaw=0;
 auto* Wall=Box(FVector(-100,5000,120),FVector(12,100,160));Check(!Director->CanSee(D,C),TEXT("solid cover blocks drone vision"));
 TArray<ANRCharacter*> Targets{C};Director->NextSquadAttack=T+100;Move->Velocity=FVector(150,0,0);C->QuietPressed();D.LastSeen=-100;Director->Sense(0,D,Targets,T);Check(D.State==ENRDroneState::Patrol,TEXT("quiet walk behind cover does not alert distant patrol"));
 C->QuietReleased();Director->Sense(0,D,Targets,T);Check(D.State==ENRDroneState::Search,TEXT("audible movement behind cover causes investigation"));Move->StopMovementImmediately();
 D.Position=FVector(-800,5000,110);D.State=ENRDroneState::Patrol;C->ReportWeaponNoise(true);Check(D.State==ENRDroneState::Patrol,TEXT("suppressor avoids alert beyond reduced sound radius"));C->ReportWeaponNoise(false);Check(D.State==ENRDroneState::Search,TEXT("unsuppressed shot attracts patrol through sound"));
 Wall->Destroy();D.Position=FVector(-400,5000,110);D.State=ENRDroneState::Patrol;D.Suspicion=0;D.Yaw=0;Director->Sense(0,D,Targets,T);Check(D.State==ENRDroneState::Search&&D.Suspicion<1,TEXT("recognition builds suspicion before attack"));
 Director->Sense(0,D,Targets,T);Director->Sense(0,D,Targets,T);Check(D.State==ENRDroneState::Alert,TEXT("sustained sight confirms target"));
 D.LastSeen=T-8;D.Yaw=180;Director->Sense(0,D,Targets,T);Check(D.State==ENRDroneState::Patrol&&!D.Tracked.IsValid(),TEXT("lost target is forgotten and patrol resumes"));
 D.Yaw=0;D.State=ENRDroneState::Alert;D.Suspicion=1;Director->NextSquadAttack=0;D.NextAttack=0;Director->Sense(0,D,Targets,T);
 Check(D.State==ENRDroneState::Charging&&D.FireAt>T+.8f,TEXT("ranged attack has visible reaction window"));
 const float Health=C->Attributes->GetHealth();Director->UpdateWave();Check(C->Attributes->GetHealth()==Health,TEXT("windup cannot inflict early damage"));
 Wall=Box(FVector(-100,5000,120),FVector(12,100,160));D.FireAt=T-.1f;Director->UpdateWave();Check(C->Attributes->GetHealth()==Health,TEXT("cover blocks live charged shot"));Wall->Destroy();
 D.State=ENRDroneState::Charging;D.FireAt=T-.1f;D.AimPoint=C->GetActorLocation()+FVector(0,0,30);C->TeleportTo(FVector(200,5300,88),FRotator::ZeroRotator);Director->UpdateWave();Check(C->Attributes->GetHealth()==Health,TEXT("sidestep dodges fixed aim point"));
 C->TeleportTo(FVector(200,5000,88),FRotator::ZeroRotator);D.State=ENRDroneState::Charging;D.FireAt=T-.1f;D.AimPoint=C->GetActorLocation()+FVector(0,0,30);Director->UpdateWave();Check(C->Attributes->GetHealth()<Health,TEXT("unavoided shot deals authoritative damage"));
 Check(FMath::Abs(FMath::FindDeltaAngleDegrees(C->DamageSourceYaw,180))<5,TEXT("damage direction points at drone shot origin"));
 Director->DisableSecurity(12);bool Disabled=true;for(auto H:Director->Entities)Disabled&=M.GetFragmentDataChecked<FNRDroneFragment>(H).State==ENRDroneState::Disabled;
 Check(Disabled&&Director->BlackoutUntil>T+11&&Director->AlertCount==0,TEXT("relay blackout disables the entire bounded wave"));const float After=C->Attributes->GetHealth();Director->UpdateWave();Check(C->Attributes->GetHealth()==After,TEXT("disabled security cannot shoot"));
 D.DisabledUntil=T-1;Director->UpdateWave();Check(D.State!=ENRDroneState::Disabled,TEXT("security recovers after blackout"));
 Check(!Director->HitDrone(-1,10,C)&&!Director->HitDrone(1000,10,C)&&!Director->HitDrone(0,std::numeric_limits<float>::quiet_NaN(),C),TEXT("invalid drone damage rejected"));
 D.Health=15;D.State=ENRDroneState::Patrol;Director->Publish(0,D);Director->OnRep_Views();const float Credits=C->Attributes->GetHardwareTokens();const int Kills=C->DroneKills;
 Check(Director->HitDrone(0,5,C)&&C->Attributes->GetHardwareTokens()==Credits,TEXT("nonlethal hits cannot farm credits"));
 auto* Bullets=GetWorld()->GetSubsystem<UNRBallisticsSubsystem>();Bullets->Bullets.Reset();const FVector Eye=C->GetPawnViewLocation();Bullets->Fire(C,Eye,(D.Position-Eye).GetSafeNormal(),100,40000);Bullets->Tick(.05f);
 Check(D.Health==0&&!Director->Views[0].bAlive&&C->DroneKills==Kills+1&&C->Attributes->GetHardwareTokens()==Credits+30,TEXT("live projectile kills drone and pays once"));
 Check(!Director->HitDrone(0,100,C)&&C->Attributes->GetHardwareTokens()==Credits+30,TEXT("destroyed drone cannot pay again"));
 Check(C->bLastHitKilled&&C->CombatNoticeUntil>T,TEXT("drone kill produces confirmed HUD feedback"));
 Wall=Box(FVector(-100,5000,150),FVector(12,100,100));PC->SetControlRotation(FRotator(0,180,0));C->LastPing=-100;C->ServerPing();
 Check(C->PingUntil>T&&FMath::IsNearlyEqual(float(C->TacticalPing.X),-88.f,2.f),TEXT("ping derives position from server trace"));const FVector Ping=C->TacticalPing;PC->SetControlRotation(FRotator(0,0,0));C->ServerPing();Check(C->TacticalPing.Equals(Ping),TEXT("rapid ping requests are rate limited"));
 C->bDead=true;C->LastPing=-100;C->ServerPing();Check(C->TacticalPing.Equals(Ping),TEXT("dead player cannot submit marker"));C->bDead=false;
 Director->ResetWave();Check(Director->AlertCount==0&&Director->BlackoutUntil==0&&Director->Views[0].Health==100,TEXT("new round resets security state and health"));
 Wall->Destroy();Floor->Destroy();C->ResetForRound();Check(C->DroneKills==0,TEXT("new round clears drone score"));
 C->TeleportTo(FVector(-1850,700,90),FRotator::ZeroRotator);Move->StopMovementImmediately();GetWorld()->GetTimerManager().SetTimer(Director->WaveTimer,Director,&ANRDroneDirector::UpdateWave,.1f,true);
}

void UNRSmokeSubsystem::RunOperationsNetwork(ANRCharacter* C,APlayerController* PC,float T)
{
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)
 {
  if(Stage==0&&T>1)if(auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>()){Stage=1;GetWorld()->GetTimerManager().ClearTimer(GM->PhaseTimer);}
  for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->GetPlayerState())
  {
   const FString Name=It->GetPlayerState()->GetPlayerName();if(Name.Contains(TEXT("OpsEnemy"))&&It->Team!=0){It->Team=0;It->ForceNetUpdate();}
   if(Name.Contains(TEXT("OpsOwner"))&&It->LastPing>0&&!(NetworkStage&1)){NetworkStage|=1;UE_LOG(LogTemp,Display,TEXT("NR_NETOPS_SERVER accepted bounded ping : PASS"));}
  }
  if(!(NetworkStage&2))for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)if(It->Views.Num()==24&&It->Bodies.IsEmpty()&&!It->WarningLines){NetworkStage|=2;UE_LOG(LogTemp,Display,TEXT("NR_NETOPS_SERVER ECS without visual components : PASS"));}
  return;
 }
 if(!C||!PC||C->HasAuthority())return;
 if(NetworkStage==0){NetworkStage=1;PC->ServerChangeName(FParse::Param(FCommandLine::Get(),TEXT("NROpsEnemy"))?TEXT("OpsEnemy"):FParse::Param(FCommandLine::Get(),TEXT("NROpsObserver"))?TEXT("OpsObserver"):TEXT("OpsOwner"));}
 auto Check=[this](bool OK,const TCHAR* Name){bFailed|=!OK;UE_LOG(LogTemp,Display,TEXT("NR_NETOPS %s : %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(Stage==0)
 {
  int Players=0;bool EnemyReady=false;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It){++Players;EnemyReady|=It->Team==0&&It->GetPlayerState()&&It->GetPlayerState()->GetPlayerName().Contains(TEXT("OpsEnemy"));}
  if(Players<3||!EnemyReady||T<3)return;Stage=1;OperationsEpoch=T;
  for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)Check(It->Views.Num()==24&&It->Bodies.Num()==3,TEXT("drone variants replicate to client"));
  if(!FParse::Param(FCommandLine::Get(),TEXT("NROpsObserver"))&&!FParse::Param(FCommandLine::Get(),TEXT("NROpsEnemy")))C->ServerPing();
 }
 if(Stage==1&&T>OperationsEpoch+5)
 {
  Stage=2;const bool Enemy=FParse::Param(FCommandLine::Get(),TEXT("NROpsEnemy"));Check(Enemy?C->Team==0&&C->PingUntil==0:C->Team==1&&C->PingUntil>T,Enemy?TEXT("enemy receives no teammate marker"):TEXT("same team receives server validated marker"));
  UE_LOG(LogTemp,Display,TEXT("NR_NETOPS_COMPLETE %s"),bFailed?TEXT("FAILED"):TEXT("PASSED"));FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
 }
}

void UNRSmokeSubsystem::RunOperationsVisual(ANRCharacter* C,APlayerController* PC,float T)
{
 if(!C||!PC)return;ANRDroneDirector* D=nullptr;for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It){D=*It;break;}if(!D)return;auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
 if(Stage==0&&T>3)
 {
  Stage=1;GetWorld()->GetTimerManager().ClearTimer(D->WaveTimer);C->SetRole(ENRRole::Assault);C->ServerSelectSkin(0);C->bSkinLoaded=true;C->TeleportTo(FVector(-2000,1400,110),FRotator::ZeroRotator);
  for(int I=0;I<D->Entities.Num();++I){auto& F=M.GetFragmentDataChecked<FNRDroneFragment>(D->Entities[I]);F.bHasPath=false;F.Health=I<3?60:0;F.Position=I==0?FVector(-1520,1380,110):I==1?FVector(-1450,1550,110):FVector(-1500,1770,110);F.Yaw=180;F.State=I==2?ENRDroneState::Charging:ENRDroneState::Search;F.Suspicion=I==2?1:.4f;F.AimPoint=C->GetActorLocation();D->Publish(I,F);}D->AlertCount=1;D->OnRep_Views();PC->SetControlRotation((FVector(D->Views[1].Position)-C->GetPawnViewLocation()).Rotation());C->ClientReceivePing(D->Views[0].Position,true);
 }
 if(Stage==1&&T>7){Stage=2;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OperationsCombat.png"),true,false);}
 if(Stage==2&&T>8){Stage=3;D->HitDrone(0,200,C);}
 if(Stage==3&&T>8.6f){Stage=4;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OperationsFeedback.png"),true,false);}
 if(Stage==4&&T>10){Stage=5;D->DisableSecurity(12);}
 if(Stage==5&&T>12){Stage=6;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OperationsBlackout.png"),true,false);}
 if(Stage==6&&T>13){Stage=7;auto* Cam=GetWorld()->SpawnActor<ACameraActor>(FVector(2200,-1650,620),(FVector(0,100,180)-FVector(2200,-1650,620)).Rotation());Cam->GetCameraComponent()->SetFieldOfView(80);PC->SetViewTarget(Cam);}
 if(Stage==7&&T>15){Stage=8;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OperationsArena.png"),true,false);}
 if(Stage==8&&T>17){Stage=9;UE_LOG(LogTemp,Display,TEXT("NR_OPERATIONS_VISUAL_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0);}
}
