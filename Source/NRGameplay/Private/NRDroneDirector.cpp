#include "NRDroneDirector.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRCombatFX.h"
#include "NRDebrisSubsystem.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

namespace {float MaxHealth(uint8 K){return K==2?110.f:K==1?45.f:65.f;}float MoveSpeed(uint8 K){return K==2?120.f:K==1?245.f:175.f;}}
UNRDroneProcessor::UNRDroneProcessor():Query(*this)
{ExecutionFlags=int32(EProcessorExecutionFlags::Server|EProcessorExecutionFlags::Standalone);bRequiresGameThreadExecution=true;}
void UNRDroneProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>&){Query.AddRequirement<FNRDroneFragment>(EMassFragmentAccess::ReadWrite);}
void UNRDroneProcessor::Execute(FMassEntityManager& M,FMassExecutionContext& C)
{
 // ECS movement stays contiguous; world sweeps and director mutations share the game thread.
 UWorld* World=GetWorld();Query.ForEachEntityChunk(M,C,[World](FMassExecutionContext& E){auto Data=E.GetMutableFragmentView<FNRDroneFragment>();const float Dt=FMath::Min(E.GetDeltaTimeSeconds(),.05f);
 for(auto& D:Data)if(D.Health>0&&D.bHasPath&&D.State!=ENRDroneState::Charging&&D.State!=ENRDroneState::Disabled)
 {
  const FVector Delta=D.Target-D.Position,Step=Delta.GetClampedToMaxSize(D.Speed*Dt);FHitResult Hit;
  if(World->SweepSingleByChannel(Hit,D.Position,D.Position+Step,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(20)))D.bHasPath=false;
  else D.Position+=Step;
  if(Delta.SizeSquared2D()>4)D.Yaw=FMath::FixedTurn(D.Yaw,Delta.Rotation().Yaw,Dt*150);
 }});
}
ANRDroneDirector::ANRDroneDirector()
{
 bReplicates=true;bAlwaysRelevant=true;SetNetUpdateFrequency(10);PrimaryActorTick.bCanEverTick=true;
 Instances=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Drones"));SetRootComponent(Instances);
 Instances->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_SentinelDrone.SM_SentinelDrone")));Instances->SetHiddenInGame(true);
 Instances->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Instances->SetCollisionResponseToAllChannels(ECR_Ignore);
 Instances->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);Instances->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Instances->SetCanEverAffectNavigation(false);
}
void ANRDroneDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRDroneDirector,Views);DOREPLIFETIME(ANRDroneDirector,AlertCount);DOREPLIFETIME(ANRDroneDirector,BlackoutUntil);}
void ANRDroneDirector::BeginPlay()
{
 Super::BeginPlay();SetActorTickEnabled(GetNetMode()!=NM_DedicatedServer);
 if(GetNetMode()!=NM_DedicatedServer)
 {
  auto Make=[&](const TCHAR* Path){auto* V=NewObject<UInstancedStaticMeshComponent>(this);V->SetupAttachment(Instances);V->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->SetCanEverAffectNavigation(false);V->RegisterComponent();return V;};
  for(const TCHAR* Path:{TEXT("/Game/Art/Meshes/SM_DroneWatcher"),TEXT("/Game/Art/Meshes/SM_DroneHunter"),TEXT("/Game/Art/Meshes/SM_DroneBastion")})Bodies.Add(Make(Path));
  Rotors=Make(TEXT("/Game/Art/Meshes/SM_DroneRotor"));WarningLines=Make(TEXT("/Engine/BasicShapes/Cube.Cube"));WarningLines->SetCastShadow(false);WarningLines->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_AmberLight")));
 }
 if(HasAuthority())
 {
  auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();TArray<const UScriptStruct*> Types{FNRDroneFragment::StaticStruct()};Archetype=M.CreateArchetype(Types);
  ResetWave();GetWorldTimerManager().SetTimer(WaveTimer,this,&ANRDroneDirector::UpdateWave,.1f,true);
 }
}
void ANRDroneDirector::EndPlay(EEndPlayReason::Type R)
{GetWorldTimerManager().ClearTimer(WaveTimer);if(HasAuthority())if(auto* S=GetWorld()->GetSubsystem<UMassEntitySubsystem>()){auto& M=S->GetMutableEntityManager();for(auto H:Entities)if(M.IsEntityValid(H))M.DestroyEntity(H);}Super::EndPlay(R);}
void ANRDroneDirector::ResetWave()
{
 if(!HasAuthority()||!Archetype.IsValid())return;auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
 for(auto H:Entities)if(M.IsEntityValid(H))M.DestroyEntity(H);Entities.Reset();Views.Reset();BlackoutUntil=0;AlertCount=0;NextSquadAttack=0;QueryCursor=0;
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 // Stable lanes leave both team spawns clear. No random spawn inside newly added cover.
 for(int I=0;I<24;++I)
 {
  auto H=M.CreateEntity(Archetype);auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(H);D.Kind=I%3;D.Health=MaxHealth(D.Kind);D.Speed=MoveSpeed(D.Kind);
  const FVector Anchor(-1050+(I%6)*420,(I/6-1.5f)*540,84);bool Placed=false;
  for(int Attempt=0;Attempt<17&&!Placed;++Attempt)
  {
   const float Angle=(Attempt-1)*PI/4+I*.31f,Radius=Attempt==0?0.f:Attempt<=8?100.f:210.f;
   FVector Candidate=Anchor+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);FNavLocation P;
   if(Nav&&Nav->ProjectPointToNavigation(Candidate,P,FVector(90,90,200)))Candidate=P.Location+FVector(0,0,82);
   if(!GetWorld()->OverlapBlockingTestByChannel(Candidate,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(26))){D.Position=Candidate;Placed=true;}
  }
  if(!Placed){D.Position=Anchor;D.Health=0;} // No valid pocket means no active unit, never a stuck collider inside cover.
  D.Home=D.Target=D.Position;D.Yaw=I%2?0:180;Entities.Add(H);Views.AddDefaulted();Publish(I,D);
 }
 OnRep_Views();ForceNetUpdate();
}
const TCHAR* ANRDroneDirector::KindName(uint8 K){return K==2?TEXT("БАСТИОН"):K==1?TEXT("ОХОТНИК"):TEXT("НАБЛЮДАТЕЛЬ");}
float ANRDroneDirector::HearingRadius(const ANRCharacter* C)
{if(!C||C->GetVelocity().Size2D()<25)return 0;return C->IsQuietWalking()?140.f:C->bIsCrouched?240.f:C->IsSprinting()?1450.f:750.f;}
bool ANRDroneDirector::CanSee(const FNRDroneFragment& D,const ANRCharacter* C)const
{
 if(!C||!C->IsAlive())return false;const FVector Eye=C->GetPawnViewLocation(),Delta=Eye-D.Position;
 const float Range=D.Kind==0?1900.f:1500.f;if(Delta.SizeSquared()>Range*Range)return false;
 if(Delta.SizeSquared2D()>280.f*280.f&&FVector::DotProduct(FRotator(0,D.Yaw,0).Vector(),Delta.GetSafeNormal2D())<.35f)return false;
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRDroneSight),false,this);
 return !GetWorld()->LineTraceSingleByChannel(Hit,D.Position,Eye,ECC_Visibility,Q)||Hit.GetActor()==C;
}
void ANRDroneDirector::ReportNoise(FVector Position,float Radius,ANRCharacter* Source)
{
 if(!HasAuthority()||!Source||!Source->IsAlive()||!FMath::IsFinite(Radius)||Radius<=0||Position.ContainsNaN())return;
 auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
 for(auto H:Entities){auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(H);if(D.Health<=0||D.State==ENRDroneState::Disabled||FVector::DistSquared(D.Position,Position)>Radius*Radius)continue;
  if(D.State==ENRDroneState::Patrol||D.State==ENRDroneState::Search){D.State=ENRDroneState::Search;D.LastKnown=Position;D.LastSeen=GetWorld()->GetTimeSeconds();D.Suspicion=FMath::Max(D.Suspicion,.3f);}
 }
}
void ANRDroneDirector::DisableSecurity(float Seconds)
{
 if(!HasAuthority()||!FMath::IsFinite(Seconds)||Seconds<=0)return;BlackoutUntil=GetWorld()->GetTimeSeconds()+FMath::Min(Seconds,30.f);
 auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();for(int I=0;I<Entities.Num();++I){auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(Entities[I]);if(D.Health<=0)continue;D.State=ENRDroneState::Disabled;D.DisabledUntil=BlackoutUntil;D.bHasPath=false;D.Tracked.Reset();D.Suspicion=0;Publish(I,D);}AlertCount=0;OnRep_Views();ForceNetUpdate();
}
void ANRDroneDirector::Sense(int32 I,FNRDroneFragment& D,const TArray<ANRCharacter*>& Targets,float Now)
{
 ANRCharacter* Seen=nullptr;float Nearest=FLT_MAX;
 for(auto* C:Targets)
 {
  const float Distance=FVector::DistSquared(D.Position,C->GetActorLocation());
  if(Distance<Nearest&&CanSee(D,C)){Seen=C;Nearest=Distance;}
  else if(D.State==ENRDroneState::Patrol&&Distance<FMath::Square(HearingRadius(C))){D.State=ENRDroneState::Search;D.LastKnown=C->GetActorLocation();D.LastSeen=Now;D.Suspicion=.25f;}
 }
 if(Seen)
 {
  D.Tracked=Seen;D.LastKnown=Seen->GetActorLocation();D.LastSeen=Now;D.Suspicion=FMath::Min(1.f,D.Suspicion+(Seen->IsQuietWalking()?.22f:.4f));
  if(D.State!=ENRDroneState::Charging)D.State=D.Suspicion>=.95f?ENRDroneState::Alert:ENRDroneState::Search;
  D.Yaw=(Seen->GetActorLocation()-D.Position).Rotation().Yaw;
 }
 else if(Now-D.LastSeen>1.3f&&D.State!=ENRDroneState::Charging)
 {
  D.Tracked.Reset();D.Suspicion=FMath::Max(0.f,D.Suspicion-.12f);D.State=Now-D.LastSeen>6?ENRDroneState::Patrol:ENRDroneState::Search;
 }
 if(D.State==ENRDroneState::Charging)return;
 const FVector Goal=D.State==ENRDroneState::Patrol?D.Home+FVector(FMath::Sin(Now*.2f+I)*220,FMath::Cos(Now*.2f+I)*160,0):D.LastKnown;
 D.Speed=MoveSpeed(D.Kind)*(D.State==ENRDroneState::Patrol?.55f:1.f);
 D.bHasPath=false;
 // Four navigation queries per 100 ms, never one query for every entity per frame.
 auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),D.Position-FVector(0,0,70),Goal,this);
 if(Path&&Path->IsValid()&&Path->PathPoints.Num()>1){D.Target=Path->PathPoints[1]+FVector(0,0,82);D.bHasPath=true;}
 if(Seen&&D.State==ENRDroneState::Alert&&Nearest<FMath::Square(D.Kind==2?1500.f:1250.f)&&Now>=D.NextAttack&&Now>=NextSquadAttack)
 {
  D.State=ENRDroneState::Charging;D.AimPoint=Seen->GetActorLocation()+FVector(0,0,30);D.FireAt=Now+(D.Kind==2?1.25f:.9f);D.bHasPath=false;NextSquadAttack=Now+.65f;
 }
}
void ANRDroneDirector::UpdateWave()
{
 auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS)return;auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
 if(GS->Phase!=ENRPhase::Combat){for(int I=0;I<Entities.Num();++I){auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(Entities[I]);D.bHasPath=false;if(D.State==ENRDroneState::Charging)D.State=ENRDroneState::Search;Publish(I,D);}AlertCount=0;OnRep_Views();return;}
 TArray<ANRCharacter*> Targets;for(TActorIterator<ANRCharacter> It(GetWorld());It;++It)if(It->IsAlive())Targets.Add(*It);
 const float Now=GetWorld()->GetTimeSeconds();AlertCount=0;
 for(int I=0;I<Entities.Num();++I)
 {
  auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(Entities[I]);if(D.Health<=0){Publish(I,D);continue;}
  if(D.State==ENRDroneState::Disabled){if(Now<D.DisabledUntil){Publish(I,D);continue;}D.State=ENRDroneState::Patrol;D.LastSeen=-100;}
  if(D.State==ENRDroneState::Charging&&Now>=D.FireAt)
  {
   // Aim is fixed during wind-up: strafing or returning to cover genuinely avoids the shot.
   FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRDroneFire),false,this);FVector End=D.AimPoint;
   if(GetWorld()->LineTraceSingleByChannel(Hit,D.Position,D.AimPoint+(D.AimPoint-D.Position).GetSafeNormal()*50,ECC_GameTraceChannel1,Q))
   {End=Hit.ImpactPoint;if(auto* Victim=Cast<ANRCharacter>(Hit.GetActor()))if(Victim->IsAlive())UGameplayStatics::ApplyPointDamage(Victim,D.Kind==2?12.f:D.Kind==1?6.f:8.f,(End-D.Position).GetSafeNormal(),Hit,nullptr,this,nullptr);}
   MulticastAttack(D.Position,End);D.State=ENRDroneState::Alert;D.NextAttack=Now+(D.Kind==2?4.f:2.8f);
  }
  if(I%6==QueryCursor)Sense(I,D,Targets,Now);
  if(D.State==ENRDroneState::Charging||D.State==ENRDroneState::Alert)++AlertCount;
  Publish(I,D);
 }
 QueryCursor=(QueryCursor+1)%6;OnRep_Views();
}
void ANRDroneDirector::Publish(int I,const FNRDroneFragment& D)
{auto& V=Views[I];V.Position=D.Position;V.AimPoint=D.State==ENRDroneState::Charging?D.AimPoint:FVector::ZeroVector;V.Kind=D.Kind;V.State=D.Health>0?D.State:ENRDroneState::Destroyed;V.Health=uint8(FMath::Clamp(D.Health/MaxHealth(D.Kind)*100,0.f,100.f));V.Suspicion=uint8(D.Suspicion*100);V.Yaw=int16(FMath::UnwindDegrees(D.Yaw)*100);V.bAlive=D.Health>0;}
bool ANRDroneDirector::HitDrone(int I,float Damage,ANRCharacter* Shooter)
{
 if(!HasAuthority()||!Entities.IsValidIndex(I)||!FMath::IsFinite(Damage)||Damage<=0)return false;
 auto& M=GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();auto& D=M.GetFragmentDataChecked<FNRDroneFragment>(Entities[I]);if(D.Health<=0)return false;
 D.Health=FMath::Max(0.f,D.Health-Damage);if(D.State!=ENRDroneState::Disabled){D.Suspicion=1;D.State=ENRDroneState::Search;if(Shooter){D.LastKnown=Shooter->GetActorLocation();D.LastSeen=GetWorld()->GetTimeSeconds();}}
 const bool Killed=D.Health<=0;if(Killed){D.bHasPath=false;D.State=ENRDroneState::Destroyed;MulticastDestroyed(D.Position);}
 if(Shooter){const int Reward=Killed?(D.Kind==2?45:30):0;if(Reward){Shooter->GiveCredits(Reward);++Shooter->DroneKills;}Shooter->ClientDroneFeedback(D.Kind,Killed,Reward);}
 Publish(I,D);OnRep_Views();ForceNetUpdate();return true;
}
void ANRDroneDirector::OnRep_Views()
{
 if(Instances->GetInstanceCount()!=Views.Num()){Instances->ClearInstances();for(const auto& V:Views)Instances->AddInstance(FTransform(FRotator(0,V.Yaw/100.f,0),V.Position,FVector(V.bAlive?1.f:0.f)),true);}
 else for(int I=0;I<Views.Num();++I)Instances->UpdateInstanceTransform(I,FTransform(FRotator(0,Views[I].Yaw/100.f,0),Views[I].Position,FVector(Views[I].bAlive?1.f:0.f)),true,I==Views.Num()-1,true);
 if(GetNetMode()==NM_DedicatedServer)return;
 if(PreviousStates.Num()!=Views.Num())PreviousStates.Init(ENRDroneState::Patrol,Views.Num());
 for(int I=0;I<Views.Num();++I){if(Views[I].State==ENRDroneState::Charging&&PreviousStates[I]!=Views[I].State)if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_DroneCharge"),Views[I].Position,.3f);PreviousStates[I]=Views[I].State;}
}
void ANRDroneDirector::MulticastAttack_Implementation(FVector_NetQuantize Origin,FVector_NetQuantize End)
{if(GetNetMode()==NM_DedicatedServer)return;if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>()){FX->Shot(Origin,(End-Origin).GetSafeNormal(),22000,this,3,true);FX->Sound(TEXT("S_DroneShot"),Origin,.45f);FX->Impact(End,FVector::UpVector,false);}}
void ANRDroneDirector::MulticastDestroyed_Implementation(FVector_NetQuantize Position)
{if(GetNetMode()==NM_DedicatedServer)return;if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>()){FX->Impact(Position,FVector::UpVector,false);FX->Sound(TEXT("S_DroneDown"),Position,.65f);}if(auto* Debris=GetWorld()->GetSubsystem<UNRDebrisSubsystem>())Debris->SpawnBurst(Position,17);}
void ANRDroneDirector::Tick(float Dt)
{
 Super::Tick(Dt);if(Bodies.Num()!=3||!Rotors||!WarningLines)return;
 if(RenderPositions.Num()!=Views.Num())
 {
  RenderPositions.Reset();for(const auto& V:Bodies)V->ClearInstances();Rotors->ClearInstances();WarningLines->ClearInstances();
  for(const auto& V:Views){RenderPositions.Add(V.Position);for(const auto& B:Bodies)B->AddInstance(FTransform::Identity);Rotors->AddInstance(FTransform::Identity);Rotors->AddInstance(FTransform::Identity);WarningLines->AddInstance(FTransform::Identity);}
 }
 const float Time=GetWorld()->GetTimeSeconds();
 for(int I=0;I<Views.Num();++I)
 {
  const auto& V=Views[I];RenderPositions[I]=FMath::VInterpTo(RenderPositions[I],V.Position,Dt,14);
  const FRotator Rotation(0,V.Yaw/100.f,0);const FVector Position=RenderPositions[I]+FVector(0,0,FMath::Sin(Time*3+I)*1.4);
  for(int K=0;K<3;++K)Bodies[K]->UpdateInstanceTransform(I,FTransform(Rotation,Position,FVector(V.bAlive&&V.Kind==K?1.f:0.f)),true,I==Views.Num()-1,true);
  for(int Side=0;Side<2;++Side){const FVector P=Position+Rotation.RotateVector(FVector(-5,Side==0?-41:41,10));Rotors->UpdateInstanceTransform(I*2+Side,FTransform(FRotator(0,Time*(V.State==ENRDroneState::Disabled?0:Side==0?1600:-1600),0),P,FVector(V.bAlive?1.f:0.f)),true,I==Views.Num()-1&&Side==1,true);}
  const FVector Beam=FVector(V.AimPoint)-Position;WarningLines->UpdateInstanceTransform(I,FTransform(Beam.Rotation(),Position+Beam*.5,FVector(Beam.Size()/100,.007,.007)*(V.State==ENRDroneState::Charging?1.f:0.f)),true,I==Views.Num()-1,true);
 }
}
