#include "NRMeshMaker.h"
#include "Algo/Reverse.h"
#include "StaticMeshAttributes.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool NRSaveAsset(UObject* Asset, bool bMap)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkAsFullyLoaded();
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Asset);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), bMap ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());
    bool OK = UPackage::SavePackage(Package, Asset, *File, Args);
    if (!OK) UE_LOG(LogTemp, Error, TEXT("NR failed to save %s"), *File);
    return OK;
}

FNRMeshMaker::FNRMeshMaker(const TArray<UMaterialInterface*>& InMaterials) : Materials(InMaterials)
{
    FStaticMeshAttributes Attributes(Mesh);
    Attributes.Register();
    Attributes.GetVertexInstanceUVs().SetNumChannels(1);
    for (int32 Index = 0; Index < Materials.Num(); ++Index)
    {
        FPolygonGroupID Group = Mesh.CreatePolygonGroup();
        Groups.Add(Group);
        Attributes.GetPolygonGroupMaterialSlotNames()[Group] = FName(*FString::Printf(TEXT("Surface_%d"), Index));
    }
}

void FNRMeshMaker::Face(TArray<FVector> Points, FVector Normal, int32 Material)
{
    if (Points.Num() < 3) return;
    for(FVector& Point:Points)Point=AuthoringTransform.TransformPosition(Point);
    Normal=AuthoringTransform.TransformVectorNoScale(Normal);
    Normal.Normalize();
    // Unreal's left-handed mesh convention uses Cross(edge2, edge1).
    // Opposite winding culls exterior faces and exposes the unlit inside of armor.
    if (FVector::DotProduct(FVector::CrossProduct(Points[2]-Points[0], Points[1]-Points[0]), Normal) < 0) Algo::Reverse(Points);
    FStaticMeshAttributes A(Mesh);
    TArray<FVertexInstanceID> Indices;
    FVector Tangent = FVector::CrossProduct(Normal, FMath::Abs(Normal.Z) < .95 ? FVector::UpVector : FVector::RightVector).GetSafeNormal();
    FVector Bitangent = FVector::CrossProduct(Normal, Tangent);
    for (const FVector& Point : Points)
    {
        FVertexID Vertex = Mesh.CreateVertex();
        A.GetVertexPositions()[Vertex] = FVector3f(Point);
        FVertexInstanceID Instance = Mesh.CreateVertexInstance(Vertex);
        A.GetVertexInstanceNormals()[Instance] = FVector3f(Normal);
        A.GetVertexInstanceTangents()[Instance] = FVector3f(Tangent);
        A.GetVertexInstanceBinormalSigns()[Instance] = 1.f;
        A.GetVertexInstanceColors()[Instance] = FVector4f(1,1,1,1);
        A.GetVertexInstanceUVs().Set(Instance, 0, FVector2f(FVector::DotProduct(Point,Tangent)/100, FVector::DotProduct(Point,Bitangent)/100));
        Indices.Add(Instance);
    }
    Mesh.CreatePolygon(Groups[FMath::Clamp(Material,0,Groups.Num()-1)], Indices);
}

