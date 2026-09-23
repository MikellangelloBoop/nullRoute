#include "NRNode.h"
#include "NRAbilities.h"
#include "AbilitySystemComponent.h"
#include "NRGraphSubsystem.h"
#include "NRCharacter.h"
#include "NRGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationInvokerComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "NRDebrisSubsystem.h"
ANodeBase::ANodeBase(){PrivateStateASC=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("PrivateStateASC"));PrivateStateASC->SetIsReplicated(false);Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));SetRootComponent(Mesh);
 Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetRelativeScale3D(FVector(.85,.85,1.2));Mesh->SetCollisionProfileName(TEXT("BlockAll"));
 Invoker=CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("Invoker"));}
void ANodeBase::Initialize(uint8 InTeam,ENRNodeOp InOperation,bool bBlack){if(!HasAuthority())return;WakeForMutation();NodeID=FGuid::NewGuid();Team=InTeam;Operation=InOperation;bBlackNode=bBlack;if(bBlack){PrivateStateASC->InitAbilityActorInfo(this,this);PrivateStateASC->ApplyGameplayEffectToSelf(GetDefault<UGE_BlackNode>(),1.f,PrivateStateASC->MakeEffectContext());}}
void ANodeBase::BeginPlay(){Super::BeginPlay();if(HasAuthority()&&!NodeID.IsValid())NodeID=FGuid::NewGuid();GetWorld()->GetSubsystem<UNRGraphSubsystem>()->RegisterNode(this);OnRep_State();}
void ANodeBase::EndPlay(EEndPlayReason::Type R){if(auto* G=GetWorld()->GetSubsystem<UNRGraphSubsystem>())G->RemoveNode(this);GetWorldTimerManager().ClearTimer(PrintTimer);Super::EndPlay(R);}
void ANodeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(ANodeBase,NodeID);DOREPLIFETIME(ANodeBase,Team);DOREPLIFETIME(ANodeBase,NodeState);
 DOREPLIFETIME_CONDITION(ANodeBase,Operation,COND_OwnerOnly);DOREPLIFETIME_CONDITION(ANodeBase,OutputPins,COND_OwnerOnly);
 DOREPLIFETIME(ANodeBase,Revision);DOREPLIFETIME(ANodeBase,bOutput);DOREPLIFETIME(ANodeBase,PhaseEnd);DOREPLIFETIME(ANodeBase,PrintPhase);}
void ANodeBase::StartPrinting(){WakeForMutation();NodeState=ENRNodeState::Printing;PrintPhase=ENRPrintPhase::Polymer;PhaseEnd=GetWorld()->GetTimeSeconds()+1.5f;OnRep_State();GetWorldTimerManager().SetTimer(PrintTimer,this,&ANodeBase::ChangeFilament,1.5f,false);ForceNetUpdate();}
void ANodeBase::ChangeFilament(){if(NodeState==ENRNodeState::Destroyed)return;PrintPhase=ENRPrintPhase::FilamentChange;PhaseEnd=GetWorld()->GetTimeSeconds()+.65f;ForceNetUpdate();GetWorldTimerManager().SetTimer(PrintTimer,this,&ANodeBase::Reinforce,.65f,false);}
void ANodeBase::Reinforce(){if(NodeState==ENRNodeState::Destroyed)return;PrintPhase=ENRPrintPhase::Reinforcement;PhaseEnd=GetWorld()->GetTimeSeconds()+.75f;ForceNetUpdate();GetWorldTimerManager().SetTimer(PrintTimer,this,&ANodeBase::CompletePrinting,.75f,false);}
void ANodeBase::CompletePrinting(){if(!HasAuthority()||NodeState==ENRNodeState::Destroyed)return;WakeForMutation();NodeState=ENRNodeState::Active;PrintPhase=ENRPrintPhase::None;++Revision;OnRep_State();FinishMutation();}
void ANodeBase::SetOutput(bool V){if(!HasAuthority()||V==bOutput)return;WakeForMutation();bOutput=V;++Revision;FinishMutation();}
void ANodeBase::NetExecute(){if(!HasAuthority()||NodeState==ENRNodeState::Destroyed)return;WakeForMutation();NodeState=ENRNodeState::Destroyed;bOutput=false;++Revision;GetWorldTimerManager().ClearTimer(PrintTimer);OnRep_State();FinishMutation();}
bool ANodeBase::Capture(ANRCharacter* C){if(!HasAuthority()||!C||!C->IsAlive()||Team==C->Team||NodeState==ENRNodeState::Destroyed)return false;
 const auto* GS=GetWorld()->GetGameState<ANRGameState>();if(GS&&GS->Phase==ENRPhase::Preparation&&(!GS->CanPrepareAt(C->Team,GetActorLocation())||!GS->CanPrepareAt(C->Team,C->GetActorLocation())||Team!=255))return false;
 if(bBlackNode&&Team!=255&&C->OperatorRole==ENRRole::Scout){GetWorld()->GetSubsystem<UNRGraphSubsystem>()->Cascade(this);return true;}
 WakeForMutation();Team=C->Team;SetOwner(C);NodeState=ENRNodeState::Active;++Revision;OnRep_State();FinishMutation();C->GiveCredits(100);return true;}
float ANodeBase::TakeDamage(float Amount,FDamageEvent const&,AController*,AActor*){if(!HasAuthority()||!FMath::IsFinite(Amount)||Amount<=0)return 0;Health-=Amount;if(Health<=0)NetExecute();return Amount;}
void ANodeBase::OnRep_State(){
 const bool Dead=NodeState==ENRNodeState::Destroyed;Mesh->SetCollisionEnabled(Dead?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);Mesh->SetVisibility(!Dead);
 if(Dead){Invoker->Deactivate();if(auto* D=GetWorld()->GetSubsystem<UNRDebrisSubsystem>())D->SpawnBurst(GetActorLocation(),GetTypeHash(NodeID));return;}
 Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,NodeState==ENRNodeState::Printing?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Game/Art/Meshes/SM_Terminal.SM_Terminal")));
 Mesh->SetRelativeScale3D(NodeState==ENRNodeState::Printing?FVector(.85,.85,1.2):FVector(1));
 if(GetNetMode()!=NM_DedicatedServer){if(auto* Material=LoadObject<UMaterialInterface>(nullptr,NodeState==ENRNodeState::Printing?TEXT("/Game/Materials/M_Polymer.M_Polymer"):TEXT("/Game/Materials/M_Node.M_Node"))){auto* MID=UMaterialInstanceDynamic::Create(Material,this);MID->SetVectorParameterValue(TEXT("Tint"),Team==0?FLinearColor(.08,.75,1):Team==1?FLinearColor(1,.35,.08):FLinearColor(.55,.12,1));MID->SetScalarParameterValue(TEXT("Opacity"),NodeState==ENRNodeState::Printing?.3f:1.f);Mesh->SetMaterial(NodeState==ENRNodeState::Printing?0:1,MID);if(NodeState!=ENRNodeState::Printing)Mesh->SetMaterial(0,nullptr);}}
}
