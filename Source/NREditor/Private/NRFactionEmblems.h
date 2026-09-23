#pragma once
#include "NRMeshMaker.h"

// Original relief insignia for the two factions without separate patch artwork.
// The three supplied patches retain their source photograph UVs in the generators.
inline void NRReliefEmblem(FNRMeshMaker& M,FVector C,float W,float H,int F)
{
 M.Plate(C,{{-W*.45,H*.42},{W*.45,H*.42},{W*.45,-H*.2},{0,-H*.5},{-W*.45,-H*.2}},.4,0,1,.1);
 if(F==3)
 {
  TArray<FVector> Ring;
  for(int I=0;I<=6;++I){float A=2*PI*I/6;Ring.Add(C+FVector(FMath::Cos(A)*W*.32,.36,FMath::Sin(A)*H*.32));}
  M.Cable(Ring,W*.025,1,6);
  for(int I=0;I<6;++I){M.Cable({C+FVector(0,.36,0),Ring[I]},W*.012,3,6);M.Sphere(Ring[I],FVector(W*.065,.13,W*.065),3,10,6);}
  M.Sphere(C+FVector(0,.5,0),FVector(W*.12,.16,W*.12),3,12,8);
 }
 else
 {
  M.Plate(C+FVector(0,.45,0),{{-W*.3,H*.29},{-W*.27,-H*.05},{0,-H*.28},{W*.27,-H*.05},{W*.3,H*.29},{W*.12,H*.17},{0,H*.2},{-W*.12,H*.17}},.25,1,4,.05);
  for(int S:{-1,1}){M.Box(C+FVector(S*W*.13,.65,H*.05),{W*.14,.12,H*.055},2,.02);M.Cable({C+FVector(S*W*.35,.6,-H*.18),C+FVector(S*W*.12,.6,-H*.37)},W*.025,3,6);}
  M.Plate(C+FVector(0,.67,-H*.1),{{-W*.075,0},{W*.075,0},{0,-H*.08}},.1,2,2,.02);
 }
}