void FNRMeshMaker::Box(FVector Center, FVector Size, int32 Material, float Bevel, FRotator Rotation)
{
    const FVector H = Size * .5;
    const float B = FMath::Clamp(Bevel, 0.f, float(H.GetMin()*.8));
    auto Emit = [&](TArray<FVector> P, FVector N)
    {
        for (FVector& V : P) V = Center + Rotation.RotateVector(V);
        Face(P, Rotation.RotateVector(N), Material);
    };
    for (int32 Axis=0; Axis<3; ++Axis)
    {
        const int32 U=(Axis+1)%3, V=(Axis+2)%3;
        for (int32 Sign : {-1,1})
        {
            FVector N=FVector::ZeroVector; N[Axis]=Sign;
            TArray<FVector> P;
            for (FVector2D Corner : {FVector2D(-1,-1),FVector2D(1,-1),FVector2D(1,1),FVector2D(-1,1)})
            {
                FVector Q=FVector::ZeroVector; Q[Axis]=Sign*H[Axis]; Q[U]=Corner.X*(H[U]-B); Q[V]=Corner.Y*(H[V]-B); P.Add(Q);
            }
            Emit(P,N);
        }
    }
    if (B<=0) return;
    for (int32 Axis=0; Axis<3; ++Axis)
    {
        int32 U=(Axis+1)%3,V=(Axis+2)%3;
        for (int32 SU : {-1,1}) for (int32 SV : {-1,1})
        {
            TArray<FVector> P;
            for (FVector2D Corner : {FVector2D(-1,0),FVector2D(1,0),FVector2D(1,1),FVector2D(-1,1)})
            {
                FVector Q=FVector::ZeroVector; Q[Axis]=Corner.X*(H[Axis]-B);
                Q[U]=SU*(H[U]-(Corner.Y==0?0:B)); Q[V]=SV*(H[V]-(Corner.Y==0?B:0)); P.Add(Q);
            }
            FVector N=FVector::ZeroVector;N[U]=SU;N[V]=SV;Emit(P,N.GetSafeNormal());
        }
    }
    for (int32 X : {-1,1}) for (int32 Y : {-1,1}) for (int32 Z : {-1,1})
    {
        FVector S(X,Y,Z);
        Emit({S*FVector(H.X,H.Y-B,H.Z-B),S*FVector(H.X-B,H.Y,H.Z-B),S*FVector(H.X-B,H.Y-B,H.Z)},S.GetSafeNormal());
    }
}

void FNRMeshMaker::Cylinder(FVector Center, float Radius, float Height, int32 Material, int32 Sides, FRotator Rotation)
{
    TArray<FVector> Top,Bottom;
    for (int32 I=0;I<Sides;++I)
    {
        float A=2*PI*I/Sides, B=2*PI*(I+1)/Sides;
        FVector A0(Radius*FMath::Cos(A),Radius*FMath::Sin(A),-Height*.5f), A1=A0+FVector(0,0,Height);
        FVector B0(Radius*FMath::Cos(B),Radius*FMath::Sin(B),-Height*.5f), B1=B0+FVector(0,0,Height);
        auto T=[&](FVector V){return Center+Rotation.RotateVector(V);};
        Face({T(A0),T(B0),T(B1),T(A1)},Rotation.RotateVector(FVector(FMath::Cos((A+B)*.5f),FMath::Sin((A+B)*.5f),0)),Material);
        Bottom.Add(T(A0));Top.Add(T(A1));
    }
    Face(Top,Rotation.RotateVector(FVector::UpVector),Material);
    Face(Bottom,Rotation.RotateVector(-FVector::UpVector),Material);
}

void FNRMeshMaker::Sphere(FVector Center,FVector Radius,int32 Material,int32 Sides,int32 Rings)
{
    const int32 FirstInstance=Mesh.VertexInstances().Num();
    auto Point=[&](int32 I,int32 J){float P=PI*I/Rings,T=2*PI*J/Sides;return FVector(FMath::Sin(P)*FMath::Cos(T),FMath::Sin(P)*FMath::Sin(T),FMath::Cos(P));};
    for (int32 I=0;I<Rings;++I)for(int32 J=0;J<Sides;++J)
    {
        FVector A=Point(I,J), B=Point(I+1,J), C=Point(I+1,J+1), D=Point(I,J+1);
        TArray<FVector> P=I==0?TArray<FVector>{Center+A*Radius,Center+B*Radius,Center+C*Radius}:I==Rings-1?TArray<FVector>{Center+A*Radius,Center+B*Radius,Center+D*Radius}:TArray<FVector>{Center+A*Radius,Center+B*Radius,Center+C*Radius,Center+D*Radius};
        Face(P,(A+B+C+D).GetSafeNormal(),Material);

    }
    FStaticMeshAttributes Attributes(Mesh);
    for(FVertexInstanceID V:Mesh.VertexInstances().GetElementIDs())if(V.GetValue()>=FirstInstance)
    {
        const FVector P=AuthoringTransform.InverseTransformPosition(FVector(Attributes.GetVertexPositions()[Mesh.GetVertexInstanceVertex(V)]))-Center;
        const FVector N=AuthoringTransform.TransformVectorNoScale((P/(Radius*Radius)).GetSafeNormal());
        Attributes.GetVertexInstanceNormals()[V]=FVector3f(N);
        Attributes.GetVertexInstanceTangents()[V]=FVector3f(FVector::CrossProduct(FMath::Abs(N.Z)<.95?FVector::UpVector:FVector::RightVector,N).GetSafeNormal());
    }
}

