#include "NRArtAssets.h"
#include "NRMeshMaker.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Engine/Texture2D.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"

static UMaterial* Surface(const TCHAR* Name,FLinearColor Tint,float Roughness,float Metallic,const TCHAR* Texture=nullptr,bool Emissive=false)
{
    const FString Path=FString(TEXT("/Game/Art/Materials/"))+Name;
    auto* M=NewObject<UMaterial>(CreatePackage(*Path),Name,RF_Public|RF_Standalone);
    auto* Color=NewObject<UMaterialExpressionVectorParameter>(M);
    Color->ParameterName=TEXT("Tint");Color->DefaultValue=Tint;
    M->GetExpressionCollection().AddExpression(Color);
    UMaterialExpression* Base=Color;
    if(Texture)
    {
        auto* T=NewObject<UMaterialExpressionTextureObject>(M);T->Texture=LoadObject<UTexture2D>(nullptr,Texture);
        auto* P=NewObject<UMaterialExpressionWorldPosition>(M);
        auto* N=NewObject<UMaterialExpressionVertexNormalWS>(M);
        auto* Tri=NewObject<UMaterialExpressionCustom>(M);
        Tri->Code=TEXT("float3 w=pow(abs(N),8); w/=max(w.x+w.y+w.z,0.0001);float3 p=P/180.0; return Texture2DSample(T,T sampler,p.yz).rgb*w.x + Texture2DSample(T,T sampler,p.xz).rgb*w.y + Texture2DSample(T,T sampler,p.xy).rgb*w.z;");
        Tri->Code=Tri->Code.Replace(TEXT("T sampler"),TEXT("TSampler"));
        Tri->OutputType=CMOT_Float3;
        auto Input=[&](const TCHAR* Key,UMaterialExpression* E){FCustomInput I;I.InputName=Key;I.Input.Expression=E;Tri->Inputs.Add(I);M->GetExpressionCollection().AddExpression(E);};
        Input(TEXT("T"),T);Input(TEXT("P"),P);Input(TEXT("N"),N);
        M->GetExpressionCollection().AddExpression(Tri);
        auto* Multiply=NewObject<UMaterialExpressionMultiply>(M);Multiply->A.Expression=Tri;Multiply->B.Expression=Color;
        M->GetExpressionCollection().AddExpression(Multiply);Base=Multiply;
    }
    M->GetEditorOnlyData()->BaseColor.Expression=Base;
    M->GetEditorOnlyData()->Roughness.UseConstant=true;M->GetEditorOnlyData()->Roughness.Constant=Roughness;
    M->GetEditorOnlyData()->Metallic.UseConstant=true;M->GetEditorOnlyData()->Metallic.Constant=Metallic;
    if(Emissive){M->SetShadingModel(MSM_Unlit);M->GetEditorOnlyData()->EmissiveColor.Expression=Base;}
    bool Recompile=false;
    M->SetMaterialUsage(Recompile,MATUSAGE_InstancedStaticMeshes);
    if(FString(Name).Contains(TEXT("Team")))M->SetMaterialUsage(Recompile,MATUSAGE_SkeletalMesh);
    if(FString(Name).Contains(TEXT("Concrete")))M->SetMaterialUsage(Recompile,MATUSAGE_GeometryCollections);
    M->PostEditChange();NRSaveAsset(M);return M;
}

