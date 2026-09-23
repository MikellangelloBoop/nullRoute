#include "NRHUD.h"
#include "NRServerBrowserWidget.h"
#include "Engine/GameInstance.h"
#include "NRPreparationZone.h"
#include "NRVisorWidget.h"
#include "NRWorkbenchWidget.h"
#include "NRGraphWidget.h"
#include "NRCharacter.h"
#include "NRCombatFX.h"
#include "NRAttributes.h"
#include "NRGameMode.h"
#include "NRObjective.h"
#include "NRActivity.h"
#include "NRTacticalDoor.h"
#include "EngineUtils.h"
#include "NRNode.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Brushes/SlateColorBrush.h"
#include "TimerManager.h"

static const FButtonStyle& NRButtonStyle()
{
 static FButtonStyle S=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.055,.071,.078)))
 .SetHovered(FSlateColorBrush(FLinearColor(.13,.19,.20))).SetPressed(FSlateColorBrush(FLinearColor(.23,.31,.22)))
 .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));return S;
}
void ANRHUD::BeginPlay()
{
 Super::BeginPlay();if(GetNetMode()==NM_DedicatedServer||!GEngine||!GEngine->GameViewport)return;
 const FLinearColor White(.9,.93,.91),Muted(.45,.54,.55),Lime(.8,.93,.35),Cyan(.25,.76,.82),Panel(.022,.033,.039,.98);
 auto Text=[](const FString& S,int Size,FLinearColor Color,bool Bold=false){return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Size)).ColorAndOpacity(Color);};
 auto Button=[&](const FString& S,TFunction<FReply()> Action,bool Primary=false)
 {
  return SNew(SButton).ButtonStyle(&NRButtonStyle()).ContentPadding(FMargin(22,12)).OnClicked_Lambda([Action](){return Action();})
   [Text(S,14,Primary?Lime:White,true)];
 };
 TWeakObjectPtr<ANRHUD> Self=this;
 auto ClassCard=[&](int Index,const FString& Name,const FString& Ability,const FString& Description)
 {
  return SNew(SButton).ButtonStyle(&NRButtonStyle()).ContentPadding(0).OnClicked_Lambda([Self,Index](){if(auto* H=Self.Get())if(auto* C=Cast<ANRCharacter>(H->GetOwningPawn()))C->SelectClass(ENRRole(Index));return FReply::Handled();})[
   SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(FMargin(2))
   .BorderBackgroundColor_Lambda([Self,Index,Lime](){auto* H=Self.Get();auto* C=H?Cast<ANRCharacter>(H->GetOwningPawn()):nullptr;return C&&int(C->OperatorRole)==Index?Lime:FLinearColor(.12,.17,.18);})[
    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(20)[SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()[Text(FString::Printf(TEXT("0%d / СПЕЦИАЛИЗАЦИЯ"),Index+1),9,Muted)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,6)[Text(Name,16,White,true)]
    +SVerticalBox::Slot().AutoHeight()[Text(Ability,10,Lime,true)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,18,0,0)[Text(Description,11,Muted)]
   ]]];
 };
 SAssignNew(Overlay,SOverlay)
 +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1600).HeightOverride(900)[SNew(SNRVisorWidget).HUD(this)]]]
 +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.006,.011,.016,.94))
  .Visibility_Lambda([Self](){auto* H=Self.Get();return H&&H->bMenuOpen?EVisibility::Visible:EVisibility::Collapsed;})[
  SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1420).HeightOverride(800)[SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight().Padding(34,26)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("NULL / ROUTE"),25,White,true)]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Text(TEXT("SWITCHYARD    /    УЗЕЛ СВЯЗИ ECHELON"),11,Muted)]]
   +SVerticalBox::Slot().FillHeight(1).Padding(34,30)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(.34).Padding(0,0,62,0)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[Text(TEXT("СОХРАНИТЬ\nЭЛАРУ."),43,White,true)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,32)[Text(TEXT("Войдите в аркологию. Перехватите сеть.\nИзвлеките ядро до отключения сектора."),13,Muted)]
     +SVerticalBox::Slot().AutoHeight()[Button(TEXT("ПРОДОЛЖИТЬ ОПЕРАЦИЮ   →"),[Self](){if(auto* H=Self.Get())H->ToggleMenu();return FReply::Handled();},true)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,8)[SNew(SButton).ButtonStyle(&NRButtonStyle()).ContentPadding(FMargin(12,8)).OnClicked_Lambda([Self](){if(auto* H=Self.Get())H->TrainingFaction=(H->TrainingFaction+1)%NRFactions::Count;return FReply::Handled();})[SNew(STextBlock).Text_Lambda([Self](){auto* H=Self.Get();return FText::FromString(FString(TEXT("ФРАКЦИЯ: "))+NRFactions::Name(ENRFaction(H?H->TrainingFaction:2))+TEXT("  →"));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",10)).ColorAndOpacity(Cyan)]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,8)[Button(TEXT("ТРЕНИРОВКА: АТАКА"),[Self](){if(auto* H=Self.Get()){UGameplayStatics::SetGamePaused(H,false);UGameplayStatics::OpenLevel(H,FName(*UGameplayStatics::GetCurrentLevelName(H,true)),true,H->TrainingOptions(false));}return FReply::Handled();})]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("ТРЕНИРОВКА: ОБОРОНА"),[Self](){if(auto* H=Self.Get()){UGameplayStatics::SetGamePaused(H,false);UGameplayStatics::OpenLevel(H,FName(*UGameplayStatics::GetCurrentLevelName(H,true)),true,H->TrainingOptions(true));}return FReply::Handled();})]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("МАСТЕРСКАЯ ОРУЖИЯ  [ B ]"),[Self](){if(auto* H=Self.Get())if(auto* C=Cast<ANRCharacter>(H->GetOwningPawn()))C->ToggleWorkbench();return FReply::Handled();})]
     +SVerticalBox::Slot().AutoHeight()[Button(TEXT("ВЫЙТИ ИЗ ИГРЫ"),[Self](){if(auto* H=Self.Get())UKismetSystemLibrary::QuitGame(H,H->GetOwningPlayerController(),EQuitPreference::Quit,false);return FReply::Handled();})]
     +SVerticalBox::Slot().FillHeight(1)
     +SVerticalBox::Slot().AutoHeight()[Text(TEXT("ВАША ЗАДАЧА"),9,Lime,true)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,12)[Text(TEXT("01   Найти ядро Элары\n02   Забрать ядро клавишей F\n03   Доставить его к янтарному шлюзу"),13,White)]
    ]
    +SHorizontalBox::Slot().FillWidth(.66)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("ВЫБОР ОПЕРАТОРА"),15,White,true)]
      +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([Self](){auto* H=Self.Get();auto* S=H?H->GetWorld()->GetGameState<ANRGameState>():nullptr;return FText::FromString(S&&S->Phase==ENRPhase::Preparation?TEXT("ДОСТУПЕН ВЫБОР"):TEXT("СМЕНА В СЛЕДУЮЩЕМ РАУНДЕ"));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",9)).ColorAndOpacity(Lime)]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,16)[SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,10,0)[ClassCard(0,TEXT("ИНЖЕНЕР"),TEXT("ПЕЧАТЬ И ЛОГИКА"),TEXT("MX–9\nСтройте узлы.\nСоединяйте защиту."))]
      +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,10,0)[ClassCard(1,TEXT("РАЗВЕДЧИК"),TEXT("СЕТЕВОЙ ПЕРЕХВАТ"),TEXT("Spectre\nЧитайте пакеты.\nЗахватывайте сеть."))]
      +SHorizontalBox::Slot().FillWidth(1)[ClassCard(2,TEXT("ШТУРМОВИК"),TEXT("КИНЕТИЧЕСКИЙ СНОС"),TEXT("BR–74\nРазрушайте панели.\nОткрывайте проходы."))]
      +SHorizontalBox::Slot().FillWidth(1).Padding(10,0,0,0)[ClassCard(3,TEXT("МЕДИК"),TEXT("ПОЛЕВОЕ ЛЕЧЕНИЕ"),TEXT("MX–9 / поддержка\nZ: +40 здоровья.\nСоюзник / себе."))]]
     +SVerticalBox::Slot().AutoHeight()[Button(TEXT("СЕТЕВЫЕ МАТЧИ / БРАУЗЕР СЕРВЕРОВ   →"),[Self](){if(auto* H=Self.Get())H->ToggleServers();return FReply::Handled();},true)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,4)[SNew(STextBlock).Text_Lambda([Self](){auto* H=Self.Get();auto* B=H?H->GetGameInstance()->GetSubsystem<UNRServerBrowser>():nullptr;return FText::FromString(B?B->Status:TEXT(""));}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",10)).ColorAndOpacity(Cyan)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,8)[Text(TEXT("КАЧЕСТВО ГРАФИКИ"),10,Muted,true)]
     +SVerticalBox::Slot().AutoHeight()[SAssignNew(QualityButtons,SHorizontalBox)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,6)[SNew(STextBlock).Text_Lambda([Self](){auto* H=Self.Get();auto* C=H?Cast<ANRCharacter>(H->GetOwningPawn()):nullptr;return FText::FromString(C&&C->OperatorRole==ENRRole::Assault?TEXT("ШТУРМОВИК: ПОКРЫТИЕ ЗАДАЁТ ФРАКЦИЯ"):TEXT("ПОКРЫТИЕ ОРУЖИЯ  /  I — ОСМОТР В ИГРЕ"));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",10)).ColorAndOpacity(Muted)]
     +SVerticalBox::Slot().AutoHeight()[SAssignNew(SkinButtons,SHorizontalBox).IsEnabled_Lambda([Self](){auto* H=Self.Get();auto* C=H?Cast<ANRCharacter>(H->GetOwningPawn()):nullptr;return C&&C->OperatorRole!=ENRRole::Assault;})]
     +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[Text(TEXT("WASD  движение     МЫШЬ  обзор     SPACE  прыжок\n1  основное     2  пистолет     3  клинок / ЛКМ удар\nSHIFT  спринт   CTRL  присесть   R  магазин   I  осмотр\nALT  тихий шаг     Q / E  наклоны\nZ  устройство     F  взаимодействие     T  логический граф\nСКМ  командная метка / опасность"),10,Muted)]
    ]
   ]
   +SVerticalBox::Slot().AutoHeight().Padding(34,15)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("N / R     ТАКТИЧЕСКАЯ СЕТЕВАЯ ОПЕРАЦИЯ"),9,Muted)]
    +SHorizontalBox::Slot().AutoWidth()[Text(TEXT("ESC  ВЕРНУТЬСЯ В ИГРУ"),10,Lime)]]
  ]]]]
 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(28).BorderBackgroundColor(Panel)
  .Visibility_Lambda([Self](){auto* H=Self.Get();return H&&H->bGraphOpen?EVisibility::Visible:EVisibility::Collapsed;})[
  SNew(SVerticalBox)
  +SVerticalBox::Slot().AutoHeight()[Text(TEXT("КОНСОЛЬ ЛОГИЧЕСКОЙ СЕТИ"),24,White,true)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,16)[SNew(SNRGraphWidget).Character(Cast<ANRCharacter>(GetOwningPawn()))]
  +SVerticalBox::Slot().AutoHeight()[Text(TEXT("ЛКМ: источник → получатель. ПКМ: смена операции. Циклические связи запрещены."),12,Muted)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[Button(TEXT("ВЕРНУТЬСЯ В ИГРУ   [ T ]"),[Self](){if(auto* H=Self.Get())H->ToggleGraph();return FReply::Handled();},true)]]];
 StaticCastSharedPtr<SOverlay>(Overlay)->AddSlot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.006,.01,.016,.92))
 .Visibility_Lambda([Self](){return Self.IsValid()&&Self->bWorkbenchOpen?EVisibility::Visible:EVisibility::Collapsed;})[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1440).HeightOverride(840)[SAssignNew(Workbench,SNRWorkbenchWidget).HUD(this)]]]];
 const TCHAR* QualityNames[]={TEXT("НИЗКОЕ"),TEXT("СРЕДНЕЕ"),TEXT("ВЫСОКОЕ"),TEXT("УЛЬТРА")};
 for(int I=0;I<4;++I)QualityButtons->AddSlot().FillWidth(1).Padding(0,0,8,0)[SNew(SButton).ButtonStyle(&NRButtonStyle()).ContentPadding(FMargin(12,13))
  .OnClicked_Lambda([I](){if(auto* S=UGameUserSettings::GetGameUserSettings()){S->SetOverallScalabilityLevel(I);S->ApplyNonResolutionSettings();S->SaveSettings();}return FReply::Handled();})[
  SNew(STextBlock).Text(FText::FromString(QualityNames[I])).Font(FCoreStyle::GetDefaultFontStyle("Bold",10))
  .ColorAndOpacity_Lambda([I,Lime,Muted](){auto* S=UGameUserSettings::GetGameUserSettings();return FSlateColor(S&&S->GetOverallScalabilityLevel()==I?Lime:Muted);})]];
 const TCHAR* Skins[]={TEXT("КАРБОН"),TEXT("АРКТИКА"),TEXT("АВАРИЙНЫЙ"),TEXT("ФАНТОМ")};
 const FLinearColor Swatches[]={FLinearColor(.13,.18,.20),FLinearColor(.78,.82,.75),FLinearColor(.9,.39,.025),FLinearColor(.38,.12,.70)};
 for(int I=0;I<4;++I)SkinButtons->AddSlot().FillWidth(1).Padding(0,0,8,0)[SNew(SButton).ButtonStyle(&NRButtonStyle()).ContentPadding(FMargin(12,10))
  .OnClicked_Lambda([Self,I](){if(auto* H=Self.Get())if(auto* C=Cast<ANRCharacter>(H->GetOwningPawn())){C->SelectSkin(I);if(auto* FX=H->GetWorld()->GetSubsystem<UNRCombatFX>())FX->Sound(TEXT("S_UI"),C->GetActorLocation(),.3f,true);}return FReply::Handled();})[
  SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SBox).WidthOverride(11).HeightOverride(11)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Swatches[I])]]
  +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(Skins[I])).Font(FCoreStyle::GetDefaultFontStyle("Bold",9))
  .ColorAndOpacity_Lambda([Self,I,Lime,Muted](){auto* H=Self.Get();auto* C=H?Cast<ANRCharacter>(H->GetOwningPawn()):nullptr;return FSlateColor(C&&C->WeaponSkin==I?Lime:Muted);})]]];
 StaticCastSharedPtr<SOverlay>(Overlay)->AddSlot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.005,.01,.015,.98))
 .Visibility_Lambda([Self](){return Self.IsValid()&&Self->bServersOpen?EVisibility::Visible:EVisibility::Collapsed;})[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1320).HeightOverride(800)[SAssignNew(ServerBrowser,SNRServerBrowserWidget).Browser(GetGameInstance()->GetSubsystem<UNRServerBrowser>()).Player(GetOwningPlayerController()).OnClose(FSimpleDelegate::CreateWeakLambda(this,[this](){ToggleServers();}))]]]];
 if(auto* C=Cast<ANRCharacter>(GetOwningPawn()))TrainingFaction=int(C->GetFaction());
 GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef());
 const bool Test=FParse::Param(FCommandLine::Get(),TEXT("NREvolutionNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NREvolutionTest"))||FParse::Param(FCommandLine::Get(),TEXT("NREvolutionVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerSystemTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerJoinTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerLifecycle"))||FParse::Param(FCommandLine::Get(),TEXT("NRServerVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRPreparationTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRPreparationNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRPreparationVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NROperationsTest"))||FParse::Param(FCommandLine::Get(),TEXT("NROperationsNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NROperationsVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRStanceTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRStanceNetwork"))||FParse::Param(FCommandLine::Get(),TEXT("NRStanceVisual"))||FParse::Param(FCommandLine::Get(),TEXT("NRDamageAudioTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRModuleTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRExpansionTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRDetailTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRSmokeTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRVisualTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRMatchTest"))||FParse::Param(FCommandLine::Get(),TEXT("NRCollisionTest"));
 bMenuOpen=GetNetMode()==NM_Standalone&&!Test&&!GetWorld()->URL.HasOption(TEXT("SkipMenu"));ApplyInputMode();
 GetWorldTimerManager().SetTimer(ContextTimer,[this](){CachedContext=ContextText();CachedScan=ScanText();RefreshMission();RefreshOperations();},.1f,true);
}
void ANRHUD::ApplyInputMode()
{
    auto* PC=GetOwningPlayerController();if(!PC)return;
    PC->bShowMouseCursor=bMenuOpen||bGraphOpen||bWorkbenchOpen||bServersOpen;
    PC->SetIgnoreLookInput(false);PC->SetIgnoreMoveInput(false);
    if(bMenuOpen||bGraphOpen||bWorkbenchOpen||bServersOpen)
    {
        FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);
        PC->SetIgnoreLookInput(true);PC->SetIgnoreMoveInput(true);
    }
    else PC->SetInputMode(FInputModeGameOnly());
    if(GetNetMode()==NM_Standalone&&!FParse::Param(FCommandLine::Get(),TEXT("NRVisualTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("NRServerVisual")))UGameplayStatics::SetGamePaused(this,bMenuOpen||bWorkbenchOpen||bServersOpen);
}
void ANRHUD::ToggleMenu(){if(bServersOpen){ToggleServers();return;}if(bWorkbenchOpen){bWorkbenchOpen=false;bMenuOpen=false;}else bMenuOpen=!bMenuOpen;bGraphOpen=false;ApplyInputMode();}
void ANRHUD::ToggleWorkbench(){bWorkbenchOpen=!bWorkbenchOpen;bMenuOpen=false;bGraphOpen=false;if(bWorkbenchOpen&&Workbench.IsValid())Workbench->Refresh();ApplyInputMode();}
void ANRHUD::ToggleGraph(){auto* P=Cast<ANRCharacter>(GetOwningPawn());if(!P||P->OperatorRole!=ENRRole::Engineer)return;bGraphOpen=!bGraphOpen;bMenuOpen=false;bWorkbenchOpen=false;ApplyInputMode();}
void ANRHUD::EndPlay(EEndPlayReason::Type R){GetWorldTimerManager().ClearTimer(ContextTimer);if(Overlay.IsValid()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());ServerBrowser.Reset();Workbench.Reset();Overlay.Reset();Super::EndPlay(R);}

FText ANRHUD::ContextText()const
{
    auto* C=Cast<ANRCharacter>(GetOwningPawn());if(!C)return FText::GetEmpty();
    if(C->bDead)return FText::FromString(TEXT("СИГНАЛ ПОТЕРЯН"));
    if(C->bCarryingDisk)return FText::FromString(TEXT("ЭЛАРА У ВАС  /  F у точки эвакуации"));
    FHitResult Hit;FVector O;FRotator R;GetOwningPlayerController()->GetPlayerViewPoint(O,R);FCollisionQueryParams Q;Q.AddIgnoredActor(C);
    if(GetWorld()->LineTraceSingleByChannel(Hit,O,O+R.Vector()*1500,ECC_Visibility,Q))
    {
        if(auto* Prep=Cast<ANRPreparationProp>(Hit.GetActor()))return FText::FromString(Prep->Prompt());
        if(auto* Door=Cast<ANRTacticalDoor>(Hit.GetActor()))return FText::FromString(Door->Prompt());
        if(auto* A=Cast<ANRActivity>(Hit.GetActor()))return FText::FromString(A->Prompt());
        if(auto* N=Cast<ANodeBase>(Hit.GetActor()))return FText::FromString(N->Team==255?TEXT("F  ЗАХВАТИТЬ НЕЙТРАЛЬНЫЙ УЗЕЛ"):N->Team==C->Team?TEXT("X  СВЯЗЬ    G  ОПЕРАЦИЯ    T  ГРАФ"):TEXT("ВРАЖЕСКИЙ УЗЕЛ  /  Z сканирование"));
        if(auto* N=Cast<ANRObjective>(Hit.GetActor()))return FText::FromString(N->bExtraction?TEXT("F  ЭВАКУАЦИЯ ЭЛАРЫ"):TEXT("F  ЗАБРАТЬ ЭЛАРУ"));
    }
    auto* S=GetWorld()->GetGameState<ANRGameState>();if(S&&S->Phase==ENRPhase::Preparation&&C->OperatorRole==ENRRole::Scout)return FText::FromString(TEXT("ПЕРЕХВАТ СЕТИ ДОСТУПЕН ПОСЛЕ НАЧАЛА РАУНДА"));float Cool=S?FMath::Max(0.f,C->AbilityReadyTime-S->GetServerWorldTimeSeconds()):0;
    if(S&&S->Phase==ENRPhase::Preparation&&C->OperatorRole==ENRRole::Medic)return FText::FromString(TEXT("ЛЕЧЕНИЕ ДОСТУПНО С НАЧАЛА РАУНДА"));
    if(Cool>0)return FText::FromString(FString::Printf(TEXT("До готовности устройства: %.1f с"),Cool));
    const TCHAR* T[]={TEXT("T  КОНСОЛЬ ГРАФА    X  СВЯЗАТЬ УЗЛЫ"),TEXT("ДАЛЬНОСТЬ СКАНИРОВАНИЯ  /  15 м"),TEXT("УСТАНОВКА НА ПАНЕЛЬ  /  2,5 м"),TEXT("Z  ЛЕЧИТЬ СОЮЗНИКА ДО 6 м / СЕБЯ  •  +40 HP")};
    return FText::FromString(T[uint8(C->OperatorRole)]);
}
FText ANRHUD::ScanText()const
{
    auto* C=Cast<ANRCharacter>(GetOwningPawn());auto* S=GetWorld()->GetGameState<ANRGameState>();if(!C||!S||C->ScanResult.ExpiresAt<S->GetServerWorldTimeSeconds())return FText::GetEmpty();
    return FText::FromString(FString::Printf(TEXT("ПЕРЕХВАТ ПАКЕТОВ\n%s\nКОМАНДА %d  /  СВЯЗЕЙ %d\nЗАДЕРЖКА %.1f с\nПОДМЕНА MAC АКТИВНА"),*C->ScanResult.NodeId.ToString().Left(8),C->ScanResult.Team,C->ScanResult.LinkCount,C->ScanResult.Delay));
}

void ANRHUD::RefreshMission()
{
 auto* C=Cast<ANRCharacter>(GetOwningPawn());if(!C)return;
 const auto* State=GetWorld()->GetGameState<ANRGameState>();
 if(State&&State->Phase==ENRPhase::Preparation)
 {
  const bool Attack=C->Team==State->AttackTeam;
  const FVector Goal=Attack?FVector(ANRGameState::PreparationBoundaryX,0,90):FVector(1850,0,90);
  const FVector Delta=Goal-C->GetActorLocation();GuideYaw=Delta.Rotation().Yaw;GuideMeters=Delta.Size()/100;InteractionProgress=0;
  MissionGuide=Attack?TEXT("ШЛЮЗ АТАКИ / ЗАКРЫТ"):TEXT("ЗАЩИТИТЕ ЯДРО");
  ActivityGuide=FString::Printf(TEXT("ПОПАДАНИЯ: %d  /  F У ТЕРМИНАЛА: ПАТРОНЫ"),C->PracticeHits);return;
 }
 MissionGuide=TEXT("ЯДРО ПЕРЕНОСИТ ДРУГОЙ ОПЕРАТОР");GuideMeters=0;InteractionProgress=0;
 for(TActorIterator<ANRObjective> It(GetWorld());It;++It)if(It->bExtraction==C->bCarryingDisk&&!It->bTaken)
 {const FVector Delta=It->GetActorLocation()-C->GetActorLocation();GuideYaw=Delta.Rotation().Yaw;GuideMeters=Delta.Size()/100;MissionGuide=C->bCarryingDisk?TEXT("ШЛЮЗ ЭВАКУАЦИИ"):TEXT("ЯДРО ЭЛАРЫ");break;}
 int Supply=0,Targets=0,Relays=0;for(TActorIterator<ANRActivity> It(GetWorld());It;++It)if(!It->bCompleted){Supply+=It->Kind==ENRActivityKind::Supply;Targets+=It->Kind==ENRActivityKind::Target;Relays+=It->Kind==ENRActivityKind::Relay;}
 ActivityGuide=FString::Printf(TEXT("СНАБЖЕНИЕ %d   /   РЕЛЕ %d   /   МИШЕНИ %d"),Supply,Relays,Targets);
 FHitResult H;FVector Eye;FRotator Aim;GetOwningPlayerController()->GetPlayerViewPoint(Eye,Aim);FCollisionQueryParams Q;Q.AddIgnoredActor(C);
 if(GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+Aim.Vector()*1000,ECC_Visibility,Q))if(auto* A=Cast<ANRActivity>(H.GetActor()))InteractionProgress=A->Progress;
}

void ANRHUD::ToggleServers(){bServersOpen=!bServersOpen;bMenuOpen=!bServersOpen;bGraphOpen=false;bWorkbenchOpen=false;if(bServersOpen)if(auto* B=GetGameInstance()->GetSubsystem<UNRServerBrowser>())B->Refresh();ApplyInputMode();}
