#include "NRDetailAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionLocalPosition.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "TwoBoneIK.h"

UNRDetailAssetsCommandlet::UNRDetailAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRDetailAssetsCommandlet::Main(const FString&)
{
 const TCHAR* Names[]={TEXT("M_SkinCarbon"),TEXT("M_SkinCeramic"),TEXT("M_SkinHazard"),TEXT("M_SkinPhantom")};
 for(int Skin=0;Skin<4;++Skin)
 {
  auto* M=NewObject<UMaterial>(CreatePackage(*(FString(TEXT("/Game/Art/Materials/"))+Names[Skin])),Names[Skin],RF_Public|RF_Standalone);
  auto* P=NewObject<UMaterialExpressionLocalPosition>(M);
  auto* T=NewObject<UMaterialExpressionTextureSample>(M);T->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Weapons/Rifle/Textures/T_Rifle_BC"));T->SamplerType=SAMPLERTYPE_Color;
  auto* N=NewObject<UMaterialExpressionTextureSample>(M);N->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Weapons/Rifle/Textures/T_Rifle_N"));N->SamplerType=SAMPLERTYPE_Normal;
  auto* C=NewObject<UMaterialExpressionCustom>(M);C->OutputType=CMOT_Float3;
  C->Code=FString::Printf(TEXT("float skin=%d;"),Skin)+TEXT(R"(
   float shade=dot(T,float3(.3,.59,.11));
   float stripe=smoothstep(.46,.54,frac(P.y*.07+P.z*.09));
   float panel=smoothstep(-6,-3,P.z)*(1-smoothstep(31,38,P.y));
   float3 a=float3(.035,.05,.058),b=float3(.08,.105,.12);
   if(skin==1){a=float3(.63,.72,.71);b=float3(.018,.38,.48);stripe=smoothstep(.78,.82,frac((P.y+P.z)*.038));}
   if(skin==2){a=float3(.74,.29,.028);b=float3(.035,.04,.042);}
   if(skin==3){a=float3(.12,.027,.23);b=float3(.41,.13,.58);float2 q=floor(P.yz*.24);stripe=step(.6,frac(sin(dot(q,float2(12.98,78.23)))*43758.54));}
   float weave=.97+.03*sin(P.y*19)*sin(P.z*19);
   float3 paint=lerp(a,b,stripe)*weave;
   float barcode=step(.0,P.y)*step(P.y,6)*step(1,P.z)*step(P.z,2.3)*step(.53,frac(P.y*4.7));
   paint=lerp(paint,float3(.65,.68,.60),barcode*.8);
   return lerp(float3(.021,.026,.03),paint,panel)*(.45+shade*.85);
  )");
  for(auto* E:{static_cast<UMaterialExpression*>(P),static_cast<UMaterialExpression*>(T),static_cast<UMaterialExpression*>(N),static_cast<UMaterialExpression*>(C)})M->GetExpressionCollection().AddExpression(E);
  FCustomInput IP;IP.InputName=TEXT("P");IP.Input.Expression=P;C->Inputs.Add(IP);
  FCustomInput IT;IT.InputName=TEXT("T");IT.Input.Expression=T;C->Inputs.Add(IT);
  M->GetEditorOnlyData()->BaseColor.Expression=C;M->GetEditorOnlyData()->Normal.Expression=N;
  M->GetEditorOnlyData()->Roughness.UseConstant=true;M->GetEditorOnlyData()->Roughness.Constant=Skin==1?.32f:.47f;
  M->GetEditorOnlyData()->Metallic.UseConstant=true;M->GetEditorOnlyData()->Metallic.Constant=.42f;
  M->PostEditChange();NRSaveAsset(M);
 }
 auto* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponSteel"));
 auto* Rubber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponGrip"));
 auto* Edge=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponEdge"));
 auto* Amber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponAccent"));
 {
  FNRMeshMaker M({Metal,Rubber,Edge,Amber});
  for(int Side:{-1,1})
  {
   for(int I=0;I<4;++I)M.Cylinder({Side*3.1,5.+I*6,4},.27,.22,2,12,FRotator(90,0,0));
   M.Box({Side*3.12,8,1.6},{.16,8,2.6},0,.07);
   M.Box({Side*3.24,8,1.6},{.1,4.8,.38},3,.02);
   for(int I=0;I<7;++I)M.Box({Side*3.15,-12.+I*1.3,3},{.22,.44,2},1,.07);
  }
  M.Save(TEXT("SM_RifleHardware"));
 }
 {
  FNRMeshMaker M({Metal,Rubber,Edge,Amber});M.Box({0,0,0},{200,14,8},0,1.5);
  for(int I=0;I<12;++I)M.Box({-91.+I*16,0,5},{4,16,2},2,.4);
  M.Save(TEXT("SM_CableTray"));
  FNRMeshMaker V({Metal,Rubber,Edge});V.Box({0,0,0},{5,120,70},0,2);V.Box({-3,0,0},{2,112,62},1,1);
  for(int I=0;I<12;++I)V.Box({-5,0,-28.+I*5},{3,109,1.7},2,.3);V.Save(TEXT("SM_WallVent"));
  FNRMeshMaker Pipe({Metal,Edge});Pipe.Cylinder({0,0,0},8,200,0,24,FRotator(90,0,0));
  for(int I:{-1,1}){Pipe.Cylinder({I*92.,0,0},10,6,1,24,FRotator(90,0,0));}Pipe.Save(TEXT("SM_ServicePipe"));
 }
 // Bake crouched versions with fixed foot goals. No runtime IK or extra server animation work.
 auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
 auto* SourceBS=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Art/Animations/BS_Operator2D"));if(!Mesh||!SourceBS)return 1;
 auto* BS=DuplicateObject<UBlendSpace>(SourceBS,CreatePackage(TEXT("/Game/Art/Animations/BS_OperatorCrouch")),TEXT("BS_OperatorCrouch"));BS->SetFlags(RF_Public|RF_Standalone);
 const auto& Ref=Mesh->GetRefSkeleton();const int Pelvis=Ref.FindBoneIndex(TEXT("pelvis"));
 TMap<UAnimSequence*,UAnimSequence*> Baked;
 auto& Samples=const_cast<TArray<FBlendSample>&>(BS->GetBlendSamples());
 for(auto& Sample:Samples)
 {
  auto* Source=Sample.Animation.Get();if(!Source)continue;
  if(Baked.Contains(Source)){Sample.Animation=Baked[Source];continue;}
  const FString Name=TEXT("A_Crouch_")+Source->GetName();
  auto* A=DuplicateObject<UAnimSequence>(Source,CreatePackage(*(TEXT("/Game/Art/Animations/")+Name)),*Name);A->SetFlags(RF_Public|RF_Standalone);
  const auto* Model=Source->GetDataModel();auto& Ctrl=A->GetController();Ctrl.OpenBracket(FText::FromString(TEXT("Bake crouched locomotion")),false);
  TArray<TArray<FVector>> Pos,Scale;TArray<TArray<FQuat>> Rot;Pos.SetNum(Ref.GetNum());Scale.SetNum(Ref.GetNum());Rot.SetNum(Ref.GetNum());
  for(int Frame=0;Frame<=Model->GetNumberOfFrames();++Frame)
  {
   auto Local=Ref.GetRefBonePose();
   for(int B=0;B<Ref.GetNum();++B)if(Model->IsValidBoneTrackName(Ref.GetBoneName(B)))Local[B]=Model->GetBoneTrackTransform(Ref.GetBoneName(B),FFrameNumber(Frame));
   auto BaseCS=Local;for(int B=1;B<Ref.GetNum();++B)BaseCS[B]=Local[B]*BaseCS[Ref.GetParentIndex(B)];
   Local[Pelvis].AddToTranslation(FVector(0,0,-32));
   auto CS=Local;for(int B=1;B<Ref.GetNum();++B)CS[B]=Local[B]*CS[Ref.GetParentIndex(B)];
   for(const TCHAR* Side:{TEXT("l"),TEXT("r")})
   {
    int U=Ref.FindBoneIndex(FName(*(FString(TEXT("thigh_"))+Side))),L=Ref.FindBoneIndex(FName(*(FString(TEXT("calf_"))+Side))),F=Ref.FindBoneIndex(FName(*(FString(TEXT("foot_"))+Side)));
    const double UL=FVector::Dist(BaseCS[U].GetLocation(),BaseCS[L].GetLocation()),LL=FVector::Dist(BaseCS[L].GetLocation(),BaseCS[F].GetLocation());
    AnimationCore::SolveTwoBoneIK(CS[U],CS[L],CS[F],BaseCS[L].GetLocation()+FVector(0,100,0),BaseCS[F].GetLocation(),UL,LL,false,1.,1.);
    CS[F].SetRotation(BaseCS[F].GetRotation());
    for(int B:{U,L,F})Local[B]=CS[B].GetRelativeTransform(CS[Ref.GetParentIndex(B)]);
   }
   for(int B=0;B<Ref.GetNum();++B){Pos[B].Add(Local[B].GetLocation());Scale[B].Add(Local[B].GetScale3D());Rot[B].Add(Local[B].GetRotation());}
  }
  for(int B=0;B<Ref.GetNum();++B)if(Model->IsValidBoneTrackName(Ref.GetBoneName(B)))Ctrl.SetBoneTrackKeys(Ref.GetBoneName(B),Pos[B],Rot[B],Scale[B],false);
  Ctrl.CloseBracket(false);A->PostEditChange();NRSaveAsset(A);Baked.Add(Source,A);Sample.Animation=A;
 }
 BS->ValidateSampleData();BS->ResampleData();BS->PostEditChange();NRSaveAsset(BS);
 UE_LOG(LogTemp,Display,TEXT("NR_DETAIL_ASSETS_COMPLETE"));return 0;
}
