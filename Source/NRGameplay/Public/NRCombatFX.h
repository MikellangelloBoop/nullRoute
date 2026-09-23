#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRCombatFX.generated.h"
class UInstancedStaticMeshComponent;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;
class UAudioComponent;
struct FNRVisualParticle
{
    FVector Position=FVector::ZeroVector, Velocity=FVector::ZeroVector;
    FVector Normal=FVector::UpVector;
    float Age=0, Lifetime=.15f, Size=1;
    TWeakObjectPtr<AActor> Source;
};

UCLASS()
class NRGAMEPLAY_API UNRCombatFX : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(NRCombatFX,STATGROUP_Tickables); }
    virtual void Tick(float DeltaTime) override;
    void Shot(FVector Start,FVector Direction,float Speed,AActor* Source,uint8 WeaponType,bool Suppressed=false);
    void Impact(FVector Position,FVector Normal,bool Organic);
    void Breach(FVector Position);
    void Ricochet(FVector Position,FVector Direction,float Speed,AActor* Source);
    bool DamageFeedback(float HealthLost,float RemainingHealth);
    void Sound(FName Name,FVector Position,float Volume=1.f,bool Local=false,bool Quiet=false);
private:
    friend class UNRSmokeSubsystem;
    double LastDamageSound=-100;
    FName LastDamageCue;
    UPROPERTY() TObjectPtr<USoundConcurrency> DamageVoices;
    UPROPERTY() TObjectPtr<UAudioComponent> DamageAudio;
    void EnsurePool();
    void UpdateInstances(UInstancedStaticMeshComponent* Component,const TArray<FTransform>& Transforms);
    UPROPERTY() TObjectPtr<AActor> PoolOwner;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> TracerMeshes;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SparkMeshes;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> FlashMeshes;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ScorchMeshes;
    UPROPERTY() TObjectPtr<USoundAttenuation> Attenuation;
    UPROPERTY() TObjectPtr<USoundAttenuation> FootAttenuation;
    UPROPERTY() TObjectPtr<USoundAttenuation> QuietAttenuation;
    UPROPERTY() TObjectPtr<USoundConcurrency> WorldVoices;
    UPROPERTY() TObjectPtr<USoundConcurrency> LocalVoices;
    UPROPERTY() TObjectPtr<UAudioComponent> RoomTone;
    UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> Sounds;
    TArray<FNRVisualParticle> Traces,Sparks,Flashes,Scorches;
};
