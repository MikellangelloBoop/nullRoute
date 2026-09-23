#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Brushes/SlateColorBrush.h"
#include "NRCharacter.h"
#include "NRHUD.h"

class SNRWorkbenchWidget:public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SNRWorkbenchWidget){} SLATE_ARGUMENT(ANRHUD*,HUD) SLATE_END_ARGS()
 void Construct(const FArguments& A)
 {
  HUD=A._HUD;const FLinearColor Ink(.025,.035,.041),White(.91,.94,.92),Muted(.48,.59,.61),Lime(.8,.93,.35),Cyan(.25,.76,.82);
  auto Text=[&](FString S,int Size,FLinearColor Color){return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color);};
  auto Button=[&](FString S,TFunction<void()> Action){return SNew(SButton).ButtonStyle(&Style()).ContentPadding(FMargin(24,16)).OnClicked_Lambda([Action](){Action();return FReply::Handled();})[Text(S,13,Lime)];};
  TSharedPtr<SVerticalBox> Rows;TSharedPtr<SHorizontalBox> Tabs,Arsenal;
  ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(32)[SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("N / R     ОРУЖЕЙНАЯ МАСТЕРСКАЯ"),25,White)]
    +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("ЗАКРЫТЬ  [ B / ESC ]"),[this](){if(HUD.IsValid())HUD->ToggleWorkbench();})]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,20)[Text(TEXT("Соберите оружие под свой маршрут. Настройка доступна во время подготовки."),12,Muted)]
   +SVerticalBox::Slot().AutoHeight()[SAssignNew(Tabs,SHorizontalBox)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[SAssignNew(Arsenal,SHorizontalBox).Visibility_Lambda([this](){return Slot==0?EVisibility::Visible:EVisibility::Collapsed;})]
   +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,8)[SNew(STextBlock).Text_Lambda([this](){auto* C=Character();const TCHAR* Names[]={TEXT("MX–9"),TEXT("SPECTRE"),TEXT("BR–74")};return FText::FromString(FString::Printf(TEXT("%s  /  КОНФИГУРАЦИЯ"),Slot==1?TEXT("P–12"):C?NRWeaponModules::WeaponName(C->PrimaryKind()):TEXT("ОСНОВНОЕ")));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",18)).ColorAndOpacity(Cyan)]
   +SVerticalBox::Slot().AutoHeight()[SAssignNew(Rows,SVerticalBox)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,8)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.045,.067,.072)).Padding(18)[SNew(STextBlock).Text_Lambda([this](){return StatsText();}).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(White)]]
   +SVerticalBox::Slot().AutoHeight()[Text(TEXT("Урон указан до брони. Замена не пополняет боезапас. Лишние патроны возвращаются в резерв."),10,Muted)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&Style()).ContentPadding(FMargin(25,18)).IsEnabled_Lambda([this](){auto* C=Character();return C&&C->CanCustomize();}).OnClicked_Lambda([this](){if(auto* C=Character())C->ConfigureWeapon(Slot,Draft[Slot]);return FReply::Handled();})[Text(TEXT("УСТАНОВИТЬ КОМПЛЕКТ   →"),14,Lime)]]
    +SHorizontalBox::Slot().AutoWidth().Padding(10,0)[Button(TEXT("ШТАТНЫЙ КОМПЛЕКТ"),[this](){Draft[Slot]={};})]
    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(12,0)[SNew(STextBlock).Text_Lambda([this](){auto* C=Character();return FText::FromString(!C?TEXT(""):!C->CanCustomize()?TEXT("Замена заблокирована до подготовки"):C->WorkshopStatus);}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",11)).ColorAndOpacity(Cyan)]]
  ]];
  for(int I=0;I<2;++I)Tabs->AddSlot().FillWidth(1).Padding(0,0,I==0?10:0,0)[SNew(SButton).ButtonStyle(&Style()).ContentPadding(FMargin(18,13)).OnClicked_Lambda([this,I](){Slot=I;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(I==0?TEXT("01   ОСНОВНОЕ ОРУЖИЕ"):TEXT("02   ПИСТОЛЕТ P–12"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",13)).ColorAndOpacity_Lambda([this,I,Lime,Muted](){return FSlateColor(Slot==I?Lime:Muted);})]];
  for(int I=0;I<6;++I)Arsenal->AddSlot().FillWidth(1).Padding(0,0,5,0)[SNew(SButton).ButtonStyle(&Style()).ContentPadding(FMargin(8,13))
   .IsEnabled_Lambda([this](){auto* C=Character();return C&&C->CanCustomize();})
   .ToolTipText(FText::FromString(NRWeaponModules::WeaponDescription(ENRPrimaryWeapon(I))))
   .OnClicked_Lambda([this,I](){if(auto* C=Character())C->ChoosePrimary(ENRPrimaryWeapon(I));return FReply::Handled();})[
    SNew(STextBlock).Text(FText::FromString(NRWeaponModules::WeaponName(ENRPrimaryWeapon(I)))).Font(FCoreStyle::GetDefaultFontStyle("Bold",11))
    .ColorAndOpacity_Lambda([this,I,Lime,White](){auto* C=Character();return FSlateColor(C&&int(C->PrimaryWeapon)==I?Lime:White);})]];
  const TCHAR* Titles[]={TEXT("01 / ПРИЦЕЛ"),TEXT("02 / ДУЛЬНЫЙ МОДУЛЬ"),TEXT("03 / МАГАЗИН")};
  for(uint8 Socket=0;Socket<3;++Socket)
  {
   TSharedPtr<SHorizontalBox> Options;
   Rows->AddSlot().AutoHeight().Padding(0,5)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(196)[Text(Titles[Socket],11,Cyan)]]
    +SHorizontalBox::Slot().FillWidth(1)[SAssignNew(Options,SHorizontalBox)]];
   for(uint8 Choice=0;Choice<3;++Choice)Options->AddSlot().FillWidth(1).Padding(0,0,Choice<2?8:0,0)[SNew(SButton).ButtonStyle(&Style()).ContentPadding(0)
    .IsEnabled_Lambda([this,Socket,Choice](){return !(Slot==1&&Socket==0&&Choice==2);})
    .OnClicked_Lambda([this,Socket,Choice](){SetChoice(Socket,Choice);return FReply::Handled();})[
    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2).BorderBackgroundColor_Lambda([this,Socket,Choice,Lime](){return Selected(Socket)==Choice?Lime:FLinearColor(.12,.18,.19);})[
    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(FMargin(15,12))[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[Text(NRWeaponModules::Name(Socket,Choice),12,White)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[Text(NRWeaponModules::Description(Socket,Choice),10,Muted)]
    ]]]];
  }
 }
 void Refresh()
 {
  if(auto* C=Character()){Draft[0]=C->Assembly(0);Draft[1]=C->Assembly(1);Slot=C->EquippedWeapon==1?1:0;C->WorkshopStatus=TEXT("Выберите модули и нажмите «Установить комплект».");}
 }
private:
 static const FButtonStyle& Style(){static const FButtonStyle S=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.05,.08,.085))).SetHovered(FSlateColorBrush(FLinearColor(.11,.19,.20))).SetPressed(FSlateColorBrush(FLinearColor(.22,.28,.16))).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));return S;}
 ANRCharacter* Character()const{return HUD.IsValid()?Cast<ANRCharacter>(HUD->GetOwningPawn()):nullptr;}
 uint8 Selected(uint8 Socket)const{return Socket==0?uint8(Draft[Slot].Optic):Socket==1?uint8(Draft[Slot].Muzzle):uint8(Draft[Slot].Magazine);}
 void SetChoice(uint8 Socket,uint8 Choice){if(Socket==0)Draft[Slot].Optic=ENROptic(Choice);else if(Socket==1)Draft[Slot].Muzzle=ENRMuzzle(Choice);else Draft[Slot].Magazine=ENRMagazine(Choice);}
 FText StatsText()const
 {
  auto* C=Character();if(!C)return FText::GetEmpty();const auto S=NRWeaponModules::Resolve(C->OperatorRole,Slot,Draft[Slot],C->PrimaryWeapon);const auto Current=C->WeaponStats(Slot);
  return FText::FromString(FString::Printf(TEXT("УРОН  %.1f → %.1f       МАГАЗИН  %d → %d       ПЕРЕЗАРЯДКА  %.2f → %.2f с\n\nОТДАЧА  %.0f%%       СКОРОСТЬ ПУЛИ  %.0f м/с       ПРИЦЕЛ  %.0f° / ДРОБИН %d / %.0f ВЫСТР/МИН"),Current.Damage,S.Damage,Current.Capacity,S.Capacity,Current.ReloadSeconds,S.ReloadSeconds,S.RecoilScale*100,S.Speed/100,S.AimFOV,S.Pellets,60/S.Interval));
 }
 TWeakObjectPtr<ANRHUD> HUD;uint8 Slot=0;FNRWeaponAssembly Draft[2];
};
