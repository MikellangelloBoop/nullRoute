#include "NRBallisticsSubsystem.h"
#include "NRGameMode.h"
#include "NRPreparationZone.h"
#include "NRCharacter.h"
#include "NRStructure.h"
#include "NRNode.h"
#include "NRActivity.h"
#include "NRDroneDirector.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NRAlgorithms.h"
bool UNRBallisticsSubsystem::ShouldCreateSubsystem(UObject* O)const{auto* W=Cast<UWorld>(O);return W&&W->IsGameWorld();}
void UNRBallisticsSubsystem::Fire(ANRCharacter* C,FVector O,FVector D,float Damage,float Speed){if(!C||!C->HasAuthority()||!FMath::IsFinite(Damage)||!FMath::IsFinite(Speed)||Damage<=0||Speed<=0||Bullets.Num()>=512||!NR::IsFiniteVector(O)||!NR::IsFiniteVector(D))return;const auto* GS=GetWorld()->GetGameState<ANRGameState>();if(!GS||(GS->Phase!=ENRPhase::Preparation&&GS->Phase!=ENRPhase::Combat))return;FNRBullet B;B.bPractice=GS->Phase==ENRPhase::Preparation;B.Shooter=C;B.Position=O;B.Velocity=D.GetSafeNormal()*Speed;B.Damage=Damage;Bullets.Add(B);}
void UNRBallisticsSubsystem::Tick(float Dt){if(GetWorld()->GetNetMode()==NM_Client)return;const float SimDt=FMath::Clamp(Dt,0.f,.1f);const int32 Steps=FMath::Max(1,FMath::CeilToInt(SimDt/.004f));const float H=SimDt/Steps;
 for(int32 I=Bullets.Num()-1;I>=0;--I){auto& B=Bullets[I];auto* Shooter=B.Shooter.Get();const auto* GS=GetWorld()->GetGameState<ANRGameState>();bool Remove=!Shooter||!GS||(B.bPractice?GS->Phase!=ENRPhase::Preparation:GS->Phase!=ENRPhase::Combat);B.Life+=SimDt;
 for(int32 S=0;S<Steps&&!Remove;++S){const FVector Start=B.Position;B.Velocity.Z-=980.f*H;const FVector End=Start+B.Velocity*H;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRBallisticStep),true,Shooter);if(B.LastSurface.IsValid())Q.AddIgnoredActor(B.LastSurface.Get());FHitResult Hit;
 if(!GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_GameTraceChannel1,Q)){B.Position=End;continue;}auto* Actor=Hit.GetActor();const FVector Dir=B.Velocity.GetSafeNormal();Shooter->MulticastImpact(Hit.ImpactPoint,Hit.ImpactNormal,Cast<ANRCharacter>(Actor)!=nullptr||Cast<ANRDroneDirector>(Actor)!=nullptr);
 // The practice bit is stamped by the server on fire, never supplied by a client.
 if(B.bPractice){if(auto* Target=Cast<ANRPreparationProp>(Actor))UGameplayStatics::ApplyPointDamage(Target,B.Damage,Dir,Hit,Shooter->GetController(),Shooter,nullptr);Remove=true;}
 else if(auto* Character=Cast<ANRCharacter>(Actor)){if(Character->Team!=Shooter->Team){UGameplayStatics::ApplyPointDamage(Character,B.Damage,Dir,Hit,Shooter->GetController(),Shooter,nullptr);Shooter->ClientHitFeedback(!Character->IsAlive());}Remove=true;}
 else if(auto* Drones=Cast<ANRDroneDirector>(Actor)){Drones->HitDrone(Hit.Item,B.Damage,Shooter);Remove=true;}
 else if(Actor&&Actor->ActorHasTag(TEXT("SurfaceMetal"))&&B.Damage>8&&NR::MetalRicochet(B.Velocity,Hit.ImpactNormal,B.Bounces,B.Velocity)){
 ++B.Bounces;B.Damage*=.45f;B.Position=Hit.ImpactPoint+Hit.ImpactNormal*2.f;B.LastSurface=Actor;
 Shooter->MulticastRicochet(B.Position,B.Velocity.GetSafeNormal(),B.Velocity.Size());
 }
 else if(Cast<ANRActivity>(Actor)){UGameplayStatics::ApplyPointDamage(Actor,B.Damage,Dir,Hit,Shooter->GetController(),Shooter,nullptr);Remove=true;}
 else if(Cast<ANRStructure>(Actor)||Cast<ANodeBase>(Actor)){
 UGameplayStatics::ApplyPointDamage(Actor,B.Damage,Dir,Hit,Shooter->GetController(),Shooter,nullptr);
 FHitResult Exit;auto* Component=Hit.GetComponent();const float Span=Component?Component->Bounds.BoxExtent.Size()*2.f+20.f:0.f;
 if(B.Penetrations>=2||!Component||!Component->LineTraceComponent(Exit,Hit.ImpactPoint+Dir*Span,Hit.ImpactPoint+Dir*.5f,Q)){Remove=true;continue;}
 const float Thickness=FVector::Distance(Hit.ImpactPoint,Exit.ImpactPoint);const float Resistance=Cast<ANRStructure>(Actor)?.65f:.25f;
 B.Damage-=Thickness*Resistance+8.f;if(Thickness>150.f||B.Damage<5.f){Remove=true;continue;}
 ++B.Penetrations;B.LastSurface=Actor;B.Position=Exit.ImpactPoint+Dir*2.f;B.Velocity*=.7f;
 }else Remove=true;
 }
 if(Remove||B.Life>2.f)Bullets.RemoveAtSwap(I);
 }
}
