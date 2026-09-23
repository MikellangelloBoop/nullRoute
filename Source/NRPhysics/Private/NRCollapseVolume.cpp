#include "NRCollapseVolume.h"
#include "AI/Navigation/NavigationRelevantData.h"
#include "AI/NavigationModifier.h"
#include "NavAreas/NavArea_Null.h"
#include "NavigationSystem.h"
void ANRCollapseVolume::SetHole(const FBox& B){Hole=B;SetAreaClass(UNavArea_Null::StaticClass());UNavigationSystemV1::UpdateActorInNavOctree(*this);}
void ANRCollapseVolume::GetNavigationData(FNavigationRelevantData& D)const{if(Hole.IsValid)D.Modifiers.Add(FAreaNavModifier(Hole,FTransform::Identity,UNavArea_Null::StaticClass()));}