void FNRMeshMaker::SmoothFace(const TArray<FVector>& Points,const TArray<FVector>& Normals,int32 Material)
{
 if(Points.Num()<3||Points.Num()!=Normals.Num())return;
 TArray<int32> Order;for(int I=0;I<Points.Num();++I)Order.Add(I);
 if(FVector::DotProduct(FVector::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]),Normals[0])<0)Algo::Reverse(Order);
 FStaticMeshAttributes A(Mesh);TArray<FVertexInstanceID> Instances;
 for(int I:Order)
 {
  const FVector P=AuthoringTransform.TransformPosition(Points[I]);const FVector N=AuthoringTransform.TransformVectorNoScale(Normals[I]).GetSafeNormal();
  const FVector Tangent=FVector::CrossProduct(FMath::Abs(N.Z)<.95?FVector::UpVector:FVector::RightVector,N).GetSafeNormal();
  auto V=Mesh.CreateVertex();A.GetVertexPositions()[V]=FVector3f(P);auto VI=Mesh.CreateVertexInstance(V);
  A.GetVertexInstanceNormals()[VI]=FVector3f(N);A.GetVertexInstanceTangents()[VI]=FVector3f(Tangent);A.GetVertexInstanceBinormalSigns()[VI]=1;
  A.GetVertexInstanceColors()[VI]=FVector4f(1,1,1,1);A.GetVertexInstanceUVs().Set(VI,0,FVector2f(P.X/50,P.Z/50));Instances.Add(VI);
 }
 Mesh.CreatePolygon(Groups[Material],Instances);
}
void FNRMeshMaker::Shell(FVector Center,const TArray<FVector>& Sections,float StartDegrees,float EndDegrees,float Thickness,int32 Surface,int32 Edge,int32 Sides)
{
 if(Sections.Num()<2)return;
 auto P=[&](int I,int J,float Inset){const float A=FMath::DegreesToRadians(FMath::Lerp(StartDegrees,EndDegrees,float(J)/Sides));const auto S=Sections[I];return Center+FVector((S.X-Inset)*FMath::Sin(A),(S.Y-Inset)*FMath::Cos(A),S.Z);};
 auto N=[&](int I,int J){const int A=FMath::Max(0,I-1),B=FMath::Min(Sections.Num()-1,I+1);const FVector Dz=P(B,J,0)-P(A,J,0);const float T=FMath::DegreesToRadians(FMath::Lerp(StartDegrees,EndDegrees,float(J)/Sides));const FVector Dt(Sections[I].X*FMath::Cos(T),-Sections[I].Y*FMath::Sin(T),0);return FVector::CrossProduct(Dz,Dt).GetSafeNormal();};
 for(int I=0;I<Sections.Num()-1;++I)for(int J=0;J<Sides;++J)
 {
  const int Mat=(I==0||I==Sections.Num()-2)?Edge:Surface;
  SmoothFace({P(I,J,0),P(I,J+1,0),P(I+1,J+1,0),P(I+1,J,0)},{N(I,J),N(I,J+1),N(I+1,J+1),N(I+1,J)},Mat);
  SmoothFace({P(I,J,Thickness),P(I+1,J,Thickness),P(I+1,J+1,Thickness),P(I,J+1,Thickness)},{-N(I,J),-N(I+1,J),-N(I+1,J+1),-N(I,J+1)},Surface);
 }
 for(int J=0;J<Sides;++J){Face({P(0,J,0),P(0,J,Thickness),P(0,J+1,Thickness),P(0,J+1,0)},-FVector::UpVector,Edge);const int I=Sections.Num()-1;Face({P(I,J,0),P(I,J+1,0),P(I,J+1,Thickness),P(I,J,Thickness)},FVector::UpVector,Edge);}
 for(int I=0;I<Sections.Num()-1;++I)for(int J:{0,Sides}){const FVector Side=(P(I,J,0)-P(I,J==0?1:Sides-1,0)).GetSafeNormal();Face({P(I,J,0),P(I+1,J,0),P(I+1,J,Thickness),P(I,J,Thickness)},Side,Edge);}
}
void FNRMeshMaker::Cable(const TArray<FVector>& Points,float Radius,int32 Material,int32 Sides)
{
 for(int I=0;I<Points.Num()-1;++I){const FVector D=Points[I+1]-Points[I];Cylinder((Points[I]+Points[I+1])*.5f,Radius,D.Size(),Material,Sides,FQuat::FindBetweenNormals(FVector::UpVector,D.GetSafeNormal()).Rotator());}
}

