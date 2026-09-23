#include "NRBreacherAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "NRFactionEmblems.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
UMaterial* Finish(const FString& Name,FLinearColor Color,float Metal,float Rough,float Glow=0)
{
 auto* M=NewObject<UMaterial>(CreatePackage(*(TEXT("/Game/Art/Materials/")+Name)),*Name,RF_Public|RF_Standalone);
 auto* C=NewObject<UMaterialExpressionVectorParameter>(M);C->ParameterName=TEXT("Tint");C->DefaultValue=Color;
 M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->BaseColor.Expression=C;
 M->GetEditorOnlyData()->Metallic.UseConstant=true;M->GetEditorOnlyData()->Metallic.Constant=Metal;
 M->GetEditorOnlyData()->Roughness.UseConstant=true;M->GetEditorOnlyData()->Roughness.Constant=Rough;
 if(Glow>0){auto* E=NewObject<UMaterialExpressionMultiply>(M);E->A.Expression=C;E->ConstB=Glow;M->GetExpressionCollection().AddExpression(E);M->GetEditorOnlyData()->EmissiveColor.Expression=E;}
 bool Recompile=false;M->SetMaterialUsage(Recompile,MATUSAGE_SkeletalMesh);M->PostEditChange();NRSaveAsset(M);return M;
}
TArray<FVector2D> Shield(float W,float H)
{
 return {{-W*.35,-H*.5},{W*.35,-H*.5},{W*.5,-H*.28},{W*.5,H*.27},{W*.3,H*.5},{-W*.3,H*.5},{-W*.5,H*.27},{-W*.5,-H*.28}};
}
void Bolt(FNRMeshMaker& M,FVector P,float R=.55){M.Cylinder(P,R,.45,4,10,FRotator(0,0,90));M.Box(P+FVector(0,.26,0),{R,.1,.12},2);}
void Badge(FNRMeshMaker& M,FVector C,float W,float H,int F)
{
 M.Plate(C,Shield(W+1,H+1),.6,2,1,.2);
 // Contour coordinates measured on the supplied 2048 x 1117 reference views.
 if(F>=3){NRReliefEmblem(M,C+FVector(0,.3,0),W,H,F);return;}
  TArray<FVector2D> UV;
 if(F==0)UV={{.506,.280},{.710,.274},{.713,.660},{.684,.729},{.611,.819},{.554,.750},{.508,.668}};
 else if(F==1){for(int I=0;I<32;++I){float A=2*PI*I/32;UV.Add({.505+.163*FMath::Cos(A),.503+.310*FMath::Sin(A)});}}
 else UV={{.468,.231},{.587,.119},{.734,.279},{.700,.795},{.568,.865},{.432,.708}};
 FVector2D Min(1,1),Max(0,0);for(auto V:UV){Min.X=FMath::Min(Min.X,V.X);Min.Y=FMath::Min(Min.Y,V.Y);Max.X=FMath::Max(Max.X,V.X);Max.Y=FMath::Max(Max.Y,V.Y);}
 FVector2D Mid=(Min+Max)*.5;TArray<FVector> P;for(auto V:UV)P.Add(C+FVector((V.X-Mid.X)/(Max.X-Min.X)*W,.38,-(V.Y-Mid.Y)/(Max.Y-Min.Y)*H));
 for(int I=1;I<P.Num()-1;++I)M.TexturedFace({P[0],P[I],P[I+1]},{UV[0],UV[I],UV[I+1]},5);
}
}

