#include "NRMetaHumanArmorCommandlet.h"
#include "NRMeshMaker.h"
#include "NRFactionEmblems.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "ShaderCompiler.h"

namespace
{
 TArray<FVector2D> Panel(float W,float H){return {{-W*.32,-H*.5},{W*.32,-H*.5},{W*.5,-H*.3},{W*.47,H*.32},{W*.28,H*.5},{-W*.28,H*.5},{-W*.47,H*.32},{-W*.5,-H*.3}};}
 void Bolt(FNRMeshMaker& M,FVector P,float R=.42f){M.Cylinder(P,R,.3,4,10,FRotator(0,0,90));M.Box(P+FVector(0,.18,0),{R,.08,.10},2);}
 void Patch(FNRMeshMaker& M,FVector C,float W,float H,int F)
 {
  M.Plate(C,Panel(W+.6,H+.6),.4,2,2,.15);
  if(F>=3){NRReliefEmblem(M,C+FVector(0,.3,0),W,H,F);return;}
  TArray<FVector2D> UV;
  if(F==0)UV={{.506,.280},{.710,.274},{.713,.660},{.684,.729},{.611,.819},{.554,.750},{.508,.668}};
  else if(F==1){for(int I=0;I<32;++I){float A=2*PI*I/32;UV.Add({.505+.163*FMath::Cos(A),.503+.310*FMath::Sin(A)});}}
  else UV={{.468,.231},{.587,.119},{.734,.279},{.700,.795},{.568,.865},{.432,.708}};
  FVector2D Min(1,1),Max(0,0);for(auto V:UV){Min.X=FMath::Min(Min.X,V.X);Min.Y=FMath::Min(Min.Y,V.Y);Max.X=FMath::Max(Max.X,V.X);Max.Y=FMath::Max(Max.Y,V.Y);}
  auto Mid=(Min+Max)*.5;TArray<FVector> P;for(auto V:UV)P.Add(C+FVector((V.X-Mid.X)/(Max.X-Min.X)*W,.26,-(V.Y-Mid.Y)/(Max.Y-Min.Y)*H));
  for(int I=1;I<P.Num()-1;++I)M.TexturedFace({P[0],P[I],P[I+1]},{UV[0],UV[I],UV[I+1]},5);
 }
 UMaterial* MakeSuit()
 {
  auto* M=NewObject<UMaterial>(CreatePackage(TEXT("/Game/Art/MetaHuman/M_Undersuit")),TEXT("M_Undersuit"),RF_Public|RF_Standalone);
  auto* UV=NewObject<UMaterialExpressionTextureCoordinate>(M);M->GetExpressionCollection().AddExpression(UV);
  auto* Tint=NewObject<UMaterialExpressionVectorParameter>(M);Tint->ParameterName=TEXT("Tint");Tint->DefaultValue=FLinearColor(.025,.029,.034);M->GetExpressionCollection().AddExpression(Tint);
  auto* Weave=NewObject<UMaterialExpressionCustom>(M);Weave->OutputType=CMOT_Float1;Weave->Code=TEXT("float2 w=sin(UV*1800.0); return 0.8+0.2*w.x*w.y;");FCustomInput Input;Input.InputName=TEXT("UV");Input.Input.Expression=UV;Weave->Inputs.Reset();Weave->Inputs.Add(Input);M->GetExpressionCollection().AddExpression(Weave);
  auto* C=NewObject<UMaterialExpressionMultiply>(M);C->A.Expression=Tint;C->B.Expression=Weave;M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->BaseColor.Expression=C;
  auto* Normal=NewObject<UMaterialExpressionCustom>(M);Normal->OutputType=CMOT_Float3;Normal->Code=TEXT("return normalize(float3(0.07*sin(UV.x*1800.0),0.07*sin(UV.y*1800.0),1.0));");Normal->Inputs.Reset();Normal->Inputs.Add(Input);M->GetExpressionCollection().AddExpression(Normal);M->GetEditorOnlyData()->Normal.Expression=Normal;
  M->GetEditorOnlyData()->Roughness.UseConstant=true;M->GetEditorOnlyData()->Roughness.Constant=.82f;M->GetEditorOnlyData()->Metallic.UseConstant=true;M->GetEditorOnlyData()->Metallic.Constant=.02f;
  bool Recompile=false;M->SetMaterialUsage(Recompile,MATUSAGE_SkeletalMesh);M->PostEditChange();
  if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
  NRSaveAsset(M);return M;
 }
}
UNRMetaHumanArmorCommandlet::UNRMetaHumanArmorCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRMetaHumanArmorCommandlet::Main(const FString&)
{
 auto* Body=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/MetaHuman/SKM_BreacherBody"));if(!Body)return 1;
 const auto& Ref=Body->GetRefSkeleton();auto Pose=Ref.GetRefBonePose();for(int I=0;I<Pose.Num();++I)if(Ref.GetParentIndex(I)>=0)Pose[I]*=Pose[Ref.GetParentIndex(I)];
 auto B=[&](FName Name){int I=Ref.FindBoneIndex(Name);check(I!=INDEX_NONE);return Pose[I];};
 auto* Suit=MakeSuit();const TCHAR* Keys[]={TEXT("Chronos"),TEXT("Police"),TEXT("Rebel"),TEXT("Ascended"),TEXT("RustHounds")};int Saved=0;
 for(int Role=0;Role<4;++Role)for(int F=0;F<5;++F)
 {
  TArray<UMaterialInterface*> Mats;for(const TCHAR* S:{TEXT("Armor"),TEXT("Trim"),TEXT("Cloth"),TEXT("Glow"),TEXT("Steel"),TEXT("Patch")})Mats.Add(LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Materials/M_Breacher_%s_%s"),Keys[F],S)));if(F<3)Mats[2]=Suit;
  const bool Tech=Role==0,Recon=Role==1,Heavy=Role==2,Medic=Role==3;
  const TCHAR* Roles[]={TEXT("Engineer"),TEXT("Scout"),TEXT("Breacher"),TEXT("Medic")};
  auto Save=[&](FNRMeshMaker& M,const FString& Part){if(M.Save(*FString::Printf(TEXT("SM_MH_%s_%s_%s"),Roles[Role],Keys[F],*Part)))++Saved;};
  {
   FNRMeshMaker M(Mats);M.SetAuthoringTransform(B("head").Inverse());const FVector C=B("head").GetLocation()+FVector(0,0,6);
   if(F==3)
   {
    M.Sphere(C,{10.5,11.5,14.6},0,40,24);M.Cylinder(C+FVector(0,0,-14),6.1,15,2,32);
    M.Sphere(C+FVector(0,8,-1),{8.3,5.4,10.3},0,32,20);
    for(int I=0;I<5;++I)M.Sphere(C+FVector((I%2?1:-1)*(2.+I*.45),13,-4+I*2.1),{.65,.35,.65},3,12,8);
    M.Shell(C,{{9.9,11.5,-5},{10.1,11.7,-4.7},{9.6,11.3,9}},90,270,.8,4,3,28);
   }
   else
   {
   M.Sphere(C,{10.5,11.5,14.6},Tech?2:0,40,24);
   M.Cylinder(C+FVector(0,0,-14),6.1,15,2,32);
   // The brow wraps around the helmet instead of sitting on a flat rectangular face.
   M.Shell(C,{{10.7,11.6,-1.5},{11.1,12,-1},{11.2,12.4,3.5},{10.8,11.8,4.4}},-75,75,.6,2,1,30);
   M.Shell(C,{{10.9,12.1,.6},{11.2,12.7,1},{11.1,12.7,1.5},{10.9,12.1,1.8}},-58,58,.3,3,3,28);
   M.Plate(C+FVector(0,9,-7),{{-3.8,-6.4},{3.8,-6.4},{7,-1},{7.8,4.7},{3.5,6},{-3.5,6},{-7.8,4.7},{-7,-1}},5.5,0,4,.65);
   for(int S:{-1,1})
   {
    M.Cylinder(C+FVector(S*10,0,-1),3.7,2,1,28,FRotator(90,0,0));M.Cylinder(C+FVector(S*11.1,0,-1),2.8,.5,0,28,FRotator(90,0,0));
    M.Plate(C+FVector(S*6.6,10,-5),Panel(2.2,8),.8,1,4,.2);for(int I=0;I<3;++I)M.Box(C+FVector(S*4.6,12,-5-I*1.2),{2,.3,.35},2,.1);
    Bolt(M,C+FVector(S*7,9,7));
   }
   if(F==0){M.Shell(C,{{7.8,8.8,9},{6,9.2,10.4},{3.9,6.5,13.3},{1.8,3.2,14.4}},-32,32,.3,1,1,12);M.Plate(C+FVector(0,10,7.5),Panel(5,5),1,1,0,.3);}
   if((F==1&&Heavy)||Recon){M.Box(C+FVector(0,1,12.5),{10,7,2.5},0,.6);for(int S:{-1,1}){M.Cylinder(C+FVector(S*3,9,11),2,9,0,24,FRotator(0,0,90));M.Cylinder(C+FVector(S*3,13.6,11),1.5,.2,3,24,FRotator(0,0,90));}}
   if(F==2){M.Box(C+FVector(-7,-2,11),{3,10,2},4,.6);M.Box(C+FVector(8,-1,6),{4,8,5},0,.7);M.Cable({C+FVector(-10,0,-1),C+FVector(-11,6,-6),C+FVector(-6,10,-10)},.65,2);}
   if(Tech)
   {
    // Thick hood opening follows the head; rear fabric volume reads as a hood in profile.
    M.Shell(C,{{11.5,12.8,-16},{14.1,13.8,-10},{14,14.2,6},{10.8,12.5,15},{2.5,4,16}},45,315,1.1,2,2,36);
    M.Cable({C+FVector(-10,8,-12),C+FVector(-12,9,0),C+FVector(-8,8,13),C+FVector(0,5,16),C+FVector(8,8,13),C+FVector(12,9,0),C+FVector(10,8,-12)},.6,1,10);
    M.Box(C+FVector(7,12,3),{4,3,4},4,.5);
   }
   if(Recon){for(int S:{-1,1}){M.Cylinder(C+FVector(S*6,13,4),2,8,0,20,FRotator(0,0,90));M.Cylinder(C+FVector(S*6,17.1,4),1.5,.3,3,24,FRotator(0,0,90));}M.Box(C+FVector(0,13,4),{7,4,2},4,.4);}
   if(Medic||F==4)
   {
    for(int S:{-1,1}){M.Cylinder(C+FVector(S*6.4,13,-7),2.8,3.5,4,24,FRotator(0,0,90));M.Cylinder(C+FVector(S*6.4,15,-7),2.2,.6,2,24,FRotator(0,0,90));for(int I=0;I<3;++I)M.Box(C+FVector(S*6.4,15.4,-8.1+I),{3,.25,.35},1,.1);}
    M.Box(C+FVector(0,12,4),{17,2,4.4},2,.7);for(int S:{-1,1})M.Box(C+FVector(S*4.5,13.2,4),{6.8,.6,2.8},3,.5);
   }
   }
   Save(M,TEXT("head"));
  }
  {
   FNRMeshMaker M(Mats);M.SetAuthoringTransform(B("spine_04").Inverse());const FVector C(0,1,0);
   // Sectioned curved cuirass with a narrow metal rim and separate clavicle guards.
   M.Shell(C,{{17.5,13.5,115},{19,15,116},{21.5,17,131},{22,16.5,139},{18,14,147},{16.5,13,148}},-87,87,Heavy?2:1.1,Tech||Medic?2:0,F==0?1:4,32);
   M.Shell(C,{{17,12.5,117},{18,13.5,118},{20,14.5,137},{17,13,147},{16,12.5,148}},93,267,1.7,Tech||Medic?2:0,4,28);
   for(int S:{-1,1})
   {
    M.Plate({S*9.3,16.6,136},{{-7,-5},{5,-5.5},{7,1},{5,5},{-5,6},{-7,2}},1.7,0,F==0?1:4,.35);
    M.Box({S*14.,-1,148},{4.5,24,2.5},2,.8);M.Box({S*14.,8,148},{3.2,5,2.9},4,.3);
    M.Cable({{S*12.,14,145},{S*15.,17,138},{S*15.,18,130},{S*11.,18,125},{S*7.,17,125}},.55,2,10);
    Bolt(M,{S*17.,12.3,133});Bolt(M,{S*12.,13.5,145});Bolt(M,{S*15.,10,118});
    for(int I=0;I<(Recon?1:F==0?2:3);++I){float X=S*(4.+I*6);M.Box({X,17,120},{5.2,5,10},2,1);M.Box({X,19.7,123},{4.6,.55,2},0,.25);M.Box({X,19.8,118},{3,.5,1},4,.2);}
    M.Cylinder({S*10.,-14,134},2.4,19,4,24);M.Cylinder({S*10.,-14,143.4},2.65,2,1,24);
   }
   M.Shell({0,1,0},{{8.7,8.6,149},{9.2,9.3,150},{9.3,9.5,156},{9,9.1,156.5}},-180,180,1.2,0,1,40);
   for(int I=0;I<3;++I){float Z=109-I*4;M.Shell(C,{{15.2-I*.3,12.6,Z},{16.2-I*.3,13.5,Z+.6},{16.4-I*.3,13.9,Z+3},{15.8-I*.3,13,Z+3.5}},-70,70,1,0,4,24);}
   Patch(M,{0,18.7,136},5.6,8,F);
   M.Plate({0,-15,134},Panel(16,24),3.5,0,1,.5);for(int I=0;I<5;++I)M.Box({0,-17,128+I*2.6},{11,.5,.65},2,.2);
   if(F==1){M.Box({-14,-10,157},{.8,.8,22},4,.2);M.Box({0,19.5,128},{12,.4,.65},3,.15);}
   if(F==2){M.Plate({-12,18,132},Panel(7,12),2,4,4,.35);for(int I=0;I<3;++I)M.Box({12,17.5,130+I*2.1},{5,.5,.55},1,.15);}
   if(Tech)
   {
    M.Box({0,-21,134},{25,13,29},2,2);M.Plate({0,-28,135},Panel(20,23),3,0,1,.6);
    M.Box({-15,-17,159},{2,2,24},4,.3);M.Box({-15,-17,171},{3,3,2},3,.3);
    M.Plate({-9,20,130},Panel(12,18),2,0,1,.4);M.Box({-9,21.3,132},{8,.5,10},3,.4);
    for(int I=0;I<4;++I)M.Box({-9,21.65,128+I*2.2},{6,.2,.5},2,.1);
    for(int S:{-1,1})M.Cable({{S*11.,-22,148},{S*22.,-10,143},{S*22.,11,128},{S*15.,20,121}},.65,S==1?3:1,8);
    M.Shell(C,{{19.5,16,115},{22,17,109},{23,17,102}},70,290,.6,2,2,26);
   }
   if(Recon){M.Box({0,-19,138},{13,8,21},0,1.3);M.Box({-10,-13,157},{.75,.75,26},4,.2);M.Plate({0,19.1,144},Panel(21,6),1,0,1,.4);}
   if(Medic)
   {
    M.Box({0,-23,132},{31,19,34},2,2.5);M.Box({0,-33,134},{23,4,23},0,1.3);
    M.Box({0,-35.3,134},{14,.5,3.5},1,.3);M.Box({0,-35.4,134},{3.5,.5,14},1,.3);
    for(int S:{-1,1}){M.Cylinder({S*18.,-20,132},3.9,26,4,24);M.Cylinder({S*18.,-20,145},4.1,3,1,24);M.Cable({{S*18.,-20,146},{S*20.,-7,150},{S*18.,12,144},{S*13.,20,124}},.7,3,10);}
    M.Box({10,21,128},{10,5,12},2,.8);M.Box({10,23.8,129},{6,.5,1.8},1,.15);M.Box({10,23.9,129},{1.8,.5,6},1,.15);
    for(int I=0;I<3;++I){M.Cylinder({-14.+I*3.5,21,123},1.15,9,4,16);M.Cylinder({-14.+I*3.5,21,127.6},1.3,1.7,3,16);}
   }
   if(F==3){for(int S:{-1,1}){M.Sphere({S*13.,17,140},{1.7,1,1.7},3,16,10);M.Sphere({S*11.,-18,136},{1.5,1.5,1.5},3,16,10);}M.Shell(C,{{17,14,118},{21,17.8,135},{19,16,144}},-38,38,.5,0,4,22);Patch(M,{0,19,136},5.6,8,F);}
   if(F==4){M.Plate({-13,19,138},Panel(10,18),1.7,4,0,.5);for(int I=0;I<3;++I)M.Box({11,19,138+I*2.5},{7,.6,.8},3,.15,FRotator(12,0,0));}
   Save(M,TEXT("spine_04"));FNRMeshMaker Badge(Mats);Badge.SetAuthoringTransform(B("spine_04").Inverse());Patch(Badge,{0,21,135},8,11,F);Save(Badge,TEXT("Badge"));
  }
  {
   FNRMeshMaker M(Mats);M.SetAuthoringTransform(B("pelvis").Inverse());const auto C=B("pelvis").GetLocation();
   M.Shell(C,{{20,16,6},{20.8,16.8,6.5},{20.8,17,12},{20,16.4,12.5}},-180,180,1.6,2,2,40);
   M.Box(C+FVector(0,17,9.5),{8,2.3,5},4,.6);M.Box(C+FVector(0,18.3,9.5),{4.5,.35,2.2},1,.25);
   M.Plate(C+FVector(0,18,-3),{{-4,-10},{4,-10},{8,2},{7,6},{-7,6},{-8,2}},2.5,0,4,.4);
   for(int S:{-1,1}){M.Box(C+FVector(S*20,1,2),{6,9,11},2,1);M.Box(C+FVector(S*20,5.8,4),{4.5,.7,4},0,.35);}
   Save(M,TEXT("pelvis"));
  }
  for(int S:{-1,1})
  {
   const FString Side=S<0?TEXT("_r"):TEXT("_l");
   for(int Part=0;Part<4;++Part)
   {
    const FString A=(Part==0?TEXT("upperarm"):Part==1?TEXT("lowerarm"):Part==2?TEXT("thigh"):TEXT("calf"))+Side;
    const FString EndName=(Part==0?TEXT("lowerarm"):Part==1?TEXT("hand"):Part==2?TEXT("calf"):TEXT("foot"))+Side;
    const FVector Start=B(*A).GetLocation(),End=B(*EndName).GetLocation(),Z=(End-Start).GetSafeNormal();const float L=FVector::Distance(Start,End);
    const FVector X=FVector::CrossProduct(FVector(0,1,0),Z).GetSafeNormal();const FQuat Rot=FRotationMatrix::MakeFromXZ(X,Z).ToQuat();
    FNRMeshMaker M(Mats);M.SetAuthoringTransform(FTransform(Rot,Start)*B(*A).Inverse());
    const float RX=Part==0?8.5f:Part==1?6.6f:Part==2?12.3f:8.4f;const float RY=Part==0?8.1f:Part==1?6.7f:Part==2?15.5f:8.8f;
    const float Z0=L*(Part==2?.07f:.19f),Z1=L*(Recon?.66f:.83f);
    M.Shell({0,0,0},{{RX*.87,RY*.87,Z0},{RX,RY,Z0+1},{RX*.97,RY,Z0+(Z1-Z0)*.43f},{RX*.76,RY*.81,Z1-1},{RX*.69,RY*.75,Z1}},Recon?-62:-95,Recon?62:95,1.6,Tech&&Part==2?2:0,F==0?1:4,24);
    M.Shell({0,0,0},{{RX*.85,RY*.84,Z0+3},{RX*.9,RY*.9,Z0+3.6f},{RX*.73,RY*.75,Z1-3},{RX*.7,RY*.7,Z1-2.6f}},108,252,1,0,4,16);
    for(float P:{Z0+3,Z1-3})M.Shell({0,0,0},{{RX+.2,RY+.2,P},{RX+.4,RY+.4,P+.3},{RX+.4,RY+.4,P+1.5},{RX+.2,RY+.2,P+1.8}},-180,180,.8,2,2,32);
    for(int E:{-1,1}){M.Cylinder({E*(RX+.4),0,L*.23},2.2,1.5,4,20,FRotator(90,0,0));Bolt(M,{E*RX*.65,RY*.79,L*.31});}
    if(Part==0)
    {
     if(Heavy){
     M.Shell({0,0,0},{{7,7,-4},{9.2,9.2,-3},{12.3,12,3},{12,11.8,8},{10.5,10.5,12},{9.8,10,12.5}},-135,135,1.5,0,1,30);
     }else{M.Shell({0,0,0},{{7.2,7.2,-3},{9.8,9.8,0},{10.8,10.6,4},{9.8,9.8,10}},-110,110,1.1,Tech&&S<0?2:0,1,24);}
     Patch(M,{0,Heavy?12.2:10.8,5},6.3,7.5,F);
     if(Medic){M.Box({0,10.8,12},{6,.6,1.5},1,.2);M.Box({0,10.9,12},{1.5,.6,6},1,.2);}
     if(F==3)M.Sphere({0,10.8,2},{1.6,1,1.6},3,16,10);
     if(F==2&&S==1){M.Plate({-3,12.5,7},Panel(5,8),1,4,4,.3);M.Box({3,12,10},{4,.6,.7},1,.15);}
    }
    if(Part==1)
    {
     M.Plate({0,RY+1,L*.46},Panel(7,11),2,0,4,.4);M.Box({0,RY+2.1,L*.46},{4.5,.35,7},3,.25);
     for(int I=0;I<5;++I)M.Box({-.4,RY+2.35,L*.46-2.7+I*1.2},{3.2,.1,.3},2,.05);
     if(Tech&&S==1){M.Plate({0,RY+2,L*.46},Panel(11,16),1.5,0,1,.4);M.Box({0,RY+3,L*.46},{8,.4,11},3,.3);for(int I=0;I<5;++I)M.Box({0,RY+3.25,L*.46-4+I*2},{6,.1,.5},2,.1);}
     if(Medic&&S==-1){for(int I=0;I<3;++I)M.Cylinder({-2.+I*2,RY+2,L*.45},.7,8,1,12);}
     for(int I=0;I<3;++I)M.Box({0,-RY-1,L*.4+I*3},{5,2,1.5},0,.4);
    }
    if(Part==2)
    {
     M.Box({S*(RX+.5),0,L*.55},{5,9,13},2,.9);M.Box({S*(RX+.5),4.8,L*.55+3},{4,.5,3},0,.25);
     M.Shell({0,0,0},{{RX*.94,RY+ .3,Z0+5},{RX*.95,RY+.6,Z0+5.4},{RX*.8,RY*.91,Z1-4},{RX*.79,RY*.88,Z1-3.6}},20,23,.2,3,3,3);
    }
    if(Part==3)
    {
     M.Plate({0,10,2},{{-4,-5},{4,-5},{6.5,-1},{6,5},{3,7},{-3,7},{-6,5},{-6.5,-1}},3.4,0,1,.55);
     M.Plate({0,RY*.86,L*.56},Panel(6.7,L*.43),1.5,0,4,.35);
     M.Cable({{S*(RX+.7),0,6},{S*(RX+.7),-1,17},{S*(RX*.75),-2,L*.84}},.85,4,12);
    }
    Save(M,A);
   }
   {
    const FString A=TEXT("hand")+Side;FNRMeshMaker M(Mats);const FVector Start=B(*A).GetLocation(),End=B(*(TEXT("middle_01")+Side)).GetLocation();const FVector Z=(End-Start).GetSafeNormal(),X=FVector::CrossProduct(FVector(0,1,0),Z).GetSafeNormal();const FQuat Rot=FRotationMatrix::MakeFromXZ(X,Z).ToQuat();M.SetAuthoringTransform(FTransform(Rot,Start)*B(*A).Inverse());
    // Small dorsal guard; all five fingers are the skinned MetaHuman mesh.
    M.Plate({0,-2.2,4},Panel(7,6.5),1,0,4,.3);for(int I=0;I<4;++I)M.Box({-2.5+I*1.65,-2.8,7},{1.25,1.2,1.9},1,.4);Save(M,A);
   }
   {
    const FString A=TEXT("foot")+Side;FNRMeshMaker M(Mats);M.SetAuthoringTransform(B(*A).Inverse());auto C=B(*A).GetLocation();C.Z=5;
    M.Box(C+FVector(0,8,-3.5),{14.5,33,8},2,1.2);M.Box(C+FVector(0,9,2),{13.5,29,8},0,1.6);
    M.Box(C+FVector(0,19,1),{14,10,7},0,1.3);M.Plate(C+FVector(0,24,1.5),Panel(13,6),1.6,0,4,.3);
    M.Shell(C+FVector(0,1,0),{{5.8,6,4},{6.4,6.5,4.6},{6.2,6.3,10},{5.8,5.9,10.4}},-180,180,1,0,1,30);
    for(int I=0;I<3;++I)M.Box(C+FVector(0,4+I*4,6.2-I*.5),{10,1.5,1.5},2,.4);
    for(int I=0;I<7;++I)M.Box(C+FVector(0,-4+I*4.5,-7.4),{13,1.6,1},4,.2);Save(M,A);
   }
  }
  if(!Heavy){FNRMeshMaker M(Mats);M.Sleeve(2);M.Plate({0,4.3,73},Panel(Tech?10:7,Tech?35:25),2,0,1,.4);M.Cylinder({0,0,94},3.6,3,1,24);if(Tech)M.Box({0,5.6,80},{6,.7,10},3,.3);if(Medic){M.Box({0,5.6,73},{6,.6,1.5},1,.2);M.Box({0,5.7,73},{1.5,.6,6},1,.2);}Save(M,TEXT("Sleeve"));}
 }
 UE_LOG(LogTemp,Display,TEXT("NR_METAHUMAN_ARMOR_COMPLETE meshes=%d"),Saved);return Saved==335?0:2;
}
