#include "NRPreparationZone.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANRPreparationProp::ANRPreparationProp()
{
 bReplicates=true;bReplicateUsingRegisteredSubObjectList=true;NetUpdateFrequency=5;MinNetUpdateFrequency=1;
 PrimaryActorTick.bCanEverTick=false;
 Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));SetRootComponent(Collision);
 Collision->SetBoxExtent(FVector(14,30,48));Collision->SetCollisionProfileName(TEXT("BlockAll"));
 Collision->SetGenerateOverlapEvents(false);Collision->SetCanEverAffectNavigation(false);
 Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));Visual->SetupAttachment(Collision);
 Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->SetCanEverAffectNavigation(false);
}
void ANRPreparationProp::BeginPlay(){Super::BeginPlay();OnRep_State();}
void ANRPreparationProp::EndPlay(EEndPlayReason::Type R){GetWorldTimerManager().ClearTimer(FlashTimer);Super::EndPlay(R);}
void ANRPreparationProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRPreparationProp,bStation);DOREPLIFETIME(ANRPreparationProp,bAttackSide);DOREPLIFETIME(ANRPreparationProp,bAvailable);DOREPLIFETIME(ANRPreparationProp,bHit);}
bool ANRPreparationProp::ValidOperator(const ANRCharacter* C)const
{
 const auto* S=GetWorld()->GetGameState<ANRGameState>();
 return HasAuthority()&&bAvailable&&C&&C->IsAlive()&&S&&S->Phase==ENRPhase::Preparation&&
  (C->Team==S->AttackTeam)==bAttackSide&&S->CanPrepareAt(C->Team,C->GetActorLocation());
}
void ANRPreparationProp::SetAvailable(bool Value)
{if(!HasAuthority())return;bAvailable=Value;bHit=false;LastHit=-100;GetWorldTimerManager().ClearTimer(FlashTimer);OnRep_State();ForceNetUpdate();}
float ANRPreparationProp::TakeDamage(float Amount,const FDamageEvent&,AController* DamageInstigator,AActor* Causer)
{
 auto* C=DamageInstigator?Cast<ANRCharacter>(DamageInstigator->GetPawn()):Cast<ANRCharacter>(Causer);
 const float Now=GetWorld()->GetTimeSeconds();
 if(bStation||!FMath::IsFinite(Amount)||Amount<=0||!ValidOperator(C)||Now-LastHit<.07f)return 0;
 LastHit=Now;bHit=true;C->PracticeHits=FMath::Min(C->PracticeHits+1,999);C->ClientHitFeedback(false);
 // Practice never awards credits, kills, drone rewards or objective progress.
 OnRep_State();ForceNetUpdate();GetWorldTimerManager().SetTimer(FlashTimer,[this](){bHit=false;OnRep_State();ForceNetUpdate();},.18f,false);return Amount;
}
bool ANRPreparationProp::Interact(ANRCharacter* C)
{
 if(!bStation||!ValidOperator(C)||FVector::DistSquared(C->GetActorLocation(),GetActorLocation())>260.f*260.f)return false;
 FHitResult H;FCollisionQueryParams Q;Q.AddIgnoredActor(C);
 if(GetWorld()->LineTraceSingleByChannel(H,C->GetPawnViewLocation(),GetActorLocation(),ECC_Visibility,Q)&&H.GetActor()!=this)return false;
 C->RefillAmmunition();C->Say(3);return true;
}
FString ANRPreparationProp::Prompt()const
{
 if(!bAvailable)return TEXT("ПОДГОТОВКА ЗАВЕРШЕНА");
 return bStation?TEXT("F  ПОПОЛНИТЬ ПАТРОНЫ   /   B  МОДУЛИ   /   TAB  КЛАСС"):TEXT("ТРЕНИРОВОЧНАЯ МИШЕНЬ  /  БЕЗ НАГРАД И БОЕВОГО УРОНА");
}
void ANRPreparationProp::OnRep_State()
{
 Collision->SetBoxExtent(bStation?FVector(42,42,52):FVector(14,30,48));
 Collision->SetCollisionEnabled((bStation||bAvailable)?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 if(GetNetMode()==NM_DedicatedServer)return;
 Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,bStation?TEXT("/Game/Art/Meshes/SM_SupplyStation"):TEXT("/Game/Art/Meshes/SM_RangeTarget")));
 Visual->SetVisibility(bStation||bAvailable);
 Visual->SetMaterial(3,LoadObject<UMaterialInterface>(nullptr,!bAvailable?TEXT("/Game/Art/Materials/M_Graphite"):bHit?TEXT("/Game/Art/Materials/M_AmberLight"):TEXT("/Game/Art/Materials/M_CyanLight")));
 if(!bStation&&TargetDetails.IsEmpty())
 {
  // A readable target face is purely cosmetic and never creates extra server hit bodies.
  auto* Graphite=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Graphite"));
  auto* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_CyanLight"));
  auto Add=[&](const TCHAR* Mesh,FVector At,FVector Scale,UMaterialInterface* Material,bool Disk)
  {
   auto* Part=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(Collision);
   Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mesh));Part->SetRelativeLocation(At);Part->SetRelativeScale3D(Scale);if(Disk)Part->SetRelativeRotation(FRotator(90,0,0));
   Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);Part->SetCastShadow(false);Part->SetMaterial(0,Material);Part->RegisterComponent();TargetDetails.Add(Part);return Part;
  };
  Add(TEXT("/Engine/BasicShapes/Cube"),FVector(15,0,16),FVector(.025,.46,.56),Glow,false);
  Add(TEXT("/Engine/BasicShapes/Cube"),FVector(17,0,16),FVector(.025,.42,.52),Graphite,false);
  TargetMarker=Add(TEXT("/Engine/BasicShapes/Cylinder"),FVector(19,0,16),FVector(.22,.22,.015),Glow,true);
  Add(TEXT("/Engine/BasicShapes/Cylinder"),FVector(20,0,16),FVector(.14,.14,.01),Graphite,true);
  Add(TEXT("/Engine/BasicShapes/Cylinder"),FVector(21,0,16),FVector(.06,.06,.01),Glow,true);
 }
 for(const auto& Part:TargetDetails)Part->SetVisibility(!bStation&&bAvailable);
 if(TargetMarker)TargetMarker->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,bHit?TEXT("/Game/Art/Materials/M_AmberLight"):TEXT("/Game/Art/Materials/M_CyanLight")));
}

