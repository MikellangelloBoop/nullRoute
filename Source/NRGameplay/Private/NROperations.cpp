#include "NRCharacter.h"
#include "NRHUD.h"
#include "NRDroneDirector.h"
#include "NRCombatFX.h"
#include "NRGameMode.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

void ANRCharacter::ReportWeaponNoise(bool Suppressed)
{if(!HasAuthority())return;for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)It->ReportNoise(GetActorLocation(),Suppressed?700.f:2400.f,this);}
void ANRCharacter::ClientDroneFeedback_Implementation(uint8 Kind,bool Killed,int32 Reward)
{
 ClientHitFeedback_Implementation(Killed);
 if(Killed){CombatNotice=FString::Printf(TEXT("%s УНИЧТОЖЕН  /  +%d CR"),ANRDroneDirector::KindName(Kind),Reward);CombatNoticeUntil=GetWorld()->GetTimeSeconds()+2.5f;}
}
void ANRCharacter::PingPressed(){if(IsAlive()&&!IsUIBlocking())ServerPing();}
void ANRCharacter::ServerPing_Implementation()
{
 const float Now=GetWorld()->GetTimeSeconds();if(!IsAlive()||Now-LastPing<1.25f)return;LastPing=Now;
 FHitResult H;FVector Eye;FRotator Aim;GetActorEyesViewPoint(Eye,Aim);
 const bool Hit=TraceView(H,5000);const FVector Point=Hit?H.ImpactPoint:Eye+Aim.Vector()*2500;
 const auto* Target=Cast<ANRCharacter>(H.GetActor());const bool Danger=Cast<ANRDroneDirector>(H.GetActor())||(Target&&Target->Team!=Team);
 // Server derives the point from the validated eye. Enemy clients receive no team marker RPC.
 for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
  if(auto* PC=It->Get())if(auto* Teammate=Cast<ANRCharacter>(PC->GetPawn()))if(Teammate->Team==Team)Teammate->ClientReceivePing(Point,Danger);
}
void ANRCharacter::ClientReceivePing_Implementation(FVector_NetQuantize Position,bool Danger)
{TacticalPing=Position;bDangerPing=Danger;PingUntil=GetWorld()->GetTimeSeconds()+8;if(auto* FX=GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_TacticalPing"),GetActorLocation(),.35f,true);}
void ANRHUD::RefreshOperations()
{
 auto* C=Cast<ANRCharacter>(GetOwningPawn());auto* PC=GetOwningPlayerController();if(!C||!PC)return;
 TargetDrone.Reset();TargetHealth=0;Threat=0;AlertDrones=0;BlackoutSeconds=0;
 const auto* GS=GetWorld()->GetGameState<ANRGameState>();const float Now=GS?GS->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
 FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRVisorContact),false,C);FVector Eye;FRotator Aim;PC->GetPlayerViewPoint(Eye,Aim);
 GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+Aim.Vector()*2500,ECC_Visibility,Q);
 if(const auto* D=Cast<ANRDroneDirector>(H.GetActor()))if(D->GetViews().IsValidIndex(H.Item))
 {const auto& V=D->GetViews()[H.Item];if(V.bAlive){TargetDrone=ANRDroneDirector::KindName(V.Kind);TargetHealth=V.Health/100.f;}}
 for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)
 {
  AlertDrones+=It->AlertCount;BlackoutSeconds=FMath::Max(BlackoutSeconds,It->BlackoutUntil-Now);
  // Visible, nearby contacts only. No player or drone silhouettes are exposed through walls.
  for(int I=0;I<It->GetViews().Num();++I){const auto& V=It->GetViews()[I];if(V.bAlive&&V.Suspicion>Threat*100&&FVector::DistSquared(V.Position,Eye)<1800.f*1800.f)
  {FHitResult Sight;if(!GetWorld()->LineTraceSingleByChannel(Sight,Eye,V.Position,ECC_Visibility,Q)||(Sight.GetActor()==*It&&Sight.Item==I))Threat=V.Suspicion/100.f;}}
 }
 BlackoutSeconds=FMath::Max(0.f,BlackoutSeconds);
 if(C->PingUntil>GetWorld()->GetTimeSeconds())
 {
  FVector2D Screen;int W=1600,Ht=900;PC->GetViewportSize(W,Ht);
  if(PC->ProjectWorldLocationToScreen(C->TacticalPing,Screen)){Screen.X*=1600.f/FMath::Max(W,1);Screen.Y*=900.f/FMath::Max(Ht,1);}
  else {const float Yaw=FMath::FindDeltaAngleDegrees(Aim.Yaw,(C->TacticalPing-Eye).Rotation().Yaw);Screen=FVector2D(Yaw<0?360:1240,450);}
  PingScreen=FVector2D(FMath::Clamp(Screen.X,360.f,1240.f),FMath::Clamp(Screen.Y,280.f,630.f));PingMeters=FVector::Dist(Eye,C->TacticalPing)/100;
 }
}
