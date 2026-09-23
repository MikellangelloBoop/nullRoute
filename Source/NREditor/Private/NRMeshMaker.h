#pragma once
#include "CoreMinimal.h"
#include "MeshDescription.h"
class UStaticMesh;
class UMaterialInterface;

// Editor-only mesh authoring. Runtime receives cooked, shared static mesh assets.
class FNRMeshMaker
{
public:
    FNRMeshMaker(const TArray<UMaterialInterface*>& InMaterials);
    void Box(FVector Center, FVector Size, int32 Material, float Bevel = 0, FRotator Rotation = FRotator::ZeroRotator);
    void Cylinder(FVector Center, float Radius, float Height, int32 Material, int32 Sides = 20, FRotator Rotation = FRotator::ZeroRotator);
    void Sleeve(int32 Material);
    void Blade(int32 Material);
    void Sphere(FVector Center, FVector Radius, int32 Material, int32 Sides = 20, int32 Rings = 10);
    void SetAuthoringTransform(const FTransform& Value){AuthoringTransform=Value;}
    void Plate(FVector Center, const TArray<FVector2D>& Outline, float Depth, int32 Surface, int32 Rim, float Bevel=0.7f);
    void TexturedFace(const TArray<FVector>& Points,const TArray<FVector2D>& UVs,int32 Material);
    void SmoothFace(const TArray<FVector>& Points,const TArray<FVector>& Normals,int32 Material);
    // Each section is (X radius, Y radius, height); angles run around +Y.
    void Shell(FVector Center,const TArray<FVector>& Sections,float StartDegrees,float EndDegrees,float Thickness,int32 Surface,int32 Edge,int32 Sides=24);
    void Cable(const TArray<FVector>& Points,float Radius,int32 Material,int32 Sides=10);
    UStaticMesh* Save(const TCHAR* Name);
private:
    void Face(TArray<FVector> Points, FVector Normal, int32 Material);
    FMeshDescription Mesh;
    TArray<FPolygonGroupID> Groups;
    TArray<UMaterialInterface*> Materials;
    FTransform AuthoringTransform=FTransform::Identity;
};
bool NRSaveAsset(UObject* Asset, bool bMap = false);
