#include "NRArsenalAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Materials/MaterialInterface.h"
UNRArsenalAssetsCommandlet::UNRArsenalAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNRArsenalAssetsCommandlet::Main(const FString&)
{
 TArray<UMaterialInterface*> Materials;
 for(const TCHAR* Key:{TEXT("M_WeaponReceiver"),TEXT("M_WeaponSteel"),TEXT("M_WeaponGrip"),TEXT("M_WeaponAccent"),TEXT("M_Ceramic"),TEXT("M_WeaponEdge")})
 {auto* M=LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Art/Materials/"))+Key));if(!M)return 1;Materials.Add(M);}
 for(int Kind=0;Kind<2;++Kind)
 {
  FNRMeshMaker M(Materials);const bool LMG=Kind==1;
  // Shared +Y barrel and grip origin preserve existing sockets, sights and hand animations.
  M.Box({0,5,4},{9,31,13},0,1.3);M.Box({0,8,10},{8.2,26,3},LMG?4:1,.6);
  M.Box({0,-17,3},{6,17,9},LMG?0:4,1);M.Box({0,-27,1},{7.5,4,14},2,1);
  M.Box({0,-28.5,1},{8,1,15},1,.3);M.Box({0,-17,8},{6.8,12,3},2,.5);
  M.Box({0,0,-7},{5.7,7,13},2,.65,FRotator(12,0,0));
  M.Box({0,5,-5.4},{1,9,1.5},1,.2);M.Box({0,9,-2.5},{1,1.5,7},1,.2);
  M.Cylinder({0,34,4},LMG?1.4f:2.3f,33,1,32,FRotator(0,0,90));
  M.Cylinder({0,50,4},LMG?2.0f:3.0f,4,1,32,FRotator(0,0,90));
  M.Cylinder({0,52.1,4},LMG?1.2f:2.2f,.25,2,32,FRotator(0,0,90));
  M.Box({0,28,4},{10,23,10},LMG?0:2,1);
  for(int I=0;I<9;++I){M.Box({0,19.+I*2.4,10.5},{8,.7,1.5},1,.15);for(int Side:{-1,1})M.Box({Side*5.05,20.+I*2.2,5},{.12,1.1,4.5},2,.05);}
  // Front post and open rear aperture line up at the established sight height.
  M.Box({0,42,11.5},{1.4,2,8},1,.3);M.Box({0,42,15.6},{.65,1.5,.55},3,.15);
  for(int Side:{-1,1})M.Box({Side*2.0,-7,13.5},{1,2,5},1,.2);
  M.Box({0,-7,11.5},{4.5,2,1},1,.2);
  for(int Side:{-1,1})
  {
   M.Box({Side*4.65,9,5},{.35,17,5},LMG?4:0,.2);
   for(float Y:{-6.f,2.f,15.f})M.Cylinder({Side*4.9,Y,4},.6,.55,5,12,FRotator(90,0,0));
  }
  M.Box({4.9,1,8},{.3,11,2.5},2,.1);M.Box({5.7,-1,8},{2,3,1.3},1,.2);
  if(LMG)
  {
   M.Box({0,12,-9},{14,14,19},0,1.1);M.Box({0,12,-18.5},{14.7,14.8,2},2,.5);
   for(int I=0;I<5;++I)M.Box({-7.1,7.+I*2.2,-9},{.3,1.1,12},5,.15);
   M.Box({0,8,13},{4,19,2},1,.3);
   for(int Side:{-1,1}){M.Box({Side*3.9,33,-2},{1.5,20,1.8},1,.35);M.Box({Side*3.9,41,-2},{2.6,3,2},2,.3);}
   M.Box({0,20,13.5},{8,5,3},3,.3);
  }
  else
  {
   M.Box({0,11,-10},{6,10,16},2,.65,FRotator(-8,0,0));M.Box({0,13,-18},{7,11,2},1,.4);
   for(int I=0;I<4;++I)M.Box({3.1,8.+I*2,-10},{.2,.8,10},5,.1);
   for(int I=0;I<4;++I){M.Cylinder({-5.2,-6.+I*4,6},1.0,5,3,16);M.Cylinder({-5.2,-6.+I*4,3.5},1.05,.6,1,16);}
   M.Box({0,26,-2.2},{7,15,2},3,.5);
  }
  if(!M.Save(LMG?TEXT("SM_Lancer60"):TEXT("SM_Forge12")))return 2;
 }
 UE_LOG(LogTemp,Display,TEXT("NR_ARSENAL_ASSETS_COMPLETE"));return 0;
}
