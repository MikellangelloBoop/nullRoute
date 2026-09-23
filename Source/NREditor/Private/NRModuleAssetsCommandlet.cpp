#include "NRModuleAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Materials/MaterialInterface.h"

UNRModuleAssetsCommandlet::UNRModuleAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRModuleAssetsCommandlet::Main(const FString&)
{
 auto* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponSteel"));
 auto* Rubber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponGrip"));
 auto* Edge=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponEdge"));
 auto* Accent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponAccent"));
 if(!Metal||!Rubber||!Edge||!Accent)return 1;
 for(int Kind=0;Kind<2;++Kind)
 {
  FNRMeshMaker M({Metal,Rubber,Edge,Accent});const float Half=Kind==0?3.1f:3.9f,Length=Kind==0?3.f:11.f;
  M.Box({0,0,0},{7,Length+3,1.5},0,.35);M.Box({0,0,1.1},{4,Length+1,1.6},1,.25);
  // An open optic window avoids an opaque lens blocking the eye ray in ADS.
  for(int Side:{-1,1}){M.Box({Side*Half,0,4.7},{.75,Length,6.2},0,.3);M.Cylinder({Side*(Half+.55),0,4.6},1.1,1.2,2,16,FRotator(90,0,0));}
  M.Box({0,0,7.4},{Half*2+.6f,Length,.75},0,.3);M.Box({0,0,1.9},{Half*2,Length,.7},0,.2);
  M.Box({0,-Length*.5,1.5},{1.3,.3,.4},3,.1);
  if(!M.Save(Kind==0?TEXT("SM_ModReflex"):TEXT("SM_Mod2x")))return 2;
 }
 {
  FNRMeshMaker M({Metal,Rubber,Edge,Accent});M.Cylinder({0,8,0},2.1,16,0,32,FRotator(0,0,90));
  for(int I=0;I<5;++I)M.Cylinder({0,2.+I*3,0},2.2,.5,2,32,FRotator(0,0,90));
  M.Cylinder({0,16.1,0},1.8,.2,1,24,FRotator(0,0,90));M.Cylinder({0,16.3,0},.6,.25,0,16,FRotator(0,0,90));
  if(!M.Save(TEXT("SM_ModSuppressor")))return 3;
 }
 {
  FNRMeshMaker M({Metal,Rubber,Edge,Accent});M.Box({0,2.5,0},{4.8,5,4.8},0,.65);
  for(int Side:{-1,1})for(int I=0;I<3;++I)M.Box({Side*2.42,1.+I*1.2,0},{.12,.55,2.5},1,.05);
  M.Cylinder({0,5.1,0},1.8,.25,1,20,FRotator(0,0,90));if(!M.Save(TEXT("SM_ModCompensator")))return 4;
 }
 for(int Kind=0;Kind<2;++Kind)
 {
  FNRMeshMaker M({Metal,Rubber,Edge,Accent});
  if(Kind==0){M.Box({0,0,-4},{5.4,8.6,9},0,.65);for(int S:{-1,1})for(int I=0;I<4;++I)M.Box({S*2.73,0,-1.-I*1.8},{.2,6,.45},1,.1);M.Box({0,0,-8.5},{6.2,9.3,1.3},2,.3);}
  else {M.Box({0,0,-.4},{5.7,8.8,1.8},0,.4);for(int S:{-1,1})M.Box({S*2.4,0,-2.3},{.8,7,4},1,.3);M.Box({0,0,-4},{5.5,7,1.2},3,.3);}
  if(!M.Save(Kind==0?TEXT("SM_ModExtended"):TEXT("SM_ModQuick")))return 5;
 }
 UE_LOG(LogTemp,Display,TEXT("NR_MODULE_ASSETS six modular parts saved"));return 0;
}