UNRBreacherAssetsCommandlet::UNRBreacherAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRBreacherAssetsCommandlet::Main(const FString&)
{
 IFileManager::Get().MakeDirectory(*(FPaths::ProjectDir()/TEXT("ArtSource/Breacher/Models")),true);
 auto* Skeleton=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Meshes/SKM_OperatorBody"));if(!Skeleton)return 1;
 const auto& Ref=Skeleton->GetRefSkeleton();TArray<FTransform> Pose=Ref.GetRefBonePose();
 FString Bind=TEXT("name,parent,x,y,z,qx,qy,qz,qw\n");
 for(int I=0;I<Pose.Num();++I){const auto P=Pose[I].GetLocation();const auto Q=Pose[I].GetRotation();Bind+=FString::Printf(TEXT("%s,%d,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f\n"),*Ref.GetBoneName(I).ToString(),Ref.GetParentIndex(I),P.X,P.Y,P.Z,Q.X,Q.Y,Q.Z,Q.W);}
 FFileHelper::SaveStringToFile(Bind,*(FPaths::ProjectDir()/TEXT("ArtSource/Breacher/Models/BindPose.csv")));
 for(int I=0;I<Pose.Num();++I)if(Ref.GetParentIndex(I)!=INDEX_NONE)Pose[I]*=Pose[Ref.GetParentIndex(I)];
 auto Bone=[&](FName N){const int I=Ref.FindBoneIndex(N);check(I!=INDEX_NONE);return Pose[I];};
 for(const TCHAR* N:{TEXT("head"),TEXT("spine_04"),TEXT("pelvis"),TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l")})UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_BONE %s %s"),N,*Bone(N).GetLocation().ToString());
 const TCHAR* Keys[]={TEXT("Chronos"),TEXT("Police"),TEXT("Rebel"),TEXT("Ascended"),TEXT("RustHounds")};
 int Saved=0;
 for(int F=0;F<5;++F)
 {
  const FString Prefix=FString(TEXT("M_Breacher_"))+Keys[F]+TEXT("_");
  auto* Armor=Finish(Prefix+TEXT("Armor"),F==3?FLinearColor(.76,.80,.81):F==4?FLinearColor(.19,.085,.037):F==0?FLinearColor(.032,.041,.048):F==1?FLinearColor(.035,.070,.105):FLinearColor(.085,.082,.076),.65,F==4?.88:F==2?.68:.42);
  auto* Trim=Finish(Prefix+TEXT("Trim"),F==3?FLinearColor(.53,.60,.67):F==4?FLinearColor(.32,.35,.15):F==0?FLinearColor(.63,.40,.115):F==1?FLinearColor(.46,.57,.62):FLinearColor(.40,.08,.31),.78,.34);
  auto* Cloth=Finish(Prefix+TEXT("Cloth"),F==4?FLinearColor(.075,.09,.038):F==3?FLinearColor(.09,.12,.15):FLinearColor(.018,.022,.027),.03,.88);
  auto* Glow=Finish(Prefix+TEXT("Glow"),F==3?FLinearColor(.48,.05,.94):F==4?FLinearColor(.94,.32,.035):F==2?FLinearColor(.22,.8,.07):FLinearColor(.015,.65,.9),.25,.24,2.2);
  auto* Steel=Finish(Prefix+TEXT("Steel"),FLinearColor(.17,.20,.22),.85,.35);
  auto* Patch=Finish(Prefix+TEXT("Patch"),FLinearColor::White,.12,.68);
  if(F<3){auto* Texture=LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/Art/Textures/T_Patch"))+Keys[F]));if(!Texture)return 2;
  auto* Sample=NewObject<UMaterialExpressionTextureSample>(Patch);Sample->Texture=Texture;Sample->SamplerType=SAMPLERTYPE_Color;
  Patch->GetExpressionCollection().AddExpression(Sample);Patch->GetEditorOnlyData()->BaseColor.Expression=Sample;Patch->PostEditChange();NRSaveAsset(Patch);}
  TArray<UMaterialInterface*> Materials{Armor,Trim,Cloth,Glow,Steel,Patch};
  auto Save=[&](FNRMeshMaker& M,const FString& Part){if(M.Save(*(FString(TEXT("SM_Breacher_"))+Keys[F]+TEXT("_")+Part)))++Saved;};
  {
   FNRMeshMaker M(Materials);M.SetAuthoringTransform(Bone("head").Inverse());const FVector C=Bone("head").GetLocation()+FVector(0,0,3);
   M.Sphere(C,{11.1,10.4,13.4},0,32,18);
   M.Plate(C+FVector(0,8.8,-5),{{-7,-7},{7,-7},{9,1},{6,6},{-6,6},{-9,1}},5,0,1);
   M.Plate(C+FVector(0,9.8,3),{{-8,-2},{-4,-4},{4,-4},{8,-2},{8,3},{-8,3}},2,2,1,.35);
   M.Box(C+FVector(0,11.1,3),{13,.5,.65},3,.2);
   for(int S:{-1,1}){M.Box(C+FVector(S*7,10.7,2),{.6,.6,2.5},3,.2);M.Cylinder(C+FVector(S*10.4,0,0),4.3,2.6,1,24,FRotator(90,0,0));M.Cylinder(C+FVector(S*11.8,0,0),3.1,.5,0,24,FRotator(90,0,0));M.Plate(C+FVector(S*5,9,-6),Shield(3,8),1,1,4,.25);}
   if(F==0){M.Box(C+FVector(0,0,13),{6,15,1},1,.4);M.Plate(C+FVector(0,7,10),Shield(6,5),2,1,4,.4);}
   if(F==1){M.Box(C+FVector(0,-1,13),{11,7,2},0,.5);for(int S:{-1,1}){M.Cylinder(C+FVector(S*3,8,12),2.2,9,0,20,FRotator(0,0,90));M.Cylinder(C+FVector(S*3,12.7,12),1.7,.3,3,20,FRotator(0,0,90));}}
   if(F==2){M.Box(C+FVector(-6,-2,12),{2,11,2},1,.5);M.Box(C+FVector(7,-2,9),{3,9,5},4,.6);M.Cylinder(C+FVector(-11,4,-4),2.1,3,2,16,FRotator(90,0,0));}
   Save(M,TEXT("head"));
  }
  {
   FNRMeshMaker M(Materials);M.SetAuthoringTransform(Bone("spine_04").Inverse());const FVector C(0,0,132);
   M.Sphere(C+FVector(0,0,-3),{16,10,24},2,24,16);M.Cylinder({0,1,151},7.2,15,2,28);
   M.Plate(C+FVector(0,-9,1),Shield(33,35),6,0,1,1);
   M.Plate(C+FVector(0,12,1),Shield(35,35),8,0,1,1.1);
   for(int S:{-1,1}){M.Plate(C+FVector(S*9.1,17,7),Shield(16,15),3,0,1,.7);M.Box(C+FVector(S*14,0,18),{5,27,4},2,.8);Bolt(M,C+FVector(S*14,17,10));Bolt(M,C+FVector(S*12,17,-8));}
   M.Plate(C+FVector(0,13,21),{{-10,-4},{10,-4},{13,0},{10,5},{-10,5},{-13,0}},6,0,1,.8);
   for(int I=0;I<3;++I){M.Plate(C+FVector(0,12,-20-I*4),Shield(27-I*3,6),5,0,1,.45);}
   for(int S:{-1,1})for(int I=0;I<(F==0?2:3);++I){M.Box(C+FVector(S*(4+I*6),17,-10),{5,5,10},2,.65);M.Box(C+FVector(S*(4+I*6),19.6,-7),{4,.6,2},0,.25);}
   Badge(M,C+FVector(0,19,8),8,12,F);
   for(int S:{-1,1})M.Cylinder(C+FVector(S*11,-14,0),3,24,F==0?1:4,20);
   M.Plate(C+FVector(0,-16,0),Shield(17,27),5,0,1,.9);
   for(int I=0;I<5;++I)M.Box(C+FVector(0,-19,-7+I*3),{12,1,1},2,.3);
   if(F==1){M.Box(C+FVector(-13,-13,26),{1.2,1.2,24},4,.2);M.Box(C+FVector(0,19,-2),{16,.5,1},3,.2);}
   if(F==2){M.Plate(C+FVector(-12,20,1),Shield(8,18),2,4,1,.5);for(int I=0;I<4;++I)M.Box(C+FVector(11,18,3+I*3),{6,.7,.8},1,.2,FRotator(12,0,0));}
   Save(M,TEXT("spine_04"));
   FNRMeshMaker B(Materials);B.SetAuthoringTransform(Bone("spine_04").Inverse());Badge(B,{0,23,135},9,13,F);Save(B,TEXT("Badge"));
  }
  {
   FNRMeshMaker M(Materials);M.SetAuthoringTransform(Bone("pelvis").Inverse());const FVector C=Bone("pelvis").GetLocation();
   M.Sphere(C,{16,10,12},2,24,14);
   M.Box(C+FVector(0,0,4),{34,24,7},2,1);M.Box(C+FVector(0,14,4),{9,3,6},1,.65);
   M.Plate(C+FVector(0,10,-6),{{-7,-11},{7,-11},{11,5},{7,9},{-7,9},{-11,5}},6,0,1,.8);
   for(int S:{-1,1}){M.Box(C+FVector(S*18,-1,0),{7,11,13},2,.8);M.Box(C+FVector(S*18,5,2),{5,1,8},0,.6);}
   Save(M,TEXT("pelvis"));
  }
  for(int S:{-1,1})
  {
   const FString Side=S==-1?TEXT("_r"):TEXT("_l");
   for(int Part=0;Part<4;++Part)
   {
    const FString A=(Part==0?TEXT("upperarm"):Part==1?TEXT("lowerarm"):Part==2?TEXT("thigh"):TEXT("calf"))+Side;
    const FString B=(Part==0?TEXT("lowerarm"):Part==1?TEXT("hand"):Part==2?TEXT("calf"):TEXT("foot"))+Side;
    const FVector Start=Bone(*A).GetLocation(),End=Bone(*B).GetLocation();const float L=FVector::Distance(Start,End);
    const FVector Z=(End-Start).GetSafeNormal();const FVector X=FVector::CrossProduct(FVector(0,1,0),Z).GetSafeNormal();const FQuat R=FRotationMatrix::MakeFromXZ(X,Z).ToQuat();
    FNRMeshMaker M(Materials);M.SetAuthoringTransform(FTransform(R,Start)*Bone(*A).Inverse());
    const float W=Part==0?18:Part==1?13:Part==2?19:16;const float D=Part<2?8:10;
    M.Sphere({0,0,0},{W*.36,W*.36,W*.36},2,20,12);M.Sphere({0,0,L},{W*.29,W*.29,W*.29},2,20,12);
    M.Cylinder({0,0,L*.50},W*.35,L*.76,2,20);
    for(float ZPos:{L*.18f,L*.75f})M.Cylinder({0,0,ZPos},W*.40,2,Part==0?1:4,24);
    M.Plate({0,D*.42,L*.48},Shield(W,L*.80),D,0,1,.8);
    M.Plate({0,-D*.44,L*.50},Shield(W*.65,L*.66),3,0,4,.6);
    for(int E:{-1,1}){M.Cylinder({E*W*.43,0,L*.13},3.3,2.4,1,20,FRotator(90,0,0));Bolt(M,{E*W*.30,D*.95,L*.25});}
    if(Part==0){M.Sphere({0,0,2},{11,11,9},0,24,12);M.Plate({0,8,5},Shield(20,15),5,0,1,.9);Badge(M,{0,11,5},7,9,F);}
    if(Part==1){M.Plate({0,D*.96,L*.5},Shield(8,12),1.2,2,1,.3);M.Box({0,D+1,L*.52},{5,.5,7},3,.3);for(int I=0;I<3;++I)M.Box({0,D+1.3,L*.43+I*2},{3,.25,.35},2,.1);}
    if(Part==2){M.Box({S*9.0,0,L*.52},{5,12,15},2,.8);M.Box({W*.31,D*.93,L*.4},{.6,.5,L*.42},3,.2);}
    if(Part==3){M.Plate({0,8,2},Shield(16,14),5,0,1,.7);M.Plate({0,8,L*.62},Shield(11,L*.5),3,0,1,.6);}
    if(F==2){M.Plate({-W*.18,D*.94,L*.66},Shield(W*.5,L*.27),2,4,1,.5);for(int I=0;I<3;++I)M.Box({W*.2,D*.98,L*.43+I*2},{5,.5,.7},1,.15);}
    if(F==1&&Part==0)M.Box({0,11,12},{12,1,2},3,.3);
    Save(M,A);
   }
   {
    const FString A=TEXT("hand")+Side;const FVector Start=Bone(*A).GetLocation();const FVector End=Bone(*(TEXT("middle_01")+Side)).GetLocation();
    const FVector Z=(End-Start).GetSafeNormal();const FVector X=FVector::CrossProduct(FVector(0,1,0),Z).GetSafeNormal();const FQuat R=FRotationMatrix::MakeFromXZ(X,Z).ToQuat();
    FNRMeshMaker M(Materials);M.SetAuthoringTransform(FTransform(R,Start)*Bone(*A).Inverse());
    M.Box({0,0,3},{8,5.5,9},2,1);M.Plate({0,3.1,3},Shield(7.5,8),1.7,0,1,.35);
    for(int I=0;I<4;++I){M.Box({-3.+I*2,0,9},{1.8,4.5,5.8},2,.7);M.Box({-3.+I*2,2.4,7.5},{1.7,.8,2},1,.3);}
    M.Box({S*4.4,0,4},{3.5,4,6.5},2,.9,FRotator(0,0,S*25));Save(M,A);
   }
   {
    FString A=TEXT("foot")+Side;FNRMeshMaker M(Materials);M.SetAuthoringTransform(Bone(*A).Inverse());FVector C=Bone(*A).GetLocation();C.Z=7;
    M.Box(C+FVector(0,7,-3),{16,29,7},2,1.6);M.Box(C+FVector(0,10,1),{15,23,9},0,2);M.Plate(C+FVector(0,19,1),Shield(15,7),3,0,1,.5);
    M.Box(C+FVector(0,0,7),{13,11,8},0,1);for(int I=0;I<4;++I)M.Box(C+FVector(0,3+I*5,-6.3),{16,2,1.4},4,.3);Save(M,A);
   }
  }
  {
   // +Y forward and the existing HandGrip_R origin keep animation and module sockets compatible.
   FNRMeshMaker M(Materials);
   M.Box({0,8,5},{9,35,13},0,1.2);M.Box({0,29,4},{8,28,11},0,1);
   M.Box({0,-20,4},{7,23,12},0,1.1);M.Box({0,-32,4},{8,3,15},2,.7);
   M.Box({0,-6,-6},{5,8,13},2,.7,FRotator(0,0,-15));M.Box({0,7,-10},{7,10,12},0,.7);M.Box({0,7,-16},{8,11,2},1,.5);
   for(int S:{-1,1}){M.Box({S*4.65,6,6},{1,23,8},1,.45);M.Box({S*4.7,-19,7},{.5,13,1},3,.2);for(int I=0;I<6;++I)M.Box({S*4.2,18+I*3.8,6},{.4,1.7,3.2},2,.2);}
   for(int I=0;I<18;++I)M.Box({0,-10+I*3.1,12},{7,1.4,1.6},1,.25);
   M.Box({0,30,-3},{8,19,4},2,.6);for(int I=0;I<7;++I)M.Box({0,22+I*2.3,-5},{8.3,1,1.3},4,.25);
   for(float Z:{4.f,-.5f}){M.Cylinder({0,44,Z},2.25,13,4,28,FRotator(0,0,90));M.Cylinder({0,50.7,Z},1.7,.3,2,28,FRotator(0,0,90));}
   M.Box({0,47,8},{8,7,3},0,.6);M.Box({0,43,13.3},{6,3,2},0,.4);M.Box({0,43,14.5},{1,.8,.7},3,.15);
   if(F==1)M.Box({4.5,27,0},{3,8,4},0,.5);
   if(F==2)for(int I=0;I<3;++I)M.Box({0,25.+I*5,5},{9,1.2,12},2,.3);
   Save(M,TEXT("Weapon"));
  }
  {
   FNRMeshMaker M(Materials);M.Sleeve(2);M.Plate({0,4.3,68},Shield(9,48),3,0,1,.45);M.Box({0,6.1,79},{5,.5,9},3,.3);M.Cylinder({0,0,94},3.6,3,1,24);Save(M,TEXT("Sleeve"));
  }
 }
 UE_LOG(LogTemp,Display,TEXT("NR_BREACHER_ASSETS %d meshes saved"),Saved);return Saved==90?0:3;
}