UStaticMesh* FNRMeshMaker::Save(const TCHAR* Name)
{
    if(FString(Name).StartsWith(TEXT("SM_Breacher"))||FString(Name).StartsWith(TEXT("SM_MH_")))
    {
        FStaticMeshAttributes A(Mesh);FString Obj=TEXT("# Null Route authored hard-surface mesh. Centimeters, Z up.\n");
        for(auto V:Mesh.Vertices().GetElementIDs()){const auto P=A.GetVertexPositions()[V];Obj+=FString::Printf(TEXT("v %.5f %.5f %.5f\n"),P.X,P.Y,P.Z);}
        for(auto V:Mesh.VertexInstances().GetElementIDs()){const auto UV=A.GetVertexInstanceUVs().Get(V,0);Obj+=FString::Printf(TEXT("vt %.6f %.6f\n"),UV.X,1-UV.Y);}
        for(auto Triangle:Mesh.Triangles().GetElementIDs())
        {
            auto V=Mesh.GetTriangleVertexInstances(Triangle);Obj+=FString::Printf(TEXT("usemtl Surface_%d\nf %d/%d %d/%d %d/%d\n"),Mesh.GetTrianglePolygonGroup(Triangle).GetValue(),Mesh.GetVertexInstanceVertex(V[0]).GetValue()+1,V[0].GetValue()+1,Mesh.GetVertexInstanceVertex(V[2]).GetValue()+1,V[2].GetValue()+1,Mesh.GetVertexInstanceVertex(V[1]).GetValue()+1,V[1].GetValue()+1);
        }
        FFileHelper::SaveStringToFile(Obj,*(FPaths::ProjectDir()/TEXT("ArtSource/Breacher/Models")/(FString(Name)+TEXT(".obj"))));
    }
    const FString Path=FString(TEXT("/Game/Art/Meshes/"))+Name;
    auto* Asset=NewObject<UStaticMesh>(CreatePackage(*Path),Name,RF_Public|RF_Standalone);
    for(int32 I=0;I<Materials.Num();++I)Asset->GetStaticMaterials().Add(FStaticMaterial(Materials[I],FName(*FString::Printf(TEXT("Surface_%d"),I))));
    Asset->SetNumSourceModels(1);
    Asset->GetSourceModel(0).BuildSettings.bRecomputeNormals=false;
    Asset->GetSourceModel(0).BuildSettings.bRecomputeTangents=false;
    Asset->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs=false;
    Asset->CreateMeshDescription(0,MoveTemp(Mesh));
    Asset->CommitMeshDescription(0);
    Asset->Build(false);
    Asset->CreateBodySetup();
    Asset->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
    Asset->PostEditChange();
    NRSaveAsset(Asset);
    return Asset;
}

