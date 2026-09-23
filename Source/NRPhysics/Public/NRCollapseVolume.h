#pragma once
#include "CoreMinimal.h"
#include "NavModifierVolume.h"
#include "NRCollapseVolume.generated.h"
UCLASS()class NRPHYSICS_API ANRCollapseVolume:public ANavModifierVolume {
 GENERATED_BODY()
public:
 void SetHole(const FBox& InBounds);
 virtual FBox GetNavigationBounds()const override{return Hole;}
 virtual void GetNavigationData(FNavigationRelevantData& Data)const override;
private:FBox Hole=FBox(ForceInit);
};
