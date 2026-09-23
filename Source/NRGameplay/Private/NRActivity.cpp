#include "NRActivity.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "NRDroneDirector.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "GameFramework/Controller.h"
ANRActivity::ANRActivity()
{
 Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));SetRootComponent(Collision);Collision->SetBoxExtent(FVector(40,40,52));Collision->SetCollisionProfileName(TEXT("BlockAll"));
 Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));Visual->SetupAttachment(Collision);Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->SetCanEverAffectNavigation(false);
}
void ANRActivity::BeginPlay(){Super::BeginPlay();OnRep_State();}
void ANRActivity::EndPlay(EEndPlayReason::Type R){GetWorldTimerManager().ClearAllTimersForObject(this);Super::EndPlay(R);}
void ANRActivity::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRActivity,Kind);DOREPLIFETIME(ANRActivity,bCompleted);DOREPLIFETIME(ANRActivity,Progress);}
bool ANRActivity::ValidOperator(ANRCharacter* P)const
{
 auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();if(!P||!P->IsAlive()||!GM||!GM->IsCombat()||FVector::DistSquared(P->GetActorLocation(),GetActorLocation())>260.f*260.f)return false;
 FVector Eye;FRotator Rot;P->GetActorEyesViewPoint(Eye,Rot);FHitResult H;FCollisionQueryParams Q;Q.AddIgnoredActor(P);
 return !GetWorld()->LineTraceSingleByChannel(H,Eye,GetActorLocation(),ECC_Visibility,Q)||H.GetActor()==this;
}
void ANRActivity::Interact(ANRCharacter* P)
{
 if(!HasAuthority()||bCompleted||Kind==ENRActivityKind::Target||!ValidOperator(P))return;
 if(Kind==ENRActivityKind::Supply){WakeForMutation();bCompleted=true;P->AwardSupply();OnRep_State();FinishMutation();return;}
 if(Operator.IsValid())return;WakeForMutation();Operator=P;Progress=0;
 // This timer exists only while a player channels; dormant stations have no tick.
 GetWorldTimerManager().SetTimer(ChannelTimer,this,&ANRActivity::UpdateChannel,.2f,true);
}
void ANRActivity::UpdateChannel()
{
 auto* P=Operator.Get();if(!ValidOperator(P))
 {Progress=0;Operator.Reset();GetWorldTimerManager().ClearTimer(ChannelTimer);FinishMutation();return;}
 Progress=FMath::Min(100,int(Progress)+5);ForceNetUpdate();
 if(Progress<100)return;for(TActorIterator<ANRDroneDirector> It(GetWorld());It;++It)It->DisableSecurity(12);bCompleted=true;P->GiveCredits(180);P->Say(4);
 if(auto* GS=GetWorld()->GetGameState<ANRGameState>())GS->SpendPower(P->Team,-25);
 Operator.Reset();GetWorldTimerManager().ClearTimer(ChannelTimer);OnRep_State();FinishMutation();
}
float ANRActivity::TakeDamage(float Amount,const FDamageEvent&,AController* DamageInstigator,AActor*)
{
 auto* P=DamageInstigator?Cast<ANRCharacter>(DamageInstigator->GetPawn()):nullptr;auto* GM=GetWorld()->GetAuthGameMode<ANRGameMode>();
 if(!HasAuthority()||Kind!=ENRActivityKind::Target||bCompleted||!P||!P->IsAlive()||!GM||!GM->IsCombat()||!FMath::IsFinite(Amount)||Amount<=0)return 0;
 WakeForMutation();bCompleted=true;P->GiveCredits(40);P->ClientHitFeedback(false);OnRep_State();FinishMutation();return Amount;
}
void ANRActivity::ResetActivity()
{if(!HasAuthority())return;WakeForMutation();GetWorldTimerManager().ClearTimer(ChannelTimer);Operator.Reset();Progress=0;bCompleted=false;OnRep_State();FinishMutation();}
void ANRActivity::OnRep_State()
{
 Collision->SetBoxExtent(Kind==ENRActivityKind::Target?FVector(12,27,45):FVector(40,40,52));
 if(GetNetMode()==NM_DedicatedServer)return;
 const TCHAR* Mesh=Kind==ENRActivityKind::Supply?TEXT("/Game/Art/Meshes/SM_SupplyStation"):Kind==ENRActivityKind::Relay?TEXT("/Game/Art/Meshes/SM_RelayStation"):TEXT("/Game/Art/Meshes/SM_RangeTarget");
 Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mesh));
 Visual->SetRelativeRotation(bCompleted&&Kind==ENRActivityKind::Target?FRotator(0,0,80):FRotator::ZeroRotator);
 Visual->SetMaterial(3,LoadObject<UMaterialInterface>(nullptr,bCompleted?TEXT("/Game/Art/Materials/M_Graphite"):TEXT("/Game/Art/Materials/M_CyanLight")));
}
FString ANRActivity::Prompt()const
{
 if(bCompleted)return TEXT("ЗАДАЧА ВЫПОЛНЕНА / СБРОС В НОВОМ РАУНДЕ");
 if(Kind==ENRActivityKind::Supply)return TEXT("F  СНАБЖЕНИЕ: ПАТРОНЫ + 35 HP / ОДНОРАЗОВО");
 if(Kind==ENRActivityKind::Target)return TEXT("ИСПЫТАНИЕ ТОЧНОСТИ / +40 CR ЗА МИШЕНЬ");
 return Progress>0?FString::Printf(TEXT("ПЕРЕХВАТ %d%% / ОСТАВАЙТЕСЬ РЯДОМ"),Progress):TEXT("F  ПЕРЕХВАТ РЕЛЕ / 4 СЕК / +180 CR / ОТКЛЮЧЕНИЕ ОХРАНЫ 12 С");
}