void FNRMeshMaker::Plate(FVector C,const TArray<FVector2D>& P,float D,int32 Surface,int32 Rim,float B)
{
 TArray<FVector> Back,Front,Inset;
 for(auto V:P){Back.Add(C+FVector(V.X,-D*.5,V.Y));Front.Add(C+FVector(V.X,D*.5-B,V.Y));Inset.Add(C+FVector(V.X*.89,D*.5,V.Y*.89));}
 Face(Back,FVector(0,-1,0),Surface);Face(Inset,FVector(0,1,0),Surface);
 for(int I=0;I<P.Num();++I){int J=(I+1)%P.Num();auto N=FVector(P[J].Y-P[I].Y,0,P[I].X-P[J].X).GetSafeNormal();Face({Back[I],Back[J],Front[J],Front[I]},N,Surface);Face({Front[I],Front[J],Inset[J],Inset[I]},(N+FVector(0,1,0)).GetSafeNormal(),Rim);}
}
void FNRMeshMaker::TexturedFace(const TArray<FVector>& P,const TArray<FVector2D>& UV,int32 Material)
{
 // Every badge triangle has explicitly authored UVs into an unmodified reference photograph.
 const int32 First=Mesh.VertexInstances().Num();Face(P,FVector(0,1,0),Material);
 FStaticMeshAttributes A(Mesh);
 for(auto V:Mesh.VertexInstances().GetElementIDs())if(V.GetValue()>=First)
 {
  const FVector Pos(A.GetVertexPositions()[Mesh.GetVertexInstanceVertex(V)]);
  for(int I=0;I<P.Num();++I)if(Pos.Equals(AuthoringTransform.TransformPosition(P[I]),.001)){A.GetVertexInstanceUVs().Set(V,0,FVector2f(UV[I]));break;}
 }
}

void FNRMeshMaker::Sleeve(int32 Material)
{
 const float Z[]={0,18,38,55,72,86,94,100};const float R[]={5.2,5.1,4.8,4.55,4.15,3.65,3.25,3.1};
 const int Sides=32;
 for(int Ring=0;Ring<7;++Ring)for(int I=0;I<Sides;++I)
 {
  float A=I*2*PI/Sides,B=(I+1)*2*PI/Sides;
  auto V=[&](float T,int J){float Radius=R[J]*(1+.025f*FMath::Sin(T*5+J*2));return FVector(Radius*FMath::Cos(T),Radius*.87f*FMath::Sin(T),Z[J]);};
  Face({V(A,Ring),V(B,Ring),V(B,Ring+1),V(A,Ring+1)},FVector(FMath::Cos((A+B)/2),FMath::Sin((A+B)/2),.018),Material);
 }
 TArray<FVector> Bottom,Top;for(int I=0;I<Sides;++I){float A=I*2*PI/Sides;Bottom.Add({R[0]*FMath::Cos(A),R[0]*.87f*FMath::Sin(A),0});Top.Add({R[7]*FMath::Cos(A),R[7]*.87f*FMath::Sin(A),100});}
 Face(Bottom,-FVector::UpVector,Material);Face(Top,FVector::UpVector,Material);
}

void FNRMeshMaker::Blade(int32 M)
{
 TArray<FVector> Outline{{0,8,-2.6},{0,24,-2.6},{0,36,0},{0,25,3.4},{0,8,3.4}};
 for(int S:{-1,1})
 {
  FVector Ridge(S*.65,19,.4);
  for(int I=0;I<Outline.Num();++I){FVector A=Outline[I],B=Outline[(I+1)%Outline.Num()];Face({A,B,Ridge},FVector(S,0,0),M);}
 }
}
