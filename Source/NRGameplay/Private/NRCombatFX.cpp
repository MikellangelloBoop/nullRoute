#include "NRCombatFX.h"
#include "Engine/World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

bool UNRCombatFX::ShouldCreateSubsystem(UObject* Outer) const
{
    auto* W=Cast<UWorld>(Outer);
    return W&&W->IsGameWorld()&&!IsRunningDedicatedServer();
}

void UNRCombatFX::EnsurePool()
{
    if(PoolOwner||GetWorld()->GetNetMode()==NM_DedicatedServer)return;
    PoolOwner=GetWorld()->SpawnActor<AActor>();
    auto Mesh=[&](const TCHAR* Shape,const TCHAR* Material)
    {
        auto* C=NewObject<UInstancedStaticMeshComponent>(PoolOwner);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Shape));
        C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Material));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);C->RegisterComponent();
        return C;
    };
    TracerMeshes=Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("/Game/Art/Materials/M_Tracer.M_Tracer"));
    SparkMeshes=Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("/Game/Art/Materials/M_Tracer.M_Tracer"));
    FlashMeshes=Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"),TEXT("/Game/Art/Materials/M_Tracer.M_Tracer"));
    ScorchMeshes=Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"),TEXT("/Game/Art/Materials/M_Impact.M_Impact"));
}

void UNRCombatFX::Sound(FName Name,FVector Position,float Volume,bool Local,bool Quiet)
{
    if(GetWorld()->GetNetMode()==NM_DedicatedServer)return;
    auto& S=Sounds.FindOrAdd(Name);
    if(!S)S=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Art/Audio/%s.%s"),*Name.ToString(),*Name.ToString()));
    if(!Attenuation)
    {
        Attenuation=NewObject<USoundAttenuation>(this);
        Attenuation->Attenuation.bAttenuate=true;
        Attenuation->Attenuation.bSpatialize=true;
        Attenuation->Attenuation.AttenuationShapeExtents=FVector(140);
        Attenuation->Attenuation.FalloffDistance=5500;
        Attenuation->Attenuation.bEnableOcclusion=true;
        Attenuation->Attenuation.OcclusionLowPassFilterFrequency=1600;
        Attenuation->Attenuation.OcclusionVolumeAttenuation=.45f;
        FootAttenuation=NewObject<USoundAttenuation>(this);FootAttenuation->Attenuation=Attenuation->Attenuation;
        FootAttenuation->Attenuation.AttenuationShapeExtents=FVector(80);FootAttenuation->Attenuation.FalloffDistance=1500;
        QuietAttenuation=NewObject<USoundAttenuation>(this);QuietAttenuation->Attenuation=FootAttenuation->Attenuation;QuietAttenuation->Attenuation.AttenuationShapeExtents=FVector(35);QuietAttenuation->Attenuation.FalloffDistance=450;
        WorldVoices=NewObject<USoundConcurrency>(this);WorldVoices->Concurrency.MaxCount=32;WorldVoices->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopQuietest;
        LocalVoices=NewObject<USoundConcurrency>(this);LocalVoices->Concurrency.MaxCount=12;LocalVoices->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopOldest;
    }
    if(!S)return;
    const float Pitch=FMath::FRandRange(.97f,1.03f);
    if(Local)UGameplayStatics::PlaySound2D(GetWorld(),S,Volume,Pitch,0,LocalVoices);
    else UGameplayStatics::PlaySoundAtLocation(GetWorld(),S,Position,FRotator::ZeroRotator,Volume,Pitch,0,Name.ToString().StartsWith(TEXT("S_Step"))?(Quiet?QuietAttenuation:FootAttenuation):Attenuation,WorldVoices);
}

