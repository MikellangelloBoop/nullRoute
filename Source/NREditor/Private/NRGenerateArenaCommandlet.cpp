#include "NRGenerateArenaCommandlet.h"
#include "NRNode.h"
#include "NRObjective.h"
#include "ActorFactories/ActorFactory.h"
#include "NRStructure.h"
#include "NRDroneDirector.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Factories/WorldFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#include "GeometryCollection/GeometryCollectionClusteringUtility.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "Builders/CubeBuilder.h"
#include "Editor.h"

UNRGenerateArenaCommandlet::UNRGenerateArenaCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
static bool SaveNR(UPackage* P,UObject* Asset,bool bMap=false){FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;A.SaveFlags=SAVE_None;P->MarkAsFullyLoaded();FAssetRegistryModule::AssetCreated(Asset);P->MarkPackageDirty();const FString Path=FPackageName::LongPackageNameToFilename(P->GetName(),bMap?FPackageName::GetMapPackageExtension():FPackageName::GetAssetPackageExtension());const bool OK=UPackage::SavePackage(P,Asset,*Path,A);if(!OK)UE_LOG(LogTemp,Error,TEXT("Could not save generated asset: %s"),*Path);return OK;}
int32 UNRGenerateArenaCommandlet::Main(const FString& Params){
 auto Mat=[](const TCHAR* Name,FLinearColor Color,bool Emissive){const FString Path=FString(TEXT("/Game/Materials/"))+Name;UPackage* P=CreatePackage(*Path);auto* M=NewObject<UMaterial>(P,Name,RF_Public|RF_Standalone);auto* V=NewObject<UMaterialExpressionVectorParameter>(M);V->ParameterName=TEXT("Tint");V->DefaultValue=Color;M->GetExpressionCollection().AddExpression(V);M->GetEditorOnlyData()->BaseColor.Expression=V;if(Emissive)M->GetEditorOnlyData()->EmissiveColor.Expression=V;M->GetEditorOnlyData()->Roughness.Constant=.7f;M->GetEditorOnlyData()->Roughness.UseConstant=true;M->PostEditChange();SaveNR(P,M);return M;};
 UMaterial* Floor=Mat(TEXT("M_Floor"),FLinearColor(.065,.08,.11),false);
 UMaterial* Wall=Mat(TEXT("M_Wall"),FLinearColor(.18,.23,.28),false);
 UMaterial* Cyan=Mat(TEXT("M_Cyan"),FLinearColor(.04,2.5,3.5),true);
 UMaterial* Orange=Mat(TEXT("M_Orange"),FLinearColor(3.5,.6,.04),true);
 UMaterial* NodeMat=Mat(TEXT("M_Node"),FLinearColor(.06,.75,1),true);

 // GPU material evaluates a signed distance to the cable center line.
 {UPackage* P=CreatePackage(TEXT("/Game/Materials/M_SDFLine"));auto* M=NewObject<UMaterial>(P,TEXT("M_SDFLine"),RF_Public|RF_Standalone);M->BlendMode=BLEND_Translucent;M->SetShadingModel(MSM_Unlit);M->TwoSided=true;
 auto* UV=NewObject<UMaterialExpressionTextureCoordinate>(M);auto* SDF=NewObject<UMaterialExpressionCustom>(M);SDF->Code=TEXT("float d=abs(UV.y-0.5)-0.10; return 1.0-smoothstep(-fwidth(d),fwidth(d),d);");SDF->OutputType=CMOT_Float1;FCustomInput Input;Input.InputName=TEXT("UV");Input.Input.Expression=UV;SDF->Inputs.Add(Input);auto* Color=NewObject<UMaterialExpressionConstant3Vector>(M);Color->Constant=FLinearColor(.1f,3.f,2.f);M->GetExpressionCollection().AddExpression(UV);M->GetExpressionCollection().AddExpression(SDF);M->GetExpressionCollection().AddExpression(Color);M->GetEditorOnlyData()->Opacity.Expression=SDF;M->GetEditorOnlyData()->EmissiveColor.Expression=Color;M->PostEditChange();SaveNR(P,M);}
 {UPackage* P=CreatePackage(TEXT("/Game/Materials/M_Visor"));auto* M=NewObject<UMaterial>(P,TEXT("M_Visor"),RF_Public|RF_Standalone);M->MaterialDomain=MD_PostProcess;
 auto* Scene=NewObject<UMaterialExpressionSceneTexture>(M);Scene->SceneTextureId=PPI_PostProcessInput0;
 auto* Stencil=NewObject<UMaterialExpressionSceneTexture>(M);Stencil->SceneTextureId=PPI_CustomStencil;
 auto* Depth=NewObject<UMaterialExpressionSceneTexture>(M);Depth->SceneTextureId=PPI_SceneDepth;
 auto* CustomDepth=NewObject<UMaterialExpressionSceneTexture>(M);CustomDepth->SceneTextureId=PPI_CustomDepth;
 auto* Mix=NewObject<UMaterialExpressionCustom>(M);Mix->OutputType=CMOT_Float3;Mix->Code=TEXT("float enemy=1-step(0.5,abs(S.r-2));float weak=1-step(0.5,abs(S.r-5));float hidden=step(D.r+1,C.r);float3 tint=enemy*float3(1,0.08,0.04)+weak*float3(0.02,0.8,1);return lerp(Scene.rgb,tint,saturate(enemy+weak)*(0.2+0.35*hidden));");
 auto Add=[&](const TCHAR* Name,UMaterialExpression* Expression){FCustomInput I;I.InputName=Name;I.Input.Expression=Expression;Mix->Inputs.Add(I);M->GetExpressionCollection().AddExpression(Expression);};Add(TEXT("Scene"),Scene);Add(TEXT("S"),Stencil);Add(TEXT("D"),Depth);Add(TEXT("C"),CustomDepth);M->GetExpressionCollection().AddExpression(Mix);M->GetEditorOnlyData()->EmissiveColor.Expression=Mix;M->PostEditChange();SaveNR(P,M);}

 {UPackage* P=CreatePackage(TEXT("/Game/Materials/M_Polymer"));auto* M=NewObject<UMaterial>(P,TEXT("M_Polymer"),RF_Public|RF_Standalone);M->BlendMode=BLEND_Translucent;M->TwoSided=true;auto* Tint=NewObject<UMaterialExpressionVectorParameter>(M);Tint->ParameterName=TEXT("Tint");Tint->DefaultValue=FLinearColor(.1f,.8f,1.f);auto* Opacity=NewObject<UMaterialExpressionScalarParameter>(M);Opacity->ParameterName=TEXT("Opacity");Opacity->DefaultValue=.3f;M->GetExpressionCollection().AddExpression(Tint);M->GetExpressionCollection().AddExpression(Opacity);M->GetEditorOnlyData()->BaseColor.Expression=Tint;M->GetEditorOnlyData()->EmissiveColor.Expression=Tint;M->GetEditorOnlyData()->Opacity.Expression=Opacity;M->PostEditChange();SaveNR(P,M);}
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 UPackage* CP=CreatePackage(TEXT("/Game/Structures/GC_Panel"));auto* Collection=NewObject<UGeometryCollection>(CP,TEXT("GC_Panel"),RF_Public|RF_Standalone);
 TArray<UMaterialInterface*> Materials{Wall};
 for(int32 X=0;X<4;++X)for(int32 Y=0;Y<2;++Y)FGeometryCollectionEngineConversion::AppendStaticMesh(Cube,Materials,FTransform(FRotator::ZeroRotator,FVector((X-1.5f)*150,(Y-.5f)*150,0),FVector(1.5,1.5,.25)),Collection,false);
 auto GC=Collection->GetGeometryCollection();GC->ReindexMaterials();FGeometryCollectionClusteringUtility::ClusterAllBonesUnderNewRoot(GC.Get());Collection->EnableClustering=true;Collection->DamageThreshold={500.f};for(auto& Size:Collection->SizeSpecificData)for(auto& Shape:Size.CollisionShapes)Shape.ImplicitType=EImplicitTypeEnum::Chaos_Implicit_Box;Collection->InvalidateCollection();Collection->CreateSimulationData();SaveNR(CP,Collection);
 UPackage* WP=CreatePackage(TEXT("/Game/Maps/NR_Arcology"));auto* Factory=NewObject<UWorldFactory>();auto* W=Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(),WP,TEXT("NR_Arcology"),RF_Public|RF_Standalone,nullptr,GWarn));if(!W)return 1;
 auto Box=[&](FVector P,FVector Scale,UMaterialInterface* M,FRotator R=FRotator::ZeroRotator){auto* A=W->SpawnActor<AStaticMeshActor>(P,R);A->GetStaticMeshComponent()->SetStaticMesh(Cube);A->SetActorScale3D(Scale);A->GetStaticMeshComponent()->SetMaterial(0,M);A->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));return A;};
 Box(FVector(0,0,-60),FVector(50,40,1),Floor);
 Box(FVector(0,-2000,350),FVector(50,.5,8),Wall);Box(FVector(0,2000,350),FVector(50,.5,8),Wall);Box(FVector(-2500,0,350),FVector(.5,40,8),Wall);Box(FVector(2500,0,350),FVector(.5,40,8),Wall);
 for(int32 X=-2;X<=2;++X)for(int32 Y=-1;Y<=1;++Y){if(X==0&&Y==0)continue;FVector P(X*850,Y*1200,220);Box(P,FVector(.65,.65,5),Wall);Box(P+FVector(36,0,0),FVector(.035,.2,4),X<0?Orange:Cyan);}
 for(int32 I=0;I<6;++I){Box(FVector(-1600+I*650,1500,1),FVector(3,.15,.025),Cyan);Box(FVector(-1600+I*650,-1500,1),FVector(3,.15,.025),Orange);}
 Box(FVector(0,0,400),FVector(8,8,.3),Floor);Box(FVector(-850,0,400),FVector(5,8,.3),Floor);Box(FVector(850,0,400),FVector(5,8,.3),Floor);
 for(int32 I=0;I<12;++I){Box(FVector(-1250-I*65,-600,17+I*32),FVector(.65,2,.32),Wall);Box(FVector(1250+I*65,600,17+I*32),FVector(.65,2,.32),Wall);}
 for(int32 Side=-1;Side<=1;Side+=2){for(int32 I=0;I<3;++I){auto* S=W->SpawnActor<ANRStructure>(FVector(Side*550,(I-1)*320,400),FRotator::ZeroRotator);S->GetGeometryCollectionComponent()->SetRestCollection(Collection);S->bFloor=true;}
 auto* WallPanel=W->SpawnActor<ANRStructure>(FVector(Side*1100,0,170),FRotator(90,0,0));WallPanel->GetGeometryCollectionComponent()->SetRestCollection(Collection);}
 for(int32 I=0;I<14;++I){const float X=(I%7-3)*550.f;const float Y=I<7?-950:950;Box(FVector(X,Y,65),FVector(1.5,1.2,1.3),Wall);}
 auto* Disk=W->SpawnActor<ANRObjective>(FVector(1850,0,85),FRotator::ZeroRotator);Disk->Mesh->SetMaterial(0,Cyan);Disk->SetActorLabel(TEXT("ELARA DISK // E to recover"));
 auto* Extract=W->SpawnActor<ANRObjective>(FVector(-2100,0,60),FRotator::ZeroRotator);Extract->bExtraction=true;Extract->Mesh->SetRelativeScale3D(FVector(1.2,2,.7));Extract->Mesh->SetMaterial(0,Orange);Extract->SetActorLabel(TEXT("SYNDICATE EXTRACTION"));
 for(int32 Team=0;Team<2;++Team){auto* Start=W->SpawnActor<APlayerStart>(FVector(Team==1?-1950:1950,-350,110),FRotator(0,Team==1?0:180,0));Start->PlayerStartTag=Team==0?TEXT("Team0"):TEXT("Team1");}
 auto* Drones=W->SpawnActor<ANRDroneDirector>(FVector(550,550,0),FRotator::ZeroRotator);Drones->SetActorLabel(TEXT("Mass drone wave"));
 auto Label=[&](FVector P,FRotator R,const TCHAR* Text,FColor Color){auto* T=W->SpawnActor<ATextRenderActor>(P,R);T->GetTextRender()->SetText(FText::FromString(Text));T->GetTextRender()->SetWorldSize(55);T->GetTextRender()->SetTextRenderColor(Color);T->GetTextRender()->SetHorizontalAlignment(EHTA_Center);};
 Label(FVector(2400,0,520),FRotator(0,180,0),TEXT("ECHELON  /  ELARA CORE"),FColor(90,220,255));Label(FVector(-2400,0,520),FRotator::ZeroRotator,TEXT("SYNDICATE  /  NULL ROUTE"),FColor(255,140,60));
 auto* Sun=W->SpawnActor<ADirectionalLight>(FVector(0,0,1200),FRotator(-65,-30,0));Sun->GetLightComponent()->SetIntensity(3.5f);Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 for(int32 X=-1;X<=1;++X){auto* L=W->SpawnActor<APointLight>(FVector(X*1500,0,850),FRotator::ZeroRotator);L->PointLightComponent->SetIntensity(180000);L->PointLightComponent->SetAttenuationRadius(2200);L->PointLightComponent->SetMobility(EComponentMobility::Movable);}
 auto* Bounds=W->SpawnActor<ANavMeshBoundsVolume>(FVector(0,0,300),FRotator::ZeroRotator);auto* Builder=NewObject<UCubeBuilder>(Bounds);Builder->X=5200;Builder->Y=4200;Builder->Z=1000;UActorFactory::CreateBrushForVolumeActor(Bounds,Builder);
 W->SpawnActor<ARecastNavMesh>(); // RuntimeGeneration=Dynamic comes from DefaultEngine.ini.
 if(!SaveNR(WP,W,true))return 2;UE_LOG(LogTemp,Display,TEXT("NULL ROUTE ARENA GENERATED"));return 0;
}



