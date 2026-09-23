#include "NRExpansionAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Materials/MaterialInterface.h"
#include "TwoBoneIK.h"
UNRExpansionAssetsCommandlet::UNRExpansionAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRExpansionAssetsCommandlet::Main(const FString&)
{
 auto* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponSteel"));
 auto* Rubber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponGrip"));
 auto* Edge=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponEdge"));
 auto* Light=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_CyanLight"));
 auto* White=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Ceramic"));
 auto* Amber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponAccent"));
 {
  FNRMeshMaker M({Steel,Rubber,White,Amber});M.Blade(2);M.Box({0,0,0},{3.3,13,4.3},1,.65);M.Box({0,7,0},{5.4,1.5,7.2},0,.4);
  for(int I=0;I<7;++I)M.Box({0,-5.+I*1.7,0},{3.65,.55,4.5},1,.2);
  M.Box({0,-7,0},{3.9,1.3,4.9},0,.3);M.Box({1.87,-1,0},{.16,5,1},3,.05);M.Save(TEXT("SM_RouteKnife"));
 }
 for(int Kind=0;Kind<3;++Kind)
 {
  FNRMeshMaker M({Steel,Rubber,White,Light,Amber});
  if(Kind<2)
  {
   M.Box({0,0,-30},{76,78,34},0,4);M.Box({0,0,-5},{68,70,48},2,3);M.Box({0,0,28},{73,74,14},0,3);
   for(int S:{-1,1}){M.Box({S*37.,0,-8},{5,62,56},1,1);for(int I=0;I<6;++I)M.Box({S*40.,-24.+I*9,-8},{2,4,37},0,.6);}
   M.Box({-38,0,15},{3,48,23},1,1);M.Box({-40,0,17},{1,38,13},3,.4);
   for(int S:{-1,1})M.Box({0,S*39.,-23},{64,3,6},4,1);
   if(Kind==1){M.Cylinder({0,0,43},21,25,0,24);M.Cylinder({0,0,44},18,18,3,24);M.Cylinder({0,0,60},25,6,2,24);}
   else {M.Box({0,0,38},{45,38,8},2,2);M.Box({0,0,43},{30,8,2},4,.3);M.Box({0,0,43},{8,28,2},4,.3);}
  }
  else
  {
   M.Box({0,0,-39},{21,50,10},0,2);M.Box({0,0,-10},{8,9,57},0,1);
   M.Box({0,0,12},{8,46,46},2,4);M.Cylinder({-5,0,12},14,2,4,32,FRotator(90,0,0));M.Cylinder({-7,0,12},9,2,1,32,FRotator(90,0,0));M.Cylinder({-9,0,12},4,2,3,24,FRotator(90,0,0));
   M.Box({0,0,39},{8,22,13},0,2);
  }
  M.Save(Kind==0?TEXT("SM_SupplyStation"):Kind==1?TEXT("SM_RelayStation"):TEXT("SM_RangeTarget"));
 }
 {
  FNRMeshMaker M({Steel,Rubber,White,Light,Amber});
  M.Box({0,0,8},{250,90,16},0,4);M.Box({0,0,90},{232,65,154},2,6);M.Box({0,0,176},{248,77,17},0,3);
  for(int S:{-1,1}){M.Box({S*118.,0,90},{13,81,170},0,2);M.Box({0,S*34.,104},{207,3,89},1,1);M.Box({0,S*37.,149},{207,2,4},3,.5);
   for(int I=0;I<10;++I)M.Box({-92.+I*20,S*37.,95},{8,4,52},0,1);M.Box({0,S*38.,40},{195,2,15},4,1);}
  M.Save(TEXT("SM_ServiceDivider"));
 }
 {
  FNRMeshMaker M({Steel,Rubber,White,Light,Amber});M.Cylinder({0,0,18},115,36,0,48);M.Cylinder({0,0,194},75,320,1,40);
  for(int I=0;I<8;++I){float A=I*PI/4;FVector P(FMath::Cos(A)*82,FMath::Sin(A)*82,193);M.Box(P,{18,18,325},2,3,FRotator(0,I*45,0));M.Box(P*FVector(1.08,1.08,1),{4,4,285},3,1);}
  for(int Z:{45,160,285,355})M.Cylinder({0,0,double(Z)},98,12,0,48);M.Cylinder({0,0,373},105,24,2,48);M.Save(TEXT("SM_CoolingCore"));
 }

 auto* Source=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS"));
 auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
 auto* ViewSkeleton=LoadObject<USkeleton>(nullptr,TEXT("/Game/Art/Animations/SK_FirstPerson"));
 if(!Source||!Mesh||!ViewSkeleton)return 2;
 const auto& Ref=Mesh->GetRefSkeleton();const auto* Model=Source->GetDataModel();
 TArray<FTransform> Original=Ref.GetRefBonePose(),BaseCS;BaseCS.SetNum(Original.Num());
 for(int I=0;I<Original.Num();++I){FName N=Ref.GetBoneName(I);if(Model->IsValidBoneTrackName(N))Original[I]=Model->GetBoneTrackTransform(N,0);const int P=Ref.GetParentIndex(I);BaseCS[I]=P==INDEX_NONE?Original[I]:Original[I]*BaseCS[P];}
 const auto* Socket=Mesh->FindSocket(TEXT("HandGrip_R"));if(!Socket)return 3;
 const FTransform Gun=Socket->GetSocketLocalTransform()*BaseCS[Ref.FindBoneIndex(Socket->BoneName)];
 const int UL=Ref.FindBoneIndex(TEXT("upperarm_l")),LL=Ref.FindBoneIndex(TEXT("lowerarm_l")),HL=Ref.FindBoneIndex(TEXT("hand_l"));
 const int UR=Ref.FindBoneIndex(TEXT("upperarm_r")),LR=Ref.FindBoneIndex(TEXT("lowerarm_r")),HR=Ref.FindBoneIndex(TEXT("hand_r"));
 for(int Variant=0;Variant<3;++Variant)
 {
  const bool Reload=Variant==1;const TCHAR* Name=Variant==0?TEXT("A_FP_Pistol"):Variant==1?TEXT("A_FP_PistolReload"):TEXT("A_FP_Knife");
  auto* Anim=DuplicateObject<UAnimSequence>(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Art/Animations/A_FP_Ready")),CreatePackage(*(FString(TEXT("/Game/Art/Animations/"))+Name)),Name);Anim->SetFlags(RF_Public|RF_Standalone);
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
   FVector Left=Variant==2?FVector(28,-16,-25):FVector(5,1,-3);
   if(Reload)
   {
    const float T=Frame/48.f;
    const FVector P[]={Left,FVector(6,-2,-13),FVector(11,-5,-34),FVector(6,-2,-13),Left};
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

 auto* SourceBS=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Art/Animations/BS_Operator2D"));
 auto* BS=DuplicateObject<UBlendSpace>(SourceBS,CreatePackage(TEXT("/Game/Art/Animations/BS_OperatorPistol")),TEXT("BS_OperatorPistol"));BS->SetFlags(RF_Public|RF_Standalone);
 auto& Samples=const_cast<TArray<FBlendSample>&>(BS->GetBlendSamples());
 for(auto& Sample:Samples)if(Sample.Animation){FString Path=Sample.Animation->GetPathName().Replace(TEXT("Rifle"),TEXT("Pistol"));if(auto* A=LoadObject<UAnimSequence>(nullptr,*Path))Sample.Animation=A;}
 BS->ValidateSampleData();BS->ResampleData();BS->PostEditChange();NRSaveAsset(BS);

 UE_LOG(LogTemp,Display,TEXT("NR_EXPANSION_ASSETS_COMPLETE"));return 0;
}