ANRPreparationZone::ANRPreparationZone()
{
 bReplicates=true;bAlwaysRelevant=true;bReplicateUsingRegisteredSubObjectList=true;NetUpdateFrequency=2;MinNetUpdateFrequency=1;
 PrimaryActorTick.bCanEverTick=false;
 Barrier=CreateDefaultSubobject<UBoxComponent>(TEXT("PreparationBarrier"));SetRootComponent(Barrier);
 Barrier->SetBoxExtent(FVector(12,1990,650));Barrier->SetCollisionProfileName(TEXT("BlockAll"));
 Barrier->SetGenerateOverlapEvents(false);Barrier->SetCanEverAffectNavigation(false);
}
void ANRPreparationZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ANRPreparationZone,bClosed);}
void ANRPreparationZone::BeginPlay()
{
 Super::BeginPlay();
 if(HasAuthority())for(int Side=0;Side<2;++Side)for(int Index=0;Index<4;++Index)
 {
  const bool Attack=Side==0,Station=Index==3;
  const FVector P=Station?FVector(Attack?-2120:2120,Attack?300:-300,52):FVector(Attack?-2370:2370,Attack?700+Index*320:-1150+Index*320,48);
  const FTransform T(FRotator(0,Attack?0:180,0),P);
  auto* Prop=GetWorld()->SpawnActorDeferred<ANRPreparationProp>(ANRPreparationProp::StaticClass(),T,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  Prop->bStation=Station;Prop->bAttackSide=Attack;Prop->FinishSpawning(T);Props.Add(Prop);
 }
 if(GetNetMode()!=NM_DedicatedServer)CreateLocalVisuals();OnRep_Closed();
}
void ANRPreparationZone::SetPreparing(bool Value)
{if(!HasAuthority())return;bClosed=Value;OnRep_Closed();for(const auto& Prop:Props)if(IsValid(Prop.Get()))Prop->SetAvailable(Value);ForceNetUpdate();}
void ANRPreparationZone::OnRep_Closed()
{
 Barrier->SetCollisionEnabled(bClosed?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 for(const auto& Part:GateVisuals)Part->SetVisibility(bClosed);
}
void ANRPreparationZone::CreateLocalVisuals()
{
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube"));
 auto* Cyan=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_CyanLight"));
 auto* Amber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_AmberLight"));
 auto Box=[&](FVector World,FVector Scale,UMaterialInterface* M,bool Gate)
 {
  auto* Part=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(Barrier);
  Part->SetStaticMesh(Cube);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);
  Part->SetRelativeLocation(World-GetActorLocation());Part->SetRelativeScale3D(Scale);Part->SetMaterial(0,M);Part->SetCastShadow(false);Part->RegisterComponent();if(Gate)GateVisuals.Add(Part);
 };
 auto Label=[&](FVector World,FRotator Rotation,const TCHAR* Text,FColor Color,float Size,bool Gate=false)
 {
  auto* Part=NewObject<UTextRenderComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(Barrier);Part->SetRelativeLocation(World-GetActorLocation());
  Part->SetRelativeRotation(Rotation);Part->SetText(FText::FromString(Text));Part->SetTextRenderColor(Color);Part->SetWorldSize(Size);Part->SetHorizontalAlignment(EHTA_Center);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->RegisterComponent();if(Gate)GateVisuals.Add(Part);
 };
 const float X=GetActorLocation().X;
 auto* Screen=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_Polymer")),this);
 Screen->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.02,.24,.4));Screen->SetScalarParameterValue(TEXT("Opacity"),.025);
 Box(FVector(X,0,370),FVector(.015,39.4,7.6),Screen,true);
 for(int Y=-1800;Y<=1800;Y+=300)Box(FVector(X,Y,370),FVector(.028,.035,7.6),Cyan,true);
 for(float Z:{12.f,250.f,500.f,740.f})Box(FVector(X,0,Z),FVector(.028,39.4,.025),Cyan,true);
 Box(FVector(X-45,0,1),FVector(.11,38,.012),Amber,false);Box(FVector(X+45,0,1),FVector(.11,38,.012),Cyan,false);
 Box(FVector(X,0,489),FVector(.05,10,.9),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Graphite")),true);
 Label(FVector(X-18,0,475),FRotator(0,180,0),TEXT("ATTACK / READY BAY"),FColor(255,190,90),37,true);
 Label(FVector(X+18,0,475),FRotator::ZeroRotator,TEXT("DEFEND / PREPARE THE SITE"),FColor(85,220,255),33,true);
 Label(FVector(-2410,1020,200),FRotator::ZeroRotator,TEXT("WARMUP / NO LIVE DAMAGE"),FColor(255,190,90),24);
 Label(FVector(2410,-830,200),FRotator(0,180,0),TEXT("DEFENSE / CALIBRATION"),FColor(85,220,255),24);
 for(int Side:{-1,1})
 {
  const bool Attack=Side<0;const float Y=Attack?300:-300;
  Label(FVector(Side*2120,Y,175),FRotator(0,Attack?0:180,0),TEXT("LOADOUT / F RESUPPLY"),Attack?FColor(255,190,90):FColor(85,220,255),20);
  Box(FVector(Side*2230,Attack?1020:-830,1),FVector(.035,8,.012),Attack?Amber:Cyan,false);
 }
}
