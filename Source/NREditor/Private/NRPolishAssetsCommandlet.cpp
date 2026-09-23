#include "NRPolishAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TwoBoneIK.h"
#include "SkeletalMeshAttributes.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Animation/BlendSpace.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionMultiply.h"

static UMaterial* Finish(const TCHAR* Name,FLinearColor Color,float Metal,float Rough)
{
 auto* M=NewObject<UMaterial>(CreatePackage(*(FString(TEXT("/Game/Art/Materials/"))+Name)),Name,RF_Public|RF_Standalone);
 auto* C=NewObject<UMaterialExpressionVectorParameter>(M);C->ParameterName=TEXT("Tint");C->DefaultValue=Color;
 M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->BaseColor.Expression=C;
 M->GetEditorOnlyData()->Metallic.UseConstant=true;M->GetEditorOnlyData()->Metallic.Constant=Metal;
 M->GetEditorOnlyData()->Roughness.UseConstant=true;M->GetEditorOnlyData()->Roughness.Constant=Rough;
 bool Recompile=false;M->SetMaterialUsage(Recompile,MATUSAGE_SkeletalMesh);M->SetMaterialUsage(Recompile,MATUSAGE_InstancedStaticMeshes);
 M->PostEditChange();NRSaveAsset(M);return M;
}

