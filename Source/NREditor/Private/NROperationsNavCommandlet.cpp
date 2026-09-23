#include "NROperationsNavCommandlet.h"
#include "NRMeshMaker.h"
#include "FileHelpers.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
UNROperationsNavCommandlet::UNROperationsNavCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UNROperationsNavCommandlet::Main(const FString&)
{
 UWorld* W=UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/Maps/NR_Arcology"));if(!W)return 2;int Count=0;
 for(TActorIterator<ANavMeshBoundsVolume> It(W);It;++It)
 {
  auto* B=NewObject<UCubeBuilder>();B->X=5200;B->Y=4200;B->Z=1400;
  // Build alone does not initialize a volume's Model/Polys. The editor factory creates the persisted brush.
  UActorFactory::CreateBrushForVolumeActor(*It,B);It->SetActorLocation(FVector(0,0,400));It->ReregisterAllComponents();It->PostEditMove(true);
  const FBox Bounds=It->GetComponentsBoundingBox(true);if(!Bounds.IsValid||Bounds.GetSize().X<5000||Bounds.GetSize().Y<4000)return 3;
  if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W))Nav->OnNavigationBoundsUpdated(*It);
  ++Count;
 }
 if(Count==0||!NRSaveAsset(W,true))return 4;UE_LOG(LogTemp,Display,TEXT("NR_OPERATIONS_NAV_COMPLETE %d valid volumes"),Count);return 0;
}
