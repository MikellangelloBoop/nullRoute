#include "NRCharacter.h"
#include "NRLocomotion.h"
#include "NRGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Materials/MaterialInterface.h"

ENRFaction ANRCharacter::GetFaction()const
{
 if(const auto* GS=GetWorld()->GetGameState<ANRGameState>())return GS->FactionForTeam(Team);
 return Team==1?ENRFaction::Rebel:ENRFaction::Chronos;
}
UAnimationAsset* ANRCharacter::AnimationForBody(UAnimationAsset* Source)const
{
 if(!bUsingMetaHuman||!Source)return Source;
 auto* Animation=LoadObject<UAnimationAsset>(nullptr,*(TEXT("/Game/Art/MetaHuman/Animations/MH_")+Source->GetName()));
 ensureMsgf(Animation,TEXT("Missing MetaHuman animation for %s"),*Source->GetPathName());
 return Animation;
}
void ANRCharacter::RefreshAppearance()
{
 if(!bVisualsReady||GetNetMode()==NM_DedicatedServer)return;
 const ENRFaction Faction=GetFaction();const bool Assault=OperatorRole==ENRRole::Assault;const bool ClassRifle=Assault&&PrimaryKind()==ENRPrimaryWeapon::BR74;
 if(!bUsingMetaHuman)
 {
  auto* Model=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/MetaHuman/SKM_BreacherBody"));
  auto* Hands=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/MetaHuman/SKM_BreacherHands"));
  if(Model&&Hands)
  {
   bUsingMetaHuman=true;GetMesh()->SetSkeletalMesh(Model);ViewArms->SetSkeletalMesh(Hands);
   GetMesh()->EmptyOverrideMaterials();ViewArms->EmptyOverrideMaterials();WorldWeaponSlot=255;
   GetMesh()->PlayAnimation(AnimationForBody(LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/Art/Animations/BS_Operator2D"))),true);
   GetMesh()->TickAnimation(0,false);GetMesh()->RefreshBoneTransforms();PlayReadyAnimation();
  }
 }
 const TCHAR* Key=NRFactions::Key(Faction);
 const TCHAR* RoleKeys[]={TEXT("Engineer"),TEXT("Scout"),TEXT("Breacher"),TEXT("Medic")};const TCHAR* RoleKey=RoleKeys[FMath::Min(3,int(OperatorRole))];
 static const TCHAR* Bones[]={TEXT("head"),TEXT("spine_04"),TEXT("pelvis"),TEXT("upperarm_l"),TEXT("upperarm_r"),TEXT("lowerarm_l"),TEXT("lowerarm_r"),TEXT("thigh_l"),TEXT("thigh_r"),TEXT("calf_l"),TEXT("calf_r"),TEXT("foot_l"),TEXT("foot_r"),TEXT("hand_l"),TEXT("hand_r"),TEXT("Badge")};
 if(ArmorParts.IsEmpty())for(const TCHAR* Bone:Bones)
 {
  auto* Part=NewObject<UStaticMeshComponent>(this);Part->SetupAttachment(GetMesh(),FString(Bone)==TEXT("Badge")?FName(TEXT("spine_04")):FName(Bone));
  Part->ComponentTags.Add(TEXT("BreacherArmor"));Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);
  Part->SetOwnerNoSee(true);Part->SetCastShadow(true);Part->SetCastHiddenShadow(true);Part->RegisterComponent();ArmorParts.Add(Part);
 }
 for(int I=0;I<ArmorParts.Num();++I)
 {
  auto* Part=ArmorParts[I].Get();Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Meshes/SM_MH_%s_%s_%s"),RoleKey,Key,Bones[I])));
  const bool Show=!bDead&&I!=15;
  Part->SetVisibility(Show);Part->SetCastShadow(Show);Part->SetCastHiddenShadow(Show);
 }
 // The skinned MetaHuman body now supplies the continuous undersuit and articulated
 // hands. Armor covers it without replacing joints with rigid primitive shapes.
 const bool ShowBody=!bDead&&bUsingMetaHuman;
 GetMesh()->SetVisibility(ShowBody,false);GetMesh()->SetCastShadow(ShowBody);GetMesh()->SetCastHiddenShadow(ShowBody);
 GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
 if(!Cast<UNRLocomotion>(GetMesh()->GetAnimInstance()))GetMesh()->SetAnimInstanceClass(UNRLocomotion::StaticClass());
 TArray<UStaticMeshComponent*> Parts;GetComponents(Parts);
 for(auto* Part:Parts)
 {
  if(Part->ComponentHasTag(TEXT("LegacyArmor"))){const bool Show=false;Part->SetVisibility(Show);Part->SetCastShadow(Show);Part->SetCastHiddenShadow(Show);}
  if(Part->ComponentHasTag(TEXT("RifleDetail"))){const bool Show=!bDead&&EquippedWeapon==0&&!ClassRifle&&uint8(PrimaryKind())<4;Part->SetVisibility(Show);Part->SetCastShadow(Show&&!Part->bOnlyOwnerSee);Part->SetCastHiddenShadow(false);}
  if(Part->ComponentHasTag(TEXT("FactionAccent")))Part->SetMaterial(3,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Materials/M_Breacher_%s_Glow"),Key)));
 }
 auto* UnderSuit=LoadObject<UMaterialInterface>(nullptr,bUsingMetaHuman&&uint8(Faction)<3?TEXT("/Game/Art/MetaHuman/M_Undersuit"):*FString::Printf(TEXT("/Game/Art/Materials/M_Breacher_%s_Cloth"),Key));
 for(int I=0;I<GetMesh()->GetNumMaterials();++I)GetMesh()->SetMaterial(I,UnderSuit);
 for(int I=0;I<ViewArms->GetNumMaterials();++I)ViewArms->SetMaterial(I,UnderSuit);
 for(auto& Sleeve:ViewSleeves)Sleeve->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Assault?*FString::Printf(TEXT("/Game/Art/Meshes/SM_Breacher_%s_Sleeve"),Key):*FString::Printf(TEXT("/Game/Art/Meshes/SM_MH_%s_%s_Sleeve"),RoleKey,Key)));
 for(auto& Cuff:ViewCuffs)Cuff->SetMaterial(2,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Materials/M_Breacher_%s_Trim"),Key)));
 if(ClassRifle&&EquippedWeapon==0)
 {
  auto* Model=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Meshes/SM_Breacher_%s_Weapon"),Key));
  if(Model){Weapon->SetStaticMesh(Model);WorldWeapon->SetStaticMesh(Model);Weapon->EmptyOverrideMaterials();WorldWeapon->EmptyOverrideMaterials();}
 }
 if(!ClassRifle&&EquippedWeapon==0)
 {
  Weapon->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Materials/M_Breacher_%s_Armor"),Key)));
  WorldWeapon->SetMaterial(0,Weapon->GetMaterial(0));
 }
 CachedFaction=Faction;CachedAppearanceRole=OperatorRole;CachedAppearanceTeam=Team;
}
