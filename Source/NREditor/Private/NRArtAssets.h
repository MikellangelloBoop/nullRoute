#pragma once
#include "CoreMinimal.h"
class UMaterial;
class UStaticMesh;
struct FNRArtAssets
{
    UMaterial *Concrete=nullptr,*Floor=nullptr,*Metal=nullptr,*Rubber=nullptr,*White=nullptr,*Cyan=nullptr,*Amber=nullptr,*Violet=nullptr,*Screen=nullptr,*Tracer=nullptr,*Impact=nullptr;
    UStaticMesh *Crate=nullptr,*Rack=nullptr,*Terminal=nullptr,*Drone=nullptr,*Core=nullptr,*Extraction=nullptr,*Panel=nullptr;
    void Build();
};
