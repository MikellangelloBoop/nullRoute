#include "NRDebrisSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NRAlgorithms.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
bool UNRDebrisSubsystem::ShouldCreateSubsystem(UObject* Outer)const{const UWorld* W=Cast<UWorld>(Outer);return !IsRunningDedicatedServer()&&W&&W->IsGameWorld();}
void UNRDebrisSubsystem::SpawnBurst(FVector Position,uint32 Seed,FVector Direction){
 if(!NR::IsFiniteVector(Position)||!NR::IsFiniteVector(Direction))return;Direction=Direction.GetSafeNormal();
 if(!DebrisMaterial){DebrisMaterial=NewObject<UPhysicalMaterial>(this);DebrisMaterial->Friction=.82f;DebrisMaterial->Restitution=.16f;}
 if(GetWorld()->GetNetMode()==NM_DedicatedServer)return;
 if(!PoolOwner)PoolOwner=GetWorld()->SpawnActor<AActor>();
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));FRandomStream R(Seed);
 for(int32 I=0;I<24&&Pieces.Num()<128;++I){FNRLocalDebris P;P.Mesh=NewObject<UStaticMeshComponent>(PoolOwner);
 P.Mesh->SetStaticMesh(Cube);P.Mesh->SetWorldLocation(Position+R.VRand()*45.f);P.Mesh->SetWorldScale3D(FVector(R.FRandRange(.035f,.11f),R.FRandRange(.025f,.075f),R.FRandRange(.02f,.06f)));
 P.Mesh->SetWorldRotation(R.VRand().Rotation());
 P.Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Ceramic")));
 P.Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));P.Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
 // Cosmetic fragments collide with static architecture only: never players, bullets or each other.
 P.Mesh->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);P.Mesh->SetPhysMaterialOverride(DebrisMaterial);
 P.Mesh->SetLinearDamping(.65f);P.Mesh->SetAngularDamping(1.2f);P.Mesh->SetCastShadow(false);
 P.Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Ignore);P.Mesh->SetCanEverAffectNavigation(false);
 P.Mesh->RegisterComponent();P.Mesh->SetSimulatePhysics(true);P.Mesh->SetUseCCD(true);P.Mesh->SetMassOverrideInKg(NAME_None,.08f);P.Mesh->SetPhysicsLinearVelocity(Direction*R.FRandRange(200,480)+R.VRand()*170.f+FVector(0,0,100));P.Mesh->SetPhysicsAngularVelocityInDegrees(R.VRand()*R.FRandRange(100,500));Pieces.Add(P);}
}
void UNRDebrisSubsystem::Tick(float Dt){
 for(int32 I=Pieces.Num()-1;I>=0;--I){auto& P=Pieces[I];P.Age+=Dt;
 if(!IsValid(P.Mesh)||NR::UpdateDebris(P.Mesh->GetPhysicsLinearVelocity().SizeSquared(),Dt,P.Quiet)||P.Age>15.f){
 if(IsValid(P.Mesh)){P.Mesh->PutAllRigidBodiesToSleep();P.Mesh->SetSimulatePhysics(false);P.Mesh->DestroyComponent();}Pieces.RemoveAtSwap(I);}}
}
