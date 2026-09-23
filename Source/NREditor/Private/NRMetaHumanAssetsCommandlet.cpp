#include "NRMetaHumanAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Editor.h"
#include "MetaHumanCharacter.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "Subsystem/MetaHumanCharacterBuild.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/Skeleton.h"
#include "SkeletalMeshAttributes.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
 FTransform BoneCS(const FReferenceSkeleton& Ref,FName Name)
 {
  int32 I=Ref.FindBoneIndex(Name);check(I!=INDEX_NONE);
  FTransform Pose=Ref.GetRefBonePose()[I];
  while((I=Ref.GetParentIndex(I))!=INDEX_NONE)Pose*=Ref.GetRefBonePose()[I];
  return Pose;
 }
 void CopyGrip(USkeletalMesh* Source,USkeletalMesh* Target)
 {
  for(const FName Name:{FName("HandGrip_R"),FName("HandGrip_L")})if(const auto* Original=Source->FindSocket(Name))
  {
   auto* Socket=DuplicateObject<USkeletalMeshSocket>(Original,Target);
   const FTransform From=BoneCS(Source->GetRefSkeleton(),Original->BoneName),To=BoneCS(Target->GetRefSkeleton(),Original->BoneName);
   FTransform Local(Original->RelativeRotation,Original->RelativeLocation,Original->RelativeScale);
   FTransform World=Local*From;World.SetLocation(To.GetLocation()+World.GetLocation()-From.GetLocation());
   Local=World*To.Inverse();Socket->RelativeLocation=Local.GetLocation();Socket->RelativeRotation=Local.Rotator();
   Target->GetMeshOnlySocketList().Add(Socket);
  }
 }
}
UNRMetaHumanAssetsCommandlet::UNRMetaHumanAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRMetaHumanAssetsCommandlet::Main(const FString&)
{
 auto* Character=LoadObject<UMetaHumanCharacter>(nullptr,TEXT("/Game/MetaHumans/Source/Breacher"));
 auto* Sub=GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
 auto* Original=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Meshes/SKM_OperatorBody"));
 if(!Character||!Sub||!Original||!Sub->TryAddObjectToEdit(Character))return 1;
 // The body has a local DNA rig in Creator. Export that supported editing mesh
 // for the sealed helmet character even when the optional facial cloud rig is unavailable.
 // This does not claim that the cloud-dependent UE Optimized face assembly succeeded.
 auto Constraints=Sub->GetBodyConstraints(Character);
 TMap<FString,float> Shape{{TEXT("Height"),185.f},{TEXT("Chest"),110.f},{TEXT("Waist"),88.f},{TEXT("Across Shoulder"),44.f},{TEXT("Fat"),.16f},{TEXT("Muscularity"),.28f}};
 for(auto& C:Constraints)if(const float* Value=Shape.Find(C.Name.ToString())){C.bIsActive=true;C.TargetMeasurement=*Value;}
 Sub->SetBodyConstraints(Character,Constraints);Sub->CommitBodyState(Character);NRSaveAsset(Character);
 const USkeletalMesh* EditingBody=Sub->GetBodyEditMesh(Character);
 auto* Body=DuplicateObject<USkeletalMesh>(EditingBody,CreatePackage(TEXT("/Game/Art/MetaHuman/SKM_BreacherBody")),TEXT("SKM_BreacherBody"));
 Body->SetFlags(RF_Public|RF_Standalone);
 auto* Rig=DuplicateObject<USkeleton>(Body->GetSkeleton(),CreatePackage(TEXT("/Game/Art/MetaHuman/SK_Breacher")),TEXT("SK_Breacher"));
 Rig->SetFlags(RF_Public|RF_Standalone);Body->SetSkeleton(Rig);
 Body->SetPostProcessAnimBlueprint(nullptr);Body->SetPhysicsAsset(nullptr);
 Body->GetAssetUserDataArray();
 auto* Suit=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Breacher_Chronos_Cloth"));
 for(auto& Material:Body->GetMaterials())Material.MaterialInterface=Suit;
 // Keep the existing LOD chain. The most detailed body is not needed beneath armor.
 TArray<int32> LODs;for(int I=1;I<FMath::Min(Body->GetLODNum(),4);++I)LODs.Add(I);
 if(!LODs.IsEmpty())FMetaHumanCharacterEditorBuild::StripLODsFromMesh(Body,LODs);
 CopyGrip(Original,Body);
 Rig->SetPreviewMesh(Body);NRSaveAsset(Rig);
 Body->PostEditChange();if(!NRSaveAsset(Body))return 2;
 // The view model uses genuine MetaHuman fingers with the same skeleton as the
 // exported body, while keeping the existing first-person animation timing.
 auto* Hands=DuplicateObject<USkeletalMesh>(Body,CreatePackage(TEXT("/Game/Art/MetaHuman/SKM_BreacherHands")),TEXT("SKM_BreacherHands"));
 Hands->SetFlags(RF_Public|RF_Standalone);
 const auto& Ref=Hands->GetRefSkeleton();TSet<int32> HandBones;
 for(int I=0;I<Ref.GetNum();++I)for(int P=I;P!=INDEX_NONE;P=Ref.GetParentIndex(P))
  if(Ref.GetBoneName(P).ToString().StartsWith(TEXT("hand_"))){HandBones.Add(I);break;}
 for(int LOD=0;LOD<Hands->GetLODNum();++LOD)if(auto* Desc=Hands->GetMeshDescription(LOD))
 {
  FSkeletalMeshAttributes Attr(*Desc);auto Weights=Attr.GetVertexSkinWeights();TSet<FVertexID> Keep;
  for(auto V:Desc->Vertices().GetElementIDs()){float W=0;for(auto Influence:Weights.Get(V))if(HandBones.Contains(Influence.GetBoneIndex()))W+=Influence.GetWeight();if(W>.65f)Keep.Add(V);}
  TArray<FPolygonID> Remove;for(auto P:Desc->Polygons().GetElementIDs()){bool Show=false;for(auto V:Desc->GetPolygonVertexInstances(P))Show|=Keep.Contains(Desc->GetVertexInstanceVertex(V));if(!Show)Remove.Add(P);}
  Desc->DeletePolygons(Remove);FElementIDRemappings Remapping;Desc->Compact(Remapping);Hands->CommitMeshDescription(LOD);
 }
 Hands->PostEditChange();if(!NRSaveAsset(Hands))return 3;
 FString Report=TEXT("MetaHuman Creator body export; local DNA body rig. Facial UE Optimized assembly is tracked separately in Saved/MetaHumanBuild.json.\n");
 Report+=FString::Printf(TEXT("Body: %s\nBones: %d\nLODs: %d\n"),*Body->GetPathName(),Ref.GetNum(),Body->GetLODNum());
 for(int I=0;I<Ref.GetNum();++I){const auto P=BoneCS(Ref,Ref.GetBoneName(I)).GetLocation();Report+=FString::Printf(TEXT("%s,%d,%.5f,%.5f,%.5f\n"),*Ref.GetBoneName(I).ToString(),Ref.GetParentIndex(I),P.X,P.Y,P.Z);}
 FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("MetaHumanBody.txt")));
 Sub->RemoveObjectToEdit(Character);
 UE_LOG(LogTemp,Display,TEXT("NR_METAHUMAN_BODY_COMPLETE bones=%d lods=%d"),Ref.GetNum(),Body->GetLODNum());return 0;
}