void FNRArtAssets::Build()
{
    Concrete=Surface(TEXT("M_Concrete"),FLinearColor(.72f,.78f,.8f),.78f,.05f,TEXT("/Game/Art/Textures/T_Concrete.T_Concrete"));
    Floor=Surface(TEXT("M_FloorPanel"),FLinearColor(.28f,.34f,.37f),.5f,.35f,TEXT("/Game/Art/Textures/T_Concrete.T_Concrete"));
    Metal=Surface(TEXT("M_Graphite"),FLinearColor(1.8f,2.f,2.1f),.38f,.65f,TEXT("/Game/Art/Textures/T_Graphite.T_Graphite"));
    Rubber=Surface(TEXT("M_Rubber"),FLinearColor(.018f,.023f,.029f),.85f,0);
    White=Surface(TEXT("M_Ceramic"),FLinearColor(.5f,.59f,.62f),.28f,.5f);
    Cyan=Surface(TEXT("M_CyanLight"),FLinearColor(.03f,2.5f,3.7f),.2f,0,nullptr,true);
    Amber=Surface(TEXT("M_AmberLight"),FLinearColor(3.6f,.8f,.08f),.2f,0,nullptr,true);
    Violet=Surface(TEXT("M_PhantomLight"),FLinearColor(1.6f,.06f,3.6f),.2f,0,nullptr,true);
    Screen=Surface(TEXT("M_Screen"),FLinearColor(.005f,.1f,.13f),.22f,.05f,nullptr,true);
    Tracer=Surface(TEXT("M_Tracer"),FLinearColor(12.f,5.f,.8f),.1f,0,nullptr,true);
    Impact=Surface(TEXT("M_Impact"),FLinearColor(.008f,.009f,.01f),.98f,0);
    Surface(TEXT("M_TeamSyndicate"),FLinearColor(.35f,.11f,.035f),.45f,.5f,TEXT("/Game/Art/Textures/T_Concrete.T_Concrete"));
    Surface(TEXT("M_TeamEchelon"),FLinearColor(.12f,.25f,.3f),.36f,.55f,TEXT("/Game/Art/Textures/T_Concrete.T_Concrete"));

    {
        FNRMeshMaker M({Metal,White,Amber,Rubber});
        M.Box({0,0,55},{160,90,110},0,7);
        M.Box({0,0,110},{166,96,10},1,3);
        M.Box({0,0,4},{166,96,8},3,2);
        for(int32 X:{-1,1})for(int32 Y:{-1,1})M.Box({X*72.,Y*42.,55},{16,14,106},1,3);
        for(int32 Y:{-1,1})
        {
            M.Box({0,Y*46.,63},{102,4,64},3,2);
            M.Box({0,Y*49.,78},{55,3,5},2,1);
            M.Box({0,Y*49.,42},{28,4,8},1,2);
            for(int32 I=-2;I<=2;++I)M.Box({I*16.,Y*49.,58},{8,3,14},0,1);
        }
        Crate=M.Save(TEXT("SM_CoverCrate"));
    }
    {
        FNRMeshMaker M({Metal,White,Cyan,Screen,Rubber});
        M.Box({0,0,110},{75,105,220},0,4);
        M.Box({0,0,7},{85,112,14},4,2);
        for(int32 Y:{-1,1})M.Box({-40,Y*47.,110},{8,8,217},1,2);
        for(int32 Row=0;Row<9;++Row)
        {
            float Z=24+Row*20;
            M.Box({-39,0,Z},{5,83,16},4,1);
            for(int32 I=-2;I<=2;++I)M.Box({-42,I*12.,Z},{3,7,3},I%2==0?2:1);
            M.Box({-43,38,Z},{2,3,8},2);
        }
        M.Box({-40,0,199},{4,80,12},3,1);
        M.Box({-43,0,199},{2,56,2},2);
        Rack=M.Save(TEXT("SM_ServerRack"));
    }
    {
        FNRMeshMaker M({Metal,Cyan,White,Screen,Rubber});
        M.Box({0,0,-53},{78,78,14},0,4);
        M.Box({8,0,-15},{36,48,66},2,4);
        M.Box({0,0,33},{73,70,24},0,5,FRotator(0,0,0));
        M.Box({-16,0,47},{43,53,3},3,1,FRotator(18,0,0));
        for(int32 I=-2;I<=2;++I)M.Box({-16,I*8.,50},{32,3,1},1,0,FRotator(18,0,0));
        M.Box({20,0,52},{12,58,5},2,1);
        M.Box({-20,-34,34},{42,3,4},1,1);
        M.Box({-20,34,34},{42,3,4},1,1);
        for(int32 Y:{-1,1})M.Cylinder({15,Y*20.,-45},5,18,4,12,FRotator(0,0,90));
        Terminal=M.Save(TEXT("SM_Terminal"));
    }
    {
        FNRMeshMaker M({Metal,Cyan,White,Rubber,Amber});
        M.Sphere({0,0,0},{26,22,14},0,20,8);
        M.Box({7,0,6},{31,35,16},2,5);
        M.Box({25,0,0},{4,19,7},3,1);
        M.Box({28,0,0},{2,13,4},1,1);
        for(int32 Y:{-1,1})
        {
            M.Box({-5,Y*28.,0},{12,29,6},0,2);
            M.Cylinder({-5,Y*41.,1},19,8,2,24);
            M.Cylinder({-5,Y*41.,6},16,2,3,24);
            M.Cylinder({-5,Y*41.,8},4,3,0,12);
            M.Box({-5,Y*41.,9},{29,3,1},1);
            M.Box({-5,Y*41.,9},{3,29,1},1);
            M.Box({-20,Y*19.,-8},{4,4,11},4,1);
        }
        Drone=M.Save(TEXT("SM_SentinelDrone"));
    }
    {
        FNRMeshMaker M({Metal,Cyan,White,Screen});
        M.Cylinder({0,0,-20},26,10,0,24);
        M.Cylinder({0,0,20},26,6,2,24);
        M.Sphere({0,0,0},{15,15,15},1,16,8);
        for(int32 I=0;I<4;++I){float A=I*PI*.5f;M.Box({22*FMath::Cos(A),22*FMath::Sin(A),0},{6,6,41},2,2);}
        Core=M.Save(TEXT("SM_ElaraCore"));
    }
    {
        FNRMeshMaker M({Metal,Amber,White,Rubber});
        M.Box({0,0,0},{170,170,12},0,5);
        for(int32 X:{-1,1})for(int32 Y:{-1,1})
        {
            M.Box({X*70.,Y*70.,18},{20,20,40},2,3);
            M.Box({X*70.,Y*70.,40},{20,20,4},1,1);
        }
        for(int32 S:{-1,1}){M.Box({0,S*77.,9},{140,3,2},1);M.Box({S*77.,0,9},{3,140,2},1);}
        M.Box({0,0,9},{70,80,3},3,1);
        Extraction=M.Save(TEXT("SM_ExtractionPad"));
    }
    {
        FNRMeshMaker M({Floor,Metal});
        M.Box({0,0,-7},{398,398,14},0,2);
        for(int32 X:{-1,1})for(int32 Y:{-1,1})M.Cylinder({X*185.,Y*185.,.2},3,1,1,8);
        Panel=M.Save(TEXT("SM_FloorTile"));
    }
    // A native single-node blendspace animates shared skeletons without a Blueprint event graph.
    auto* BS=NewObject<UBlendSpace1D>(CreatePackage(TEXT("/Game/Art/Animations/BS_Operator")),TEXT("BS_Operator"),RF_Public|RF_Standalone);
    BS->SetSkeleton(LoadObject<USkeleton>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin")));
    auto& Axis=const_cast<FBlendParameter&>(BS->GetBlendParameter(0));Axis.DisplayName=TEXT("Speed");Axis.Min=0;Axis.Max=450;
    const TCHAR* Clips[]={TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"),TEXT("/Game/Characters/Mannequins/Anims/Rifle/Walk/MF_Rifle_Walk_Fwd.MF_Rifle_Walk_Fwd"),TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd.MF_Rifle_Jog_Fwd")};
    const float Speeds[]={0,180,450};
    for(int32 I=0;I<3;++I)if(auto* A=LoadObject<UAnimSequence>(nullptr,Clips[I]))BS->AddSample(A,FVector(Speeds[I],0,0));
    BS->ValidateSampleData();BS->ResampleData();BS->PostEditChange();NRSaveAsset(BS);
}
