#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NRHUD.generated.h"
class SWidget;
class SNRWorkbenchWidget;
class SNRServerBrowserWidget;
class SEditableTextBox;
class SHorizontalBox;
UCLASS()
class NRGAMEPLAY_API ANRHUD : public AHUD
{
    GENERATED_BODY()
public:
    FText CachedContext,CachedScan;
    FString MissionGuide,ActivityGuide;float GuideYaw=0,GuideMeters=0;uint8 InteractionProgress=0;
    void RefreshMission();
 void RefreshOperations();
 FString TargetDrone;float TargetHealth=0,Threat=0,BlackoutSeconds=0;int32 AlertDrones=0;
 FVector2D PingScreen=FVector2D::ZeroVector;float PingMeters=0;
    int32 TrainingFaction=2;
    FString TrainingOptions(bool Defend)const;
    void ToggleMenu();
    void ToggleServers();
    void ToggleWorkbench();
    void ToggleGraph();
    bool IsMenuOpen() const { return bMenuOpen||bGraphOpen||bWorkbenchOpen||bServersOpen; }
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;
private:
    TSharedPtr<SWidget> Overlay;
    TSharedPtr<SNRWorkbenchWidget> Workbench;
    TSharedPtr<SNRServerBrowserWidget> ServerBrowser;
    TSharedPtr<SEditableTextBox> Address;
    TSharedPtr<SHorizontalBox> QualityButtons;
    TSharedPtr<SHorizontalBox> SkinButtons;
    FText ConnectionStatus;
    FTimerHandle ContextTimer;
    bool bMenuOpen=false,bGraphOpen=false,bWorkbenchOpen=false,bServersOpen=false;
    void ApplyInputMode();
    FText ContextText() const;
    FText ScanText() const;
};
