#include "NRStructure.h"
#include "NRCollapseVolume.h"
#include "NRInterestSubsystem.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "Field/FieldSystemObjects.h"
#include "NavigationSystem.h"
#include "Navigation/NavLinkProxy.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "NRAlgorithms.h"
#include "NRDebrisSubsystem.h"

ANRStructure::ANRStructure(const FObjectInitializer& Init):Super(Init){
 bReplicates=true; bReplicateUsingRegisteredSubObjectList=true;NetDormancy=DORM_Initial;
 SetReplicateMovement(false);PrimaryActorTick.bCanEverTick=false;NetCullDistanceSquared=400000000.f;
 auto* GC=GetGeometryCollectionComponent();GC->SetIsReplicated(true);GC->SetEnableReplication(true);
 GC->MaxSimulatedLevel=1;GC->MaxClusterLevel=1;GC->SetReplicationAbandonAfterLevel(1);
 GC->SetEnableDamageFromCollision(false);GC->ObjectType=EObjectStateTypeEnum::Chaos_Object_Kinematic;
 GC->SetCanEverAffectNavigation(true);
}
void ANRStructure::BeginPlay(){Super::BeginPlay();if(auto* Interest=GetWorld()->GetSubsystem<UNRInterestSubsystem>())Interest->RegisterActor(this);if(HasAuthority()&&!IsNetStartupActor())SetNetDormancy(DORM_Awake);}
void ANRStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRStructure,bCollapsed);}
void ANRStructure::WakeForMutation(){if(HasAuthority())SetNetDormancy(DORM_Awake);}
void ANRStructure::FinishMutation(){if(HasAuthority()){ForceNetUpdate();SetNetDormancy(DORM_DormantAll);}}
bool ANRStructure::BreachServer(FVector Origin,FVector Direction,float Strain){
 if(!HasAuthority()||bCollapsed||!NR::IsFiniteVector(Origin)||!NR::IsFiniteVector(Direction))return false;
 Direction=Direction.GetSafeNormal();if(Direction.IsNearlyZero())return false;
 WakeForMutation();bCollapsed=true;auto* GC=GetGeometryCollectionComponent();
 // An oriented, finite box restricts the field to the forward breach volume.
 auto* Mask=NewObject<UBoxFalloff>(this);FTransform Box(Direction.Rotation(),Origin+Direction*140.f,FVector(4.f,3.f,3.f));
 Mask->SetBoxFalloff(1.f,0.f,1.f,0.f,Box,Field_FallOff_None);
 auto* Dynamic=NewObject<UUniformInteger>(this);Dynamic->SetUniformInteger(int32(EObjectStateTypeEnum::Chaos_Object_Dynamic));
 GC->ApplyPhysicsField(true,EGeometryCollectionPhysicsTypeEnum::Chaos_DynamicState,nullptr,Dynamic);
 auto* StrainField=NewObject<UOperatorField>(this);auto* Magnitude=NewObject<UUniformScalar>(this);Magnitude->SetUniformScalar(Strain);
 StrainField->SetOperatorField(1.f,Mask,Magnitude,Field_Multiply);
 GC->ApplyPhysicsField(true,EGeometryCollectionPhysicsTypeEnum::Chaos_ExternalClusterStrain,nullptr,StrainField);
 auto* Velocity=NewObject<UUniformVector>(this);Velocity->SetUniformVector(900.f,Direction);
 auto* Directional=NewObject<UOperatorField>(this);Directional->SetOperatorField(1.f,Mask,Velocity,Field_Multiply);
 GC->ApplyPhysicsField(true,EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity,nullptr,Directional);
 OnRep_Collapsed();MulticastFragments(Origin,Direction,GetUniqueID());ForceNetUpdate();
 // Keep the moving L1 collection awake during initial replication.
 GetWorldTimerManager().SetTimer(SettleTimer,this,&ANRStructure::FinishMutation,8.f,false);
 return true;
}
float ANRStructure::TakeDamage(float Amount,FDamageEvent const& Event,AController* EventInstigator,AActor* Causer){
 if(!HasAuthority()||bCollapsed||!FMath::IsFinite(Amount)||Amount<=0)return 0;
 Integrity-=Amount;if(Integrity<=0){const FVector D=Causer?(GetActorLocation()-Causer->GetActorLocation()).GetSafeNormal():FVector::UpVector;BreachServer(GetActorLocation()-D*80,D);}return Amount;
}
void ANRStructure::OnRep_Collapsed(){
 if(!bCollapsed){auto* GC=GetGeometryCollectionComponent();GC->SetRestCollection(GC->GetRestCollection(),false);GC->SetCanEverAffectNavigation(true);return;}
 // Large debris keeps Chaos collision, but is not exported as a walkable floor.
 GetGeometryCollectionComponent()->SetCanEverAffectNavigation(false);
 if(HasAuthority())UpdateNavigation();
}
void ANRStructure::UpdateNavigation(){
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());if(!Nav)return;
 const FBox Bounds=GetComponentsBoundingBox(true);Nav->AddDirtyArea(Bounds,ENavigationDirtyFlag::All);
 if(bFloor){
  if(auto* Modifier=GetWorld()->SpawnActor<ANRCollapseVolume>()){const FVector Center=Bounds.GetCenter();FBox Hole(Center-FVector(Bounds.GetExtent().X,Bounds.GetExtent().Y,25),Center+FVector(Bounds.GetExtent().X,Bounds.GetExtent().Y,25));Modifier->SetHole(Hole);}

  auto* Link=GetWorld()->SpawnActor<ANavLinkProxy>(GetActorLocation(),FRotator::ZeroRotator);
  if(Link){Link->PointLinks.Reset();FNavigationLink L;L.Left=FVector(-180,0,60);L.Right=FVector(180,0,-300);L.Direction=ENavLinkDirection::LeftToRight;Link->PointLinks.Add(L);Link->SetSmartLinkEnabled(true);}
 }
}

void ANRStructure::ResetForRound(){if(!HasAuthority())return;GetWorldTimerManager().ClearTimer(SettleTimer);Integrity=180.f;if(bCollapsed){WakeForMutation();bCollapsed=false;OnRep_Collapsed();FinishMutation();}}

void ANRStructure::MulticastFragments_Implementation(FVector_NetQuantize Origin,FVector_NetQuantizeNormal Direction,uint32 Seed)
{if(GetNetMode()!=NM_DedicatedServer)if(auto* D=GetWorld()->GetSubsystem<UNRDebrisSubsystem>())D->SpawnBurst(Origin,Seed,Direction);}