void UNRCombatFX::Shot(FVector Start,FVector Direction,float Speed,AActor* Source,uint8 WeaponType,bool Suppressed)
{
    EnsurePool();if(!PoolOwner)return;
    // Pools are local and bounded: no replicated cosmetic actors or fragment RPCs.
    if(Traces.Num()<96)
    {
        FNRVisualParticle P;P.Position=Start;P.Velocity=Direction*Speed;P.Source=Source;P.Lifetime=.3f;Traces.Add(P);
    }
    if(!Suppressed&&Flashes.Num()<32)
    {
        FNRVisualParticle P;P.Position=Start;P.Normal=Direction;P.Size=1;P.Lifetime=.045f;Flashes.Add(P);
    }
    Sound(WeaponType==4?TEXT("S_Shotgun"):WeaponType==5?TEXT("S_LMG"):WeaponType==3?TEXT("S_Pistol"):WeaponType==1?TEXT("S_Marksman"):WeaponType==2?TEXT("S_Carbine"):TEXT("S_SMG"),Start,Suppressed?.16f:.58f,Cast<APawn>(Source)&&Cast<APawn>(Source)->IsLocallyControlled());
}

void UNRCombatFX::Impact(FVector Position,FVector Normal,bool Organic)
{
    EnsurePool();if(!PoolOwner)return;
    for(int32 I=0;I<(Organic?5:9)&&Sparks.Num()<160;++I)
    {
        FNRVisualParticle P;P.Position=Position+Normal*2;P.Velocity=(Normal+FMath::VRand()*.85f).GetSafeNormal()*FMath::FRandRange(120.f,450.f);
        P.Lifetime=FMath::FRandRange(.13f,.36f);P.Size=FMath::FRandRange(.6f,1.2f);Sparks.Add(P);
    }
    if(!Organic)
    {
        if(Scorches.Num()>=64)Scorches.RemoveAt(0);
        FNRVisualParticle P;P.Position=Position+Normal*.18;P.Normal=Normal;P.Size=FMath::FRandRange(3.5f,6.f);P.Lifetime=18;Scorches.Add(P);
    }
    Sound(TEXT("S_Impact"),Position,.18f);
}

void UNRCombatFX::Breach(FVector Position)
{
    EnsurePool();if(!PoolOwner)return;
    for(int32 I=0;I<50&&Sparks.Num()<160;++I)
    {
        FNRVisualParticle P;P.Position=Position;P.Velocity=FMath::VRand()*FMath::FRandRange(200.f,900.f);P.Lifetime=FMath::FRandRange(.3f,1.1f);P.Size=2;Sparks.Add(P);
    }
    FNRVisualParticle Flash;Flash.Position=Position;Flash.Size=12;Flash.Lifetime=.12f;Flashes.Add(Flash);
    Sound(TEXT("S_Breach"),Position,.9f);
}

void UNRCombatFX::UpdateInstances(UInstancedStaticMeshComponent* C,const TArray<FTransform>& T)
{
    while(C->GetInstanceCount()>T.Num())C->RemoveInstance(C->GetInstanceCount()-1);
    for(int32 I=0;I<T.Num();++I)
    {
        if(I>=C->GetInstanceCount())C->AddInstance(T[I],true);
        else C->UpdateInstanceTransform(I,T[I],true,I==T.Num()-1,true);
    }
}