UNRPolishAssetsCommandlet::UNRPolishAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRPolishAssetsCommandlet::Main(const FString&)
{

 for(const TCHAR* Name:{TEXT("M_Graphite"),TEXT("M_Ceramic")})if(auto* M=LoadObject<UMaterial>(nullptr,*(FString(TEXT("/Game/Art/Materials/"))+Name)))
 {
  for(const auto& Expression:M->GetExpressions())if(auto* P=Cast<UMaterialExpressionVectorParameter>(Expression.Get()))if(P->ParameterName==TEXT("Tint"))P->DefaultValue=FString(Name)==TEXT("M_Graphite")?FLinearColor(4,4.5,4.8):FLinearColor(.42,.5,.53);
  M->GetEditorOnlyData()->Metallic.Constant=.2f;M->GetEditorOnlyData()->Roughness.Constant=.58f;M->PostEditChange();NRSaveAsset(M);
 }
 auto* Steel=Finish(TEXT("M_WeaponSteel"),FLinearColor(.065,.082,.091),.82,.32);
 auto* Receiver=Finish(TEXT("M_WeaponReceiver"),FLinearColor(.19,.21,.18),.65,.42);
 auto* Grip=Finish(TEXT("M_WeaponGrip"),FLinearColor(.025,.032,.037),.05,.82);
 auto* Edge=Finish(TEXT("M_WeaponEdge"),FLinearColor(.32,.36,.38),.9,.27);
 auto* Accent=Finish(TEXT("M_WeaponAccent"),FLinearColor(.8,.36,.055),.35,.4);
 Finish(TEXT("M_OperatorSleeve"),FLinearColor(.035,.045,.052),.08,.76);

 auto* Rifle=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle"));
 if(!Rifle)return 1;
 const TCHAR* MeshNames[]={TEXT("SM_MX9"),TEXT("SM_Spectre"),TEXT("SM_AR7")};
 const TCHAR* MatNames[]={TEXT("M_MX9"),TEXT("M_Spectre"),TEXT("M_AR7")};
 const FLinearColor Tints[]={FLinearColor(.10,.135,.15),FLinearColor(.25,.23,.18),FLinearColor(.14,.17,.18)};
 for(int Role=0;Role<3;++Role)
 {
  // Retain the licensed source model's authored topology, UVs and normal map.
  auto* Mat=Finish(MatNames[Role],Tints[Role],.55,.43);
  auto* Albedo=NewObject<UMaterialExpressionTextureSample>(Mat);Albedo->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Weapons/Rifle/Textures/T_Rifle_BC.T_Rifle_BC"));Albedo->SamplerType=SAMPLERTYPE_Color;
  auto* Normal=NewObject<UMaterialExpressionTextureSample>(Mat);Normal->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Weapons/Rifle/Textures/T_Rifle_N.T_Rifle_N"));Normal->SamplerType=SAMPLERTYPE_Normal;
  auto* Multiply=NewObject<UMaterialExpressionMultiply>(Mat);Multiply->A.Expression=Albedo;Multiply->B.Expression=Mat->GetEditorOnlyData()->BaseColor.Expression;
  Mat->GetExpressionCollection().AddExpression(Albedo);Mat->GetExpressionCollection().AddExpression(Normal);Mat->GetExpressionCollection().AddExpression(Multiply);
  Mat->GetEditorOnlyData()->BaseColor.Expression=Multiply;Mat->GetEditorOnlyData()->Normal.Expression=Normal;Mat->PostEditChange();NRSaveAsset(Mat);
  auto* R=DuplicateObject<UStaticMesh>(Rifle,CreatePackage(*(FString(TEXT("/Game/Art/Meshes/"))+MeshNames[Role])),MeshNames[Role]);
  R->SetFlags(RF_Public|RF_Standalone);for(auto& Slot:R->GetStaticMaterials())Slot.MaterialInterface=Mat;
  R->PostEditChange();NRSaveAsset(R);
 }
 // Scout gets an actual barrel attachment, not a stretched copy of the receiver.
 FNRMeshMaker Suppressor({Steel,Grip,Edge});
 Suppressor.Cylinder({0,0,0},2.1,16,0,32,FRotator(0,0,90));
 for(int I=0;I<5;++I)Suppressor.Cylinder({0,-6.+I*3,0},2.18,.55,2,32,FRotator(0,0,90));
 Suppressor.Cylinder({0,8.1,0},1.75,.3,1,24,FRotator(0,0,90));Suppressor.Save(TEXT("SM_Suppressor"));

 // Closed cuff and sleeve meshes replace the exposed cut edge of the source body mesh.
 {
  auto* Cloth=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_OperatorSleeve.M_OperatorSleeve"));
  FNRMeshMaker Sleeve({Cloth,Grip});
  Sleeve.Sleeve(0);
  Sleeve.Save(TEXT("SM_FPSleeve"));
  FNRMeshMaker Cuff({Cloth,Grip,Steel});Cuff.Cylinder({0,0,0},3.1,4,0,32);Cuff.Cylinder({0,0,1.5},3.18,.7,1,32);Cuff.Box({3,0,0},{.7,2.8,2.2},2,.3);Cuff.Save(TEXT("SM_FPCuff"));
 }

 // Smooth character collision does not snag on visual bolts, rack handles or terminal keys.
 const TCHAR* Colliders[]={TEXT("SM_CoverCrate"),TEXT("SM_ServerRack"),TEXT("SM_Terminal"),TEXT("SM_SentinelDrone"),TEXT("SM_ExtractionPad"),TEXT("SM_ElaraCore")};
 for(const TCHAR* Name:Colliders)if(auto* M=LoadObject<UStaticMesh>(nullptr,*(FString(TEXT("/Game/Art/Meshes/"))+Name)))
 {
  M->CreateBodySetup();auto* B=M->GetBodySetup();B->AggGeom.EmptyElements();
  const auto Bounds=M->GetBounds();FKBoxElem Box;Box.Center=Bounds.Origin;Box.X=Bounds.BoxExtent.X*2;Box.Y=Bounds.BoxExtent.Y*2;Box.Z=Bounds.BoxExtent.Z*2;
  B->AggGeom.BoxElems.Add(Box);B->CollisionTraceFlag=CTF_UseSimpleAndComplex;B->InvalidatePhysicsData();B->CreatePhysicsMeshes();M->MarkPackageDirty();NRSaveAsset(M);
 }
 // Eight-way locomotion has proper strafe/backward clips rather than sliding a forward walk.
 auto* BS=NewObject<UBlendSpace>(CreatePackage(TEXT("/Game/Art/Animations/BS_Operator2D")),TEXT("BS_Operator2D"),RF_Public|RF_Standalone);
 BS->SetSkeleton(LoadObject<USkeleton>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin")));
 auto& Direction=const_cast<FBlendParameter&>(BS->GetBlendParameter(0));Direction.DisplayName=TEXT("Direction");Direction.Min=-180;Direction.Max=180;Direction.GridNum=8;
 auto& Speed=const_cast<FBlendParameter&>(BS->GetBlendParameter(1));Speed.DisplayName=TEXT("Speed");Speed.Min=0;Speed.Max=450;Speed.GridNum=2;
 const TCHAR* Dirs[]={TEXT("Bwd"),TEXT("Bwd_Left"),TEXT("Left"),TEXT("Fwd_Left"),TEXT("Fwd"),TEXT("Fwd_Right"),TEXT("Right"),TEXT("Bwd_Right"),TEXT("Bwd")};
 auto* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
 for(int I=0;I<9;++I)
 {
  const float Angle=-180+I*45;BS->AddSample(Idle,FVector(Angle,0,0));
  for(int Gait=0;Gait<2;++Gait)
  {
   FString Clip=FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/Rifle/%s/MF_Rifle_%s_%s"),Gait?TEXT("Jog"):TEXT("Walk"),Gait?TEXT("Jog"):TEXT("Walk"),Dirs[I]);
   if(auto* A=LoadObject<UAnimSequence>(nullptr,*Clip))BS->AddSample(A,FVector(Angle,Gait?450:180,0));
  }
 }
 BS->ValidateSampleData();BS->ResampleData();BS->PostEditChange();NRSaveAsset(BS);
 // Small detailed equipment assets are attached to the world skeleton only, outside the camera.
 {
  FNRMeshMaker Pack({Steel,Grip,Receiver,Accent});
  Pack.Box({0,0,0},{28,16,38},1,4);Pack.Box({0,-8,0},{25,3,30},0,1);
  for(int X:{-1,1}){Pack.Box({X*15.,0,-2},{8,14,28},2,2);Pack.Box({X*11.,-10,8},{5,3,12},3,1);}
  for(int I=0;I<4;++I){Pack.Cylinder({-9.+I*6,0,22},2.6,9,0,16);Pack.Box({-9.+I*6,-7,20},{3,1.5,5},3,.5);}
  Pack.Save(TEXT("SM_AMSBackpack"));
  FNRMeshMaker Chest({Steel,Grip,Receiver,Accent});Chest.Box({0,0,0},{29,8,32},1,3);Chest.Box({0,-5,3},{24,4,20},2,3);
  for(int X:{-1,1})Chest.Box({X*8.,-8,-9},{10,6,13},0,1.3);
  Chest.Box({0,-7,12},{13,1,2},3,.2);Chest.Save(TEXT("SM_ChestRig"));
  FNRMeshMaker Rotor({Steel,Accent});for(int I=0;I<3;++I)Rotor.Box({0,0,0},{31,2.2,.8},I%2,.4,FRotator(0,I*60,0));Rotor.Save(TEXT("SM_DroneRotor"));
 }
 {
  auto* Suit=Finish(TEXT("M_OperatorArmor"),FLinearColor(.11,.135,.15),.25,.65);
  auto* Visor=Finish(TEXT("M_OperatorVisor"),FLinearColor(.015,.065,.085),.75,.16);
  FNRMeshMaker Helmet({Suit,Grip,Visor,Accent});Helmet.Sphere({0,0,0},{11,11,13},0,32,16);
  Helmet.Sphere({0,9,1},{9.7,3.3,6},2,32,12);
  for(int Side:{-1,1}){Helmet.Box({Side*10.,0,-1},{4,11,12},1,1.5);Helmet.Box({Side*12.2,2,2},{.5,4,1},3,.15);}
  Helmet.Box({0,-9,0},{13,5,12},1,2);Helmet.Save(TEXT("SM_OperatorHelmet"));
 }
 // Bake a two-bone support-hand correction into a separate animation, never per-frame on the server.
 auto* Source=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
 auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
 if(!Source||!Mesh)return 2;


 // Fit the replacement helmet to the actual bind-pose head bounds, not guessed bone axes.
 auto* Body=DuplicateObject<USkeletalMesh>(Mesh,CreatePackage(TEXT("/Game/Art/Meshes/SKM_OperatorBody")),TEXT("SKM_OperatorBody"));Body->SetFlags(RF_Public|RF_Standalone);
 const auto& BodyRef=Body->GetRefSkeleton();TSet<int> HeadBones;
 for(int B=0;B<BodyRef.GetNum();++B)for(int P=B;P!=INDEX_NONE;P=BodyRef.GetParentIndex(P))if(BodyRef.GetBoneName(P)==TEXT("head")){HeadBones.Add(B);break;}
 FBox HeadBounds(ForceInit);
 for(int LOD=0;LOD<Body->GetLODNum();++LOD)if(auto* Desc=Body->GetMeshDescription(LOD))
 {
  FSkeletalMeshAttributes Attr(*Desc);auto Weights=Attr.GetVertexSkinWeights();auto Positions=Attr.GetVertexPositions();TSet<FVertexID> HeadVertices;
  for(FVertexID V:Desc->Vertices().GetElementIDs())
  {
   float Weight=0;for(auto W:Weights.Get(V))if(HeadBones.Contains(W.GetBoneIndex()))Weight+=W.GetWeight();
   if(Weight>.65f){HeadVertices.Add(V);if(LOD==0)HeadBounds+=FVector(Positions[V]);}
  }
  TArray<FPolygonID> Remove;for(FPolygonID P:Desc->Polygons().GetElementIDs())
  {bool Head=false;for(FVertexInstanceID V:Desc->GetPolygonVertexInstances(P))Head|=HeadVertices.Contains(Desc->GetVertexInstanceVertex(V));if(Head)Remove.Add(P);}
  Desc->DeletePolygons(Remove);FElementIDRemappings Remap;Desc->Compact(Remap);Body->CommitMeshDescription(LOD);
 }
 TArray<FTransform> RefCS=BodyRef.GetRefBonePose();for(int B=1;B<RefCS.Num();++B)RefCS[B]=RefCS[B]*RefCS[BodyRef.GetParentIndex(B)];
 const FTransform HeadRef=RefCS[BodyRef.FindBoneIndex(TEXT("head"))];
 auto* HelmetSocket=NewObject<USkeletalMeshSocket>(Body);HelmetSocket->SocketName=TEXT("NR_Helmet");HelmetSocket->BoneName=TEXT("head");
 HelmetSocket->RelativeLocation=HeadRef.InverseTransformPosition(HeadBounds.GetCenter());HelmetSocket->RelativeRotation=HeadRef.GetRotation().Inverse().Rotator();
 HelmetSocket->RelativeScale=HeadBounds.GetExtent()/FVector(11,11,13)*1.09;
 Body->GetMeshOnlySocketList().Add(HelmetSocket);Body->PostEditChange();NRSaveAsset(Body);
 UE_LOG(LogTemp,Display,TEXT("NR_HELMET bounds %s extent %s"),*HeadBounds.GetCenter().ToString(),*HeadBounds.GetExtent().ToString());
 auto* ViewSkeleton=DuplicateObject<USkeleton>(Mesh->GetSkeleton(),CreatePackage(TEXT("/Game/Art/Animations/SK_FirstPerson")),TEXT("SK_FirstPerson"));
 ViewSkeleton->SetFlags(RF_Public|RF_Standalone);
 ViewSkeleton->SetBoneTranslationRetargetingMode(0,EBoneTranslationRetargetingMode::Animation,true);NRSaveAsset(ViewSkeleton);
 const auto& Ref=Mesh->GetRefSkeleton();const auto* Model=Source->GetDataModel();
 TArray<FTransform> Original=Ref.GetRefBonePose(),BaseCS;BaseCS.SetNum(Original.Num());
 for(int I=0;I<Original.Num();++I){FName N=Ref.GetBoneName(I);if(Model->IsValidBoneTrackName(N))Original[I]=Model->GetBoneTrackTransform(N,0);const int P=Ref.GetParentIndex(I);BaseCS[I]=P==INDEX_NONE?Original[I]:Original[I]*BaseCS[P];}
 const auto* Socket=Mesh->FindSocket(TEXT("HandGrip_R"));if(!Socket)return 3;
 const FTransform Gun=Socket->GetSocketLocalTransform()*BaseCS[Ref.FindBoneIndex(Socket->BoneName)];
 const int UL=Ref.FindBoneIndex(TEXT("upperarm_l")),LL=Ref.FindBoneIndex(TEXT("lowerarm_l")),HL=Ref.FindBoneIndex(TEXT("hand_l"));
 const int UR=Ref.FindBoneIndex(TEXT("upperarm_r")),LR=Ref.FindBoneIndex(TEXT("lowerarm_r")),HR=Ref.FindBoneIndex(TEXT("hand_r"));
 for(int Reload=0;Reload<2;++Reload)
 {
  const TCHAR* Name=Reload?TEXT("A_FP_Reload"):TEXT("A_FP_Ready");
  auto* Anim=DuplicateObject<UAnimSequence>(Source,CreatePackage(*(FString(TEXT("/Game/Art/Animations/"))+Name)),Name);Anim->SetFlags(RF_Public|RF_Standalone);
  Anim->SetSkeleton(ViewSkeleton);
  auto& Ctrl=Anim->GetController();Ctrl.OpenBracket(FText::FromString(TEXT("First-person support grip")),false);
  Ctrl.SetFrameRate(FFrameRate(30,1),false);Ctrl.SetNumberOfFrames(Reload?48:1,false);
  TArray<TArray<FVector>> Positions,Scales;TArray<TArray<FQuat>> Rotations;
  Positions.SetNum(Original.Num());Scales.SetNum(Original.Num());Rotations.SetNum(Original.Num());
  for(int Frame=0;Frame<=(Reload?48:1);++Frame)
  {
   TArray<FTransform> CS=BaseCS,Local=Original;
   auto Solve=[&](int U,int L,int H,FVector Root,FVector Pole,FVector Goal)
   {
    const double Upper=FVector::Dist(CS[U].GetLocation(),CS[L].GetLocation()),Lower=FVector::Dist(CS[L].GetLocation(),CS[H].GetLocation());
    CS[U].SetTranslation(Gun.TransformPosition(Root));
    AnimationCore::SolveTwoBoneIK(CS[U],CS[L],CS[H],Gun.TransformPosition(Pole),Goal,Upper,Lower,false,1.,1.);
    for(int I:{U,L,H})Local[I]=CS[I].GetRelativeTransform(CS[Ref.GetParentIndex(I)]);
   };
   FVector Left(6.8,25,-1.2);
   if(Reload)
   {
    const float T=Frame/48.f;
    const FVector P[]={Left,FVector(7,10,-12),FVector(9,7,-32),FVector(7,10,-12),Left};
    const float Times[]={0,.23f,.48f,.7f,1};
    for(int I=0;I<4;++I)if(T>=Times[I]&&T<=Times[I+1]){float A=FMath::SmoothStep(Times[I],Times[I+1],T);Left=FMath::Lerp(P[I],P[I+1],A);break;}
   }
   // Viewmodel shoulders sit below the camera. Complete upper arms hide all trimmed mesh boundaries.
   Solve(UL,LL,HL,FVector(20,-12,-26),FVector(32,-8,-45),Gun.TransformPosition(Left));
   Solve(UR,LR,HR,FVector(-20,-18,-30),FVector(-30,-15,-48),BaseCS[HR].GetLocation());
   for(int I=0;I<Local.Num();++I){Positions[I].Add(Local[I].GetLocation());Scales[I].Add(Local[I].GetScale3D());Rotations[I].Add(Local[I].GetRotation());}
  }
  for(int I=0;I<Original.Num();++I){FName N=Ref.GetBoneName(I);if(Model->IsValidBoneTrackName(N))Ctrl.SetBoneTrackKeys(N,Positions[I],Rotations[I],Scales[I],false);}
  Ctrl.CloseBracket(false);Anim->PostEditChange();NRSaveAsset(Anim);
 }
 UE_LOG(LogTemp,Display,TEXT("NR_POLISH_ASSETS_COMPLETE"));return 0;
}
