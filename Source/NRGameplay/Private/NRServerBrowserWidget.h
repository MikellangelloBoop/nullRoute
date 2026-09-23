#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "NRServerBrowser.h"
#include "GameFramework/PlayerController.h"

class SNRServerBrowserWidget : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SNRServerBrowserWidget){} SLATE_ARGUMENT(UNRServerBrowser*,Browser) SLATE_ARGUMENT(APlayerController*,Player) SLATE_EVENT(FSimpleDelegate,OnClose) SLATE_END_ARGS()
 void Construct(const FArguments& A)
 {
  Browser=A._Browser;Player=A._Player;Close=A._OnClose;
  const FLinearColor White(.9,.94,.94),Muted(.45,.56,.58),Cyan(.25,.78,.85),Lime(.8,.93,.35),Ink(.02,.032,.04,.99);
  auto Text=[&](const FString& S,int Size,FLinearColor Color){return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color);};
  ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(32)[SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("СЕТЕВЫЕ ОПЕРАЦИИ"),28,White)]
    +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("НАЗАД  /  ESC"),[this](){Close.ExecuteIfBound();})]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,22)[Text(TEXT("LAN  /  ВЫДЕЛЕННЫЕ СЕРВЕРЫ  /  СОХРАНЁННЫЕ АДРЕСА"),11,Cyan)]
   +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,12,0)[SAssignNew(Address,SEditableTextBox).Text(FText::FromString(TEXT("201.51.19.38:7777"))).HintText(FText::FromString(TEXT("IP или домен:порт"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",18))]
    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)[Button(TEXT("ПОДКЛЮЧИТЬСЯ"),[this](){if(auto* B=Browser.Get())B->Join(Player.Get(),Address->GetText().ToString());})]
    +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("СОХРАНИТЬ"),[this](){if(auto* B=Browser.Get())if(B->AddFavorite(Address->GetText().ToString()))B->Refresh();})]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,14)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[Button(TEXT("ОБНОВИТЬ СПИСОК"),[this](){if(auto* B=Browser.Get())B->Refresh();})]
    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[Button(TEXT("ЗАПУСТИТЬ СВОЙ СЕРВЕР"),[this](){if(auto* B=Browser.Get())B->StartLocalServer();})]
    +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("ОСТАНОВИТЬ СВОЙ"),[this](){if(auto* B=Browser.Get())B->StopLocalServer();})]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Browser.IsValid()?Browser->Status:TEXT("Сервис недоступен"));}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).ColorAndOpacity(Lime)]
   +SVerticalBox::Slot().AutoHeight()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.055,.08,.09)).Padding(FMargin(14,10))[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("СЕРВЕР / КАРТА"),10,Muted)]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(100)[Text(TEXT("ИГРОКИ"),10,Muted)]]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(85)[Text(TEXT("ПИНГ"),10,Muted)]]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(190)[Text(TEXT("ФАЗА"),10,Muted)]]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(185)[Text(TEXT("ДЕЙСТВИЯ"),10,Muted)]]]]
   +SVerticalBox::Slot().FillHeight(1).Padding(0,8,0,16)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Rows,SVerticalBox)]]
   +SVerticalBox::Slot().AutoHeight()[Text(TEXT("Вход в бой после старта раунда — со следующей подготовки. Полный матч: до 10 игроков."),11,Muted)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Text(TEXT("Свой сервер работает отдельным процессом, пока открыта игра. Для постоянного хостинга используйте StartServer.ps1."),10,Muted)]
  ]];
  if(Browser.IsValid())Browser->Refresh();Rebuild();
 }
 virtual void Tick(const FGeometry& G,const double T,const float D)override
 {SCompoundWidget::Tick(G,T,D);if(Browser.IsValid()&&Revision!=Browser->Revision)Rebuild();}
private:
 TSharedRef<SWidget> Button(const FString& Text,TFunction<void()> Action)
 {
  static const FButtonStyle Style=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.065,.1,.12))).SetHovered(FSlateColorBrush(FLinearColor(.13,.22,.24))).SetPressed(FSlateColorBrush(FLinearColor(.2,.3,.25)));
  return SNew(SButton).ButtonStyle(&Style).ContentPadding(FMargin(14,12)).OnClicked_Lambda([Action](){Action();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Bold",10)).ColorAndOpacity(FLinearColor(.85,.95,.83))];
 }
 void Rebuild()
 {
  if(!Rows||!Browser.IsValid())return;Revision=Browser->Revision;Rows->ClearChildren();TArray<FNRServerEntry> Entries=Browser->GetServers();
  Entries.Sort([](const auto& A,const auto& B){if(A.bOnline!=B.bOnline)return A.bOnline;if(A.bFavorite!=B.bFavorite)return A.bFavorite;return A.Address<B.Address;});
  for(const auto& Entry:Entries)
  {
   auto Text=[](const FString& S,int Size,FLinearColor Color){return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color);};
   const FLinearColor Color=Entry.bOnline?FLinearColor(.86,.94,.92):FLinearColor(.4,.5,.54);
   Rows->AddSlot().AutoHeight().Padding(0,0,0,6)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.03,.047,.056)).Padding(14)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[Text((Entry.bFavorite?TEXT("★ "):Entry.bOfficial?TEXT("◆ "):TEXT(""))+Entry.Name,15,Color)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[Text(Entry.Address+TEXT("  /  ")+Entry.Map,10,FLinearColor(.4,.59,.63))]]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(100)[Text(Entry.bOnline?FString::Printf(TEXT("%d / %d"),Entry.Players,Entry.Capacity):TEXT("—"),16,Color)]]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(85)[Text(Entry.Ping>=0?FString::Printf(TEXT("%d мс"),Entry.Ping):TEXT("—"),13,Color)]]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(190)[Text(Entry.bOnline&&!Entry.bCompatible?TEXT("Другая версия"):Entry.Phase,11,Color)]]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(185)[SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[Button(Entry.bFavorite?TEXT("−"):TEXT("+"),[this,Entry](){if(auto* B=Browser.Get()){if(Entry.bFavorite)B->RemoveFavorite(Entry.Address);else B->AddFavorite(Entry.Address);}})]
     +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("ВОЙТИ"),[this,Entry](){if(auto* B=Browser.Get())B->Join(Player.Get(),Entry.Address);})]]]]];
  }
  if(Entries.IsEmpty())Rows->AddSlot().AutoHeight().Padding(16,30)[SNew(STextBlock).Text(FText::FromString(TEXT("Серверы пока не найдены. Запустите свой сервер или сохраните адрес друга."))).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14))];
 }
 TWeakObjectPtr<UNRServerBrowser> Browser;
 TWeakObjectPtr<APlayerController> Player;
 TSharedPtr<SEditableTextBox> Address;
 TSharedPtr<SVerticalBox> Rows;
 FSimpleDelegate Close;
 uint32 Revision=MAX_uint32;
};
