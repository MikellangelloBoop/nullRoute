#include "NRGenerateArtCommandlet.h"
#include "NRArtAssets.h"
#include "NRMeshMaker.h"
#include "NRObjective.h"
#include "NRNode.h"
#include "NRStructure.h"
#include "NRDroneDirector.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/RectLight.h"
#include "Components/RectLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/TextureCube.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Factories/WorldFactory.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "Builders/CubeBuilder.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#include "GeometryCollection/GeometryCollectionClusteringUtility.h"

UNRGenerateArtCommandlet::UNRGenerateArtCommandlet()
{
    IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;
}

int32 UNRGenerateArtCommandlet::Main(const FString& Params)
{
    FNRArtAssets Art;Art.Build();
    if(Params.Contains(TEXT("AssetsOnly")))return 0;
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Factory=NewObject<UWorldFactory>();
    auto* W=Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(),CreatePackage(TEXT("/Game/Maps/NR_Arcology")),TEXT("NR_Arcology"),RF_Public|RF_Standalone,nullptr,GWarn));
    if(!W)return 1;

    auto Place=[&](UStaticMesh* Mesh,FVector Location,FVector Scale,FRotator Rotation=FRotator::ZeroRotator,bool Collision=true,UMaterialInterface* Material=nullptr)
    {
        auto* A=W->SpawnActor<AStaticMeshActor>(Location,Rotation);
        auto* C=A->GetStaticMeshComponent();C->SetStaticMesh(Mesh);A->SetActorScale3D(Scale);
        C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));
        C->SetCanEverAffectNavigation(Collision);
        if(Material)C->SetMaterial(0,Material);
        return A;
    };
    auto Box=[&](FVector Center,FVector Dimensions,UMaterialInterface* Material,bool Collision=true,FRotator Rotation=FRotator::ZeroRotator)
    {return Place(Cube,Center,Dimensions/100,Rotation,Collision,Material);};
    auto Text=[&](FVector Location,FRotator Rotation,const FString& Message,float Size,FColor Color)
    {
        auto* A=W->SpawnActor<ATextRenderActor>(Location,Rotation);auto* C=A->GetTextRender();
        C->SetText(FText::FromString(Message));C->SetWorldSize(Size);C->SetTextRenderColor(Color);
        C->SetHorizontalAlignment(EHTA_Center);C->SetVerticalAlignment(EVRTA_TextCenter);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    };
    auto Instances=[&](UStaticMesh* Mesh,const TArray<FTransform>& Transforms,bool Collision)
    {
        auto* A=W->SpawnActor<AActor>();auto* C=NewObject<UInstancedStaticMeshComponent>(A,TEXT("Instances"));
        A->SetRootComponent(C);A->AddInstanceComponent(C);C->SetStaticMesh(Mesh);
        C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));C->SetCanEverAffectNavigation(Collision);
        C->RegisterComponent();C->AddInstances(Transforms,false,true);return A;
    };

    // A continuous collision slab keeps floor seams from disturbing movement or navigation.
    Box({0,0,-40},{5000,4000,60},Art.Metal);
    TArray<FTransform> Tiles;
    for(int32 X=-6;X<=6;++X)for(int32 Y=-4;Y<=4;++Y)Tiles.Add(FTransform(FVector(X*400,Y*400,0)));
    Instances(Art.Panel,Tiles,false);
    Box({0,-1900,-5},{5000,200,10},Art.Floor);Box({0,1900,-5},{5000,200,10},Art.Floor);

    for(int32 Side:{-1,1})
    {
        Box({0,Side*2000.,470},{5000,90,1000},Art.Concrete);
        Box({0,Side*1949.,115},{5000,15,230},Art.Metal);
        Box({0,Side*1939.,241},{5000,8,7},Art.White,false);
        Box({0,Side*1935.,250},{5000,6,3},Art.Cyan,false);
        Box({Side*2500.,0,470},{90,4000,1000},Art.Concrete);
        Box({Side*2449.,0,115},{15,4000,230},Art.Metal);
        // Modulated wall ribs and recessed panels establish a coherent architectural scale.
        for(int32 X=-5;X<=5;++X)
        {
            Box({X*440.,Side*1940.,480},{16,50,470},Art.White);
            Box({X*440.+220,Side*1943.,480},{407,10,425},Art.Floor,false);
            Box({X*440.+220,Side*1934.,650},{330,6,5},Side<0?Art.Amber:Art.Cyan,false);
        }
        for(int32 X=-2;X<=2;++X)
        {
            FVector P(X*850,Side*1400,350);
            Box(P,{78,100,720},Art.Concrete);
            Box(P+FVector(-44,0,0),{12,110,710},Art.Metal);
            Box(P+FVector(-52,0,10),{5,16,510},X<0?Art.Amber:Art.Cyan,false);
            Box(P+FVector(0,0,-328),{108,132,42},Art.Metal);
        }
    }
    // Broad overhead trusses and a daylight canopy. No expensive runtime Blueprint construction.
    for(int32 X=-2;X<=2;++X)
    {
        Box({X*1000.,0,930},{100,4100,100},Art.Metal);
        Box({X*1000.,0,875},{110,4000,12},Art.White,false);
        for(int32 Y:{-1,1})Box({X*1000.,Y*1200.,863},{78,500,6},Art.Cyan,false);
    }
    for(int32 Y:{-1,1})Box({0,Y*1780.,960},{5000,160,140},Art.White);
    for(int32 I=-4;I<=4;++I)Box({I*550.,0,1080},{12,4000,25},Art.Metal,false);

    // The mezzanine contains real holes: no hidden static slab under the Chaos panels.
    for(int32 Side:{-1,1})
    {
        Box({0,Side*830.,325},{600,760,30},Art.Floor);
        Box({-460,Side*1150.,325},{480,240,30},Art.Floor);
        for(int32 I=0;I<12;++I)
        {
            float Height=(I+1)*(340.f/12);
            Box({-1510+I*75.,Side*1150.,Height*.5f},{75,220,Height},Art.Concrete);
            Box({-1510+I*75.+32,Side*1150.,Height+1},{7,214,2},Art.White,false);
        }
        for(int32 X:{-1,1})
        {
            Box({X*275.,Side*1000.,165},{35,35,320},Art.Metal);
            for(int32 Y=0;Y<4;++Y)Box({X*294.,Side*(510.+Y*180),392},{6,6,110},Art.Metal);
            Box({X*294.,Side*780.,445},{8,650,8},Art.White);
            Box({X*294.,Side*780.,383},{5,650,5},Art.Metal);
        }
    }
    auto* Collection=NewObject<UGeometryCollection>(CreatePackage(TEXT("/Game/Structures/GC_Panel")),TEXT("GC_Panel"),RF_Public|RF_Standalone);
    TArray<UMaterialInterface*> CollectionMaterials{Art.Concrete};
    for(int32 X=0;X<4;++X)for(int32 Y=0;Y<2;++Y)
        FGeometryCollectionEngineConversion::AppendStaticMesh(Cube,CollectionMaterials,FTransform(FRotator::ZeroRotator,FVector((X-1.5)*150,(Y-.5)*150,0),FVector(1.5,1.5,.25)),Collection,false);
    auto GC=Collection->GetGeometryCollection();GC->ReindexMaterials();FGeometryCollectionClusteringUtility::ClusterAllBonesUnderNewRoot(GC.Get());
    Collection->EnableClustering=true;Collection->DamageThreshold={500.f};
    for(auto& Size:Collection->SizeSpecificData)for(auto& Shape:Size.CollisionShapes)Shape.ImplicitType=EImplicitTypeEnum::Chaos_Implicit_Box;
    Collection->InvalidateCollection();Collection->CreateSimulationData();NRSaveAsset(Collection);
    for(int32 I=-1;I<=1;++I)
    {
        auto* S=W->SpawnActor<ANRStructure>(FVector(0,I*300,327.5),FRotator::ZeroRotator);
        S->GetGeometryCollectionComponent()->SetRestCollection(Collection);S->bFloor=true;
        S->SetActorLabel(FString::Printf(TEXT("BREACHABLE MEZZANINE %d"),I+2));
    }
    for(int32 Side:{-1,1})
    {
        auto* S=W->SpawnActor<ANRStructure>(FVector(Side*1120,0,160),FRotator(90,0,0));
        S->GetGeometryCollectionComponent()->SetRestCollection(Collection);
        Box({Side*1120.,-175,170},{65,40,360},Art.Metal);Box({Side*1120.,175,170},{65,40,360},Art.Metal);
        Text({Side*1120.-30,-.1,360},{0,180,0},TEXT("STRUCTURAL / 05"),24,FColor(100,220,235));
    }

    TArray<FTransform> Racks;
    for(int32 Side:{-1,1})for(int32 Row=0;Row<4;++Row)for(int32 I=0;I<3;++I)
        Racks.Add(FTransform(FRotator(0,Side<0?0:180,0),FVector(850+Row*270,Side*(760+I*120),0)));
    Instances(Art.Rack,Racks,true);
    TArray<FVector> Covers={{-1500,-650,0},{-1150,700,0},{-600,-1050,0},{-650,900,0},{650,-450,0},{750,450,0},{1520,-330,0},{1500,420,0},{1850,1500,0},{-1900,1400,0}};
    for(int32 I=0;I<Covers.Num();++I)Place(Art.Crate,Covers[I],FVector(1),FRotator(0,I%2?90:0,0));
    // Arrival and vault gates frame the sightline and make navigation legible.
    for(int32 Side:{-1,1})
    {
        auto* Accent=Side<0?Art.Amber:Art.Cyan;
        for(int32 Y:{-1,1})
        {
            Box({Side*2190.,Y*390.,310},{180,110,650},Art.Metal);
            Box({Side*2089.,Y*390.,310},{10,62,610},Art.White,false);
            Box({Side*2080.,Y*348.,310},{8,8,610},Accent,false);
        }
        Box({Side*2190.,0,637},{180,885,70},Art.Metal);
        Box({Side*2089.,0,637},{9,820,30},Accent,false);
        Text({Side<0?-2390.:2390.,0,780},FRotator(0,Side<0?0:180,0),Side<0?TEXT("NULL / ROUTE"):TEXT("ECHELON"),105,FColor::White);
        Text({Side<0?-2390.:2390.,0,670},FRotator(0,Side<0?0:180,0),Side<0?TEXT("01    SYNDICATE ENTRY"):TEXT("07    ELARA ARCHIVE"),30,Side<0?FColor(255,175,65):FColor(80,220,255));
        for(int32 Y:{-1,1})for(int32 I=0;I<9;++I)
            Box({-2000+I*500.,Y*1600.,1},{220,5,2},Y<0?Art.Amber:Art.Cyan,false);
    }
    Box({1910,0,30},{210,210,60},Art.Metal);
    Box({1910,0,64},{190,190,8},Art.White);
    auto* Disk=W->SpawnActor<ANRObjective>(FVector(1910,0,108),FRotator::ZeroRotator);
    Disk->Mesh->SetStaticMesh(Art.Core);Disk->Mesh->SetRelativeScale3D(FVector(1));Disk->SetActorLabel(TEXT("ELARA // RECOVER"));
    auto* Exit=W->SpawnActor<ANRObjective>(FVector(-2110,0,15),FRotator::ZeroRotator);
    Exit->bExtraction=true;Exit->Mesh->SetStaticMesh(Art.Extraction);Exit->Mesh->SetRelativeScale3D(FVector(1));Exit->SetActorLabel(TEXT("SYNDICATE // EXTRACT"));
    Text({1900,0,300},{0,180,0},TEXT("E L A R A\nSECURE MEMORY CORE"),27,FColor(70,220,250));
    Text({-2160,0,250},{0,0,0},TEXT("UPLINK / EXTRACTION"),25,FColor(255,160,55));
    for(int32 Team=0;Team<2;++Team)for(int32 Seat=0;Seat<5;++Seat)
    {
        auto* Start=W->SpawnActor<APlayerStart>(FVector(Team==1?-1940:2090,-430+Seat*210,100),FRotator(0,Team==1?0:180,0));
        Start->PlayerStartTag=Team==0?TEXT("Team0"):TEXT("Team1");
    }
    W->SpawnActor<ANRDroneDirector>(FVector(650,200,0),FRotator::ZeroRotator);

    auto* Sun=W->SpawnActor<ADirectionalLight>(FVector(0,0,2000),FRotator(-48,-35,0));
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sun->GetLightComponent()->SetIntensity(3.5f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.88f,.73f));
    auto* Sky=W->SpawnActor<ASkyLight>();auto* SC=Sky->GetLightComponent();SC->SetMobility(EComponentMobility::Movable);
    SC->SourceType=SLS_SpecifiedCubemap;SC->Cubemap=LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));SC->SetIntensity(.85f);
    Place(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere")),FVector::ZeroVector,FVector(150),FRotator::ZeroRotator,false,LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap_Mat.DaylightAmbientCubemap_Mat")));
    for(int32 X=-1;X<=1;++X)
    {
        auto* L=W->SpawnActor<ARectLight>(FVector(X*1650,0,820),FRotator(-90,0,0));auto* C=Cast<URectLightComponent>(L->GetLightComponent());
        C->SetMobility(EComponentMobility::Movable);C->SetIntensity(3000);C->SetAttenuationRadius(1500);C->SetSourceWidth(500);C->SetSourceHeight(300);C->SetCastShadows(false);
    }
    auto* Post=W->SpawnActor<APostProcessVolume>();Post->bUnbound=true;
    Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.28f;
    Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.18f;
    Post->Settings.bOverride_MotionBlurAmount=true;Post->Settings.MotionBlurAmount=0;
    Post->Settings.bOverride_AmbientOcclusionIntensity=true;Post->Settings.AmbientOcclusionIntensity=.65f;
    Post->Settings.bOverride_AutoExposureMinBrightness=true;Post->Settings.AutoExposureMinBrightness=1;
    Post->Settings.bOverride_AutoExposureMaxBrightness=true;Post->Settings.AutoExposureMaxBrightness=1;
    auto* Bounds=W->SpawnActor<ANavMeshBoundsVolume>(FVector(0,0,400),FRotator::ZeroRotator);
    auto* Builder=NewObject<UCubeBuilder>(Bounds);Builder->X=5200;Builder->Y=4200;Builder->Z=1300;Builder->Build(W,Bounds);
    W->SpawnActor<ARecastNavMesh>();
    if(!NRSaveAsset(W,true))return 2;
    UE_LOG(LogTemp,Display,TEXT("NR_ART_BUILD_COMPLETE"));return 0;
}
