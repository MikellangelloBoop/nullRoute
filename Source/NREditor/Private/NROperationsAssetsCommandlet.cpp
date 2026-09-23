#include "NROperationsAssetsCommandlet.h"
#include "NRMeshMaker.h"
#include "Materials/MaterialInterface.h"
UNROperationsAssetsCommandlet::UNROperationsAssetsCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNROperationsAssetsCommandlet::Main(const FString&)
{
 auto* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_WeaponSteel"));auto* Dark=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Graphite"));
 auto* White=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Ceramic"));auto* Cyan=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_CyanLight"));auto* Amber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_AmberLight"));auto* Violet=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_PhantomLight"));
 if(!Steel||!Dark||!White||!Cyan||!Amber||!Violet)return 2;
 for(int K=0;K<3;++K)
 {
  FNRMeshMaker M({Steel,Dark,White,K==0?Cyan:K==1?Amber:Violet});
  M.Box({0,0,0},{K==1?55.:64.,44,K==2?35.:25.},0,5);
  M.Box({-5,0,16},{45,40,10},K==1?1:2,3);M.Box({0,0,-17},{40,28,10},1,3);
  for(int Side:{-1,1}){M.Box({-5,Side*30.,5},{15,33,9},0,2);M.Cylinder({-5,Side*41.,10},19,5,1,28);M.Cylinder({-5,Side*41.,9},21,3,0,28);M.Box({-7,Side*24.,0},{31,5,12},2,2);M.Box({-9,Side*27.,1},{18,2,3},3,.5);}
  M.Cylinder({34,0,2},11,8,1,24,FRotator(90,0,0));M.Cylinder({39,0,2},7,3,3,24,FRotator(90,0,0));M.Cylinder({41,0,2},3,2,0,20,FRotator(90,0,0));
  for(int I=0;I<5;++I)M.Box({-22.+I*8,0,23},{3,31,3},1,.6);
  if(K==0){M.Cylinder({-16,0,35},2,28,0,12);M.Sphere({-16,0,51},{5,5,3},3,12,6);M.Box({-23,0,22},{7,20,8},2,1);}
  if(K==1){for(int Side:{-1,1}){M.Box({-20,Side*13.,-8},{32,8,10},1,2,FRotator(0,Side*12,0));M.Cylinder({26,Side*12.,-12},3,24,0,12,FRotator(90,0,0));}M.Box({5,0,24},{18,6,3},3,.5);}
  if(K==2){M.Box({23,0,14},{12,55,28},2,4);M.Box({30,0,12},{3,43,18},1,2);for(int Side:{-1,1}){M.Cylinder({35,Side*21.,-12},6,26,0,16,FRotator(90,0,0));M.Box({18,Side*30.,7},{23,6,39},2,2);}M.Box({28,0,30},{6,24,3},3,.5);}
  M.Save(K==0?TEXT("SM_DroneWatcher"):K==1?TEXT("SM_DroneHunter"):TEXT("SM_DroneBastion"));
 }
 {
  FNRMeshMaker M({Steel,Dark,White,Cyan,Amber});M.Box({0,0,7},{205,82,14},0,4);M.Box({0,0,57},{181,60,90},1,5);M.Box({0,0,109},{198,75,14},2,3);
  for(int Side:{-1,1}){M.Box({0,Side*32.,61},{167,4,66},2,2);M.Box({0,Side*35.,74},{130,2,32},1,1);for(int I=0;I<9;++I)M.Box({-57.+I*14,Side*37.,74},{6,2,21},0,.4);M.Box({0,Side*37.,102},{164,2,3},4,.4);}
  M.Box({0,0,119},{110,42,6},0,2);M.Box({0,0,123},{93,33,2},3,1);M.Save(TEXT("SM_TacticalConsole"));
 }
 {
  FNRMeshMaker M({Steel,Dark,White,Cyan,Amber});M.Cylinder({0,0,9},34,18,0,24);M.Cylinder({0,0,70},10,120,0,16);M.Box({0,0,152},{30,30,74},2,4);
  for(int S:{-1,1}){M.Box({S*16.,0,153},{3,22,55},3,.6);M.Box({0,S*16.,153},{22,3,55},4,.6);}M.Cylinder({0,0,192},21,10,0,20);M.Save(TEXT("SM_RouteBeacon"));
 }
 {
  FNRMeshMaker M({Steel,Dark,White,Cyan,Amber});for(int Side:{-1,1}){M.Box({0,Side*185.,150},{60,55,300},0,4);M.Box({-32,Side*185.,155},{4,28,252},3,1);M.Box({0,Side*185.,16},{88,83,32},1,3);}
  M.Box({0,0,315},{70,425,55},2,4);M.Box({-37,0,315},{4,260,30},1,1);M.Box({-40,0,295},{2,340,4},4,.5);M.Save(TEXT("SM_SectorGate"));
 }
 UE_LOG(LogTemp,Display,TEXT("NR_OPERATIONS_ASSETS_COMPLETE"));return 0;
}
