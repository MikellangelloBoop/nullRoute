#include "NRPrepareFirstPersonCommandlet.h"
#include "NRMeshMaker.h"
#include "Engine/SkeletalMesh.h"
#include "SkeletalMeshAttributes.h"
#include "MeshDescription.h"
#include "Animation/Skeleton.h"

UNRPrepareFirstPersonCommandlet::UNRPrepareFirstPersonCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UNRPrepareFirstPersonCommandlet::Main(const FString&)
{
 auto* Source=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
 if(!Source)return 1;
 // Keep the source template intact; the viewmodel owns a separate trimmed mesh.
 auto* Mesh=DuplicateObject<USkeletalMesh>(Source,CreatePackage(TEXT("/Game/Art/Meshes/SKM_OperatorArms")),TEXT("SKM_OperatorArms"));
 Mesh->SetFlags(RF_Public|RF_Standalone);
 const FReferenceSkeleton& Ref=Mesh->GetRefSkeleton();TSet<int32> Arms;
 for(int32 Bone=0;Bone<Ref.GetNum();++Bone)
  for(int32 Parent=Bone;Parent!=INDEX_NONE;Parent=Ref.GetParentIndex(Parent))
   if(Ref.GetBoneName(Parent).ToString().StartsWith(TEXT("hand_"))){Arms.Add(Bone);break;}
 for(int32 LOD=0;LOD<Mesh->GetLODNum();++LOD)
 {
  auto* Desc=Mesh->GetMeshDescription(LOD);if(!Desc)continue;
  FSkeletalMeshAttributes Attr(*Desc);auto Weights=Attr.GetVertexSkinWeights();TSet<FVertexID> Keep;
  for(FVertexID Vertex:Desc->Vertices().GetElementIDs())
  {
   float Weight=0;for(const auto W:Weights.Get(Vertex))if(Arms.Contains(W.GetBoneIndex()))Weight+=W.GetWeight();
   if(Weight>.85f)Keep.Add(Vertex);
  }
  TArray<FPolygonID> Remove;
  for(FPolygonID Polygon:Desc->Polygons().GetElementIDs())
  {
   bool bArm=false;for(FVertexInstanceID Vertex:Desc->GetPolygonVertexInstances(Polygon))bArm|=Keep.Contains(Desc->GetVertexInstanceVertex(Vertex));
   if(!bArm)Remove.Add(Polygon);
  }
  Desc->DeletePolygons(Remove);FElementIDRemappings Remapping;Desc->Compact(Remapping);
  Mesh->CommitMeshDescription(LOD);
  UE_LOG(LogTemp,Display,TEXT("NR_ARMS LOD %d: removed %d body polygons, retained %d arm polygons"),LOD,Remove.Num(),Desc->Polygons().Num());
 }
 Mesh->SetSkeleton(LoadObject<USkeleton>(nullptr,TEXT("/Game/Art/Animations/SK_FirstPerson.SK_FirstPerson")));
 Mesh->PostEditChange();if(!NRSaveAsset(Mesh))return 2;
 UE_LOG(LogTemp,Display,TEXT("NR_ARMS_COMPLETE"));return 0;
}
