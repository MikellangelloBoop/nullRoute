#include "NRCharacter.h"
#include "Engine/World.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

void ANRCharacter::PresentDeath()
{
 if(GetNetMode()==NM_DedicatedServer||!bVisualsReady)return;
 auto* Clip=Cast<UAnimSequence>(AnimationForBody(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01"))));
 if(!Clip)return;
 // Local, non-colliding corpse; no server ragdoll, replicated bones or authoritative hitbox.
 auto* Corpse=GetWorld()->SpawnActor<AActor>();Corpse->SetReplicates(false);Corpse->SetLifeSpan(6);
 auto* Skin=NewObject<USkeletalMeshComponent>(Corpse);Corpse->SetRootComponent(Skin);
 Skin->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());Skin->SetCollisionEnabled(ECollisionEnabled::NoCollision);Skin->SetCanEverAffectNavigation(false);
 Skin->SetCastShadow(true);Skin->SetComponentTickEnabled(true);Skin->RegisterComponent();Skin->SetWorldTransform(GetMesh()->GetComponentTransform());
 for(int I=0;I<GetMesh()->GetNumMaterials();++I)Skin->SetMaterial(I,GetMesh()->GetMaterial(I));
 Skin->PlayAnimation(Clip,false);
 auto Copy=[&](UStaticMeshComponent* Source)
 {
  if(!Source||!Source->GetStaticMesh()||!Source->IsVisible())return;
  auto* Part=NewObject<UStaticMeshComponent>(Corpse);Part->SetupAttachment(Skin,Source->GetAttachSocketName());Part->SetStaticMesh(Source->GetStaticMesh());
  Part->SetRelativeTransform(Source->GetRelativeTransform());Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);Part->SetCastShadow(true);
  for(int I=0;I<Source->GetNumMaterials();++I)Part->SetMaterial(I,Source->GetMaterial(I));Part->RegisterComponent();
 };
 for(auto& Part:ArmorParts)Copy(Part.Get());Copy(WorldWeapon);
}