void UNRCombatFX::Tick(float Dt)
{
    EnsurePool();if(!PoolOwner)return;
    if(!Sounds.Contains(TEXT("S_RoomTone"))){auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/Art/Audio/S_RoomTone.S_RoomTone"));Sounds.Add(TEXT("S_RoomTone"),S);if(S)RoomTone=UGameplayStatics::SpawnSound2D(GetWorld(),S,.10f,1,0,nullptr,false);}
    TArray<FTransform> T;T.Reserve(160);
    for(int32 I=Traces.Num()-1;I>=0;--I)
    {
        auto& P=Traces[I];P.Age+=Dt;
        FVector Old=P.Position;P.Velocity.Z-=980*Dt;P.Position+=P.Velocity*Dt;
        FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(NRTracer),false,P.Source.Get());
        bool Hit=GetWorld()->LineTraceSingleByChannel(H,Old,P.Position,ECC_GameTraceChannel1,Q);
        if(Hit)P.Position=H.ImpactPoint;
        const FVector D=(P.Position-Old).GetSafeNormal();float Length=FMath::Min(140.f,float((P.Position-Old).Size()));
        if(P.Age>P.Lifetime){Traces.RemoveAtSwap(I);continue;}
        T.Add(FTransform(D.Rotation(),P.Position-D*Length*.5f,FVector(Length/100,.0035,.0035)));
        if(Hit)Traces.RemoveAtSwap(I);
    }
    UpdateInstances(TracerMeshes,T);T.Reset();
    for(int32 I=Sparks.Num()-1;I>=0;--I)
    {
        auto& P=Sparks[I];P.Age+=Dt;if(P.Age>P.Lifetime){Sparks.RemoveAtSwap(I);continue;}
        P.Velocity.Z-=750*Dt;P.Position+=P.Velocity*Dt;float Fade=1-P.Age/P.Lifetime;
        T.Add(FTransform(P.Velocity.Rotation(),P.Position,FVector(.08f*P.Size,.004f*Fade,.004f*Fade)));
    }
    UpdateInstances(SparkMeshes,T);T.Reset();
    for(int32 I=Flashes.Num()-1;I>=0;--I)
    {
        auto& P=Flashes[I];P.Age+=Dt;if(P.Age>P.Lifetime){Flashes.RemoveAtSwap(I);continue;}
        float S=P.Size*(1-P.Age/P.Lifetime);T.Add(FTransform(P.Normal.Rotation(),P.Position,FVector(.15,.065,.065)*S));
    }
    UpdateInstances(FlashMeshes,T);T.Reset();
    for(int32 I=Scorches.Num()-1;I>=0;--I)
    {
        auto& P=Scorches[I];P.Age+=Dt;if(P.Age>P.Lifetime){Scorches.RemoveAtSwap(I);continue;}
        T.Add(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,P.Normal),P.Position,FVector(P.Size/100,P.Size/100,.0006)));
    }
    UpdateInstances(ScorchMeshes,T);
}

void UNRCombatFX::Ricochet(FVector Position,FVector Direction,float Speed,AActor* Source)
{
 EnsurePool();if(!PoolOwner||Traces.Num()>=96)return;
 FNRVisualParticle P;P.Position=Position;P.Velocity=Direction*FMath::Clamp(Speed,100.f,100000.f);P.Source=Source;P.Lifetime=.35f;Traces.Add(P);
 Sound(TEXT("S_Ricochet"),Position,.35f);
}

bool UNRCombatFX::DamageFeedback(float HealthLost,float RemainingHealth)
{
 if(GetWorld()->GetNetMode()==NM_DedicatedServer||!FMath::IsFinite(HealthLost)||!FMath::IsFinite(RemainingHealth)||HealthLost<=0)return false;
 const double Now=GetWorld()->GetTimeSeconds();
 // A burst stays audible without stacking several body impacts on top of one another.
 if(Now-LastDamageSound<.12)return false;
 const FName Cue=RemainingHealth<=25?TEXT("S_HurtCritical"):TEXT("S_Hurt");auto& S=Sounds.FindOrAdd(Cue);
 if(!S)S=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Art/Audio/%s.%s"),*Cue.ToString(),*Cue.ToString()));
 if(!S)return false;
 if(!DamageVoices){DamageVoices=NewObject<USoundConcurrency>(this);DamageVoices->Concurrency.MaxCount=1;DamageVoices->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopOldest;}
 // A dedicated 2D voice is not attenuated by distance/occlusion or stolen by the footsteps group.
 DamageAudio=UGameplayStatics::SpawnSound2D(GetWorld(),S,FMath::Clamp(.78f+HealthLost*.006f,.78f,1.f),1,0,DamageVoices);
 if(!DamageAudio)return false;
 LastDamageSound=Now;LastDamageCue=Cue;return true;
}
