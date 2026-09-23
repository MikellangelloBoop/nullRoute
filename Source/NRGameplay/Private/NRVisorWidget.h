#pragma once
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "NRHUD.h"
#include "NRCharacter.h"
#include "NRAttributes.h"
#include "NRGameMode.h"
#include "GameFramework/PlayerController.h"

// One paint pass, no replicated UI actors, no Blueprint ticks, no per-frame object search.
class SNRVisorWidget : public SLeafWidget
{
public:
 SLATE_BEGIN_ARGS(SNRVisorWidget){} SLATE_ARGUMENT(ANRHUD*,HUD) SLATE_END_ARGS()
 void Construct(const FArguments& A){HUD=A._HUD;SetVisibility(EVisibility::HitTestInvisible);}
 virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1600,900);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override
 {
  auto* H=HUD.Get();auto* C=H?Cast<ANRCharacter>(H->GetOwningPawn()):nullptr;
  if(!C||H->IsMenuOpen())return Layer;
  auto* GS=C->GetWorld()->GetGameState<ANRGameState>();if(!GS)return Layer;
  const FLinearColor Ink(.018,.025,.029,.92),White(.9,.93,.91),Muted(.46,.55,.56),Lime(.8,.93,.35),Cyan(.25,.76,.82),Red(1,.23,.16);
  auto Box=[&](float X,float Y,float W,float Z,FLinearColor Color){FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(W,Z),FSlateLayoutTransform(FVector2D(X,Y))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,Color);};
  auto Line=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width=1){TArray<FVector2D> P{A,B};FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Width);};
  auto Text=[&](float X,float Y,const FString& S,int Size,FLinearColor Color,bool Bold=false){FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(FVector2D(1000,80),FSlateLayoutTransform(FVector2D(X,Y))),S,FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Size),ESlateDrawEffect::None,Color);};
  const float Now=C->GetWorld()->GetTimeSeconds();
  const bool Prep=GS->Phase==ENRPhase::Preparation,Attack=C->Team==GS->AttackTeam;
  const int T=FMath::Max(0,FMath::CeilToInt(GS->PhaseEnd-GS->GetServerWorldTimeSeconds()));
  // Score and phase have different visual weights, leaving the centre sightline unobstructed.
  Box(572,26,456,65,Ink);Box(572,26,4,65,Cyan);Box(1024,26,4,65,Lime);
  Text(592,38,FString(NRFactions::Key(GS->FactionForTeam(0))).ToUpper(),9,Muted,true);Text(689,33,FString::Printf(TEXT("%02d"),GS->EchelonScore),24,White,true);
  Text(762,29,FString::Printf(TEXT("%02d:%02d"),T/60,T%60),26,T<15?Red:White,true);
  Text(864,33,FString::Printf(TEXT("%02d"),GS->SyndicateScore),24,White,true);Text(913,38,FString(NRFactions::Key(GS->FactionForTeam(1))).ToUpper(),9,Muted,true);
  const FString Phase=GS->Phase==ENRPhase::Preparation?TEXT("ПОДГОТОВКА"):GS->Phase==ENRPhase::Combat?TEXT("ОПЕРАЦИЯ АКТИВНА"):GS->Phase==ENRPhase::Resolution?TEXT("РАУНД ЗАВЕРШЁН"):TEXT("ОЖИДАНИЕ");
  Text(730,65,FString::Printf(TEXT("%s  /  %02d"),*Phase,GS->Round),9,Muted);
  Box(24,20,300,157,Ink);
  Text(36,28,TEXT("N / R"),22,White,true);Text(37,60,TEXT("SWITCHYARD   /   СЕКТОР 07"),9,Muted);
  Box(36,103,3,57,Lime);Text(50,100,Prep?(Attack?TEXT("ПОДГОТОВКА / АТАКА"):TEXT("ПОДГОТОВКА / ЗАЩИТА")):C->bCarryingDisk?TEXT("02 / ЭВАКУАЦИЯ"):Attack?TEXT("01 / ИЗВЛЕЧЕНИЕ"):TEXT("01 / ОБОРОНА"),10,Lime,true);
  Text(50,120,Prep?(Attack?TEXT("Подготовьте штурм"):TEXT("Укрепите оборону")):C->bCarryingDisk?TEXT("Доставьте Элару к выходу"):Attack?TEXT("Найдите ядро Элары"):TEXT("Защитите ядро Элары"),15,White,true);
  Text(50,145,Prep?TEXT("TAB  класс   /   B  модули оружия"):TEXT("F   взаимодействие с целью"),10,Muted);
  Text(1320,32,NRFactions::Name(C->GetFaction()),10,Cyan,true);
  Text(1460,55,TEXT("ESC  МЕНЮ"),9,Muted);Text(1410,76,TEXT("B  МОДУЛИ ОРУЖИЯ"),9,Muted);
  // Small compass and cached perception data are painted in the existing single Slate pass.
  Box(616,151,368,36,Ink);
  for(int Bearing=0;Bearing<360;Bearing+=45){const float Delta=FMath::FindDeltaAngleDegrees(C->GetControlRotation().Yaw,float(Bearing));if(FMath::Abs(Delta)<65){float X=800+Delta*2.6f;Line({X,156},{X,161},Muted);const TCHAR* Cardinal=Bearing==0?TEXT("N"):Bearing==90?TEXT("E"):Bearing==180?TEXT("S"):Bearing==270?TEXT("W"):TEXT("·");Text(X-4,166,Cardinal,9,White,true);}}
  Box(799,151,2,8,Lime);
  Box(1270,116,294,116,Ink);Text(1288,128,TEXT("КОНТУР БЕЗОПАСНОСТИ"),9,Muted,true);
  const FLinearColor AlertColor=H->BlackoutSeconds>0?Cyan:H->Threat>.8f?Red:H->Threat>.05f?Lime:Muted;
  const FString Security=H->BlackoutSeconds>0?FString::Printf(TEXT("ОХРАНА ОТКЛЮЧЕНА  %.0f С"),H->BlackoutSeconds):H->Threat>.8f?TEXT("АКТИВНАЯ УГРОЗА РЯДОМ"):H->Threat>.05f?TEXT("ОХРАНА ВЕДЁТ ПОИСК"):TEXT("В ПОЛЕ ЗРЕНИЯ СПОКОЙНО");
  Text(1288,152,Security,10,AlertColor,true);Box(1288,178,256,4,Muted*.3f);Box(1288,178,256*H->Threat,4,AlertColor);
  Text(1288,194,FString::Printf(TEXT("ШУМ: %s   /   ТРЕВОГА: %d"),C->GetVelocity().Size2D()<25?TEXT("НЕТ"):C->IsQuietWalking()?TEXT("ТИХО"):C->IsSprinting()?TEXT("ВЫСОКИЙ"):TEXT("ШАГИ"),H->AlertDrones),8,Muted);
  Text(1288,211,TEXT("СКМ  МЕТКА ДЛЯ КОМАНДЫ"),8,Cyan);
  if(!H->TargetDrone.IsEmpty()&&!C->bDead){Text(710,486,H->TargetDrone,10,White,true);Box(710,508,180,3,Ink);Box(710,508,180*H->TargetHealth,3,Red);}
  if(C->PingUntil>Now){const FVector2D P=H->PingScreen;const auto Color=C->bDangerPing?Red:Cyan;Line(P+FVector2D(-9,0),P+FVector2D(0,-9),Color,2);Line(P+FVector2D(0,-9),P+FVector2D(9,0),Color,2);Line(P+FVector2D(9,0),P+FVector2D(0,9),Color,2);Line(P+FVector2D(0,9),P+FVector2D(-9,0),Color,2);Box(P.X-65,P.Y+14,130,24,Ink);Text(P.X-54,P.Y+19,FString::Printf(TEXT("%s / %.0f М"),C->bDangerPing?TEXT("УГРОЗА"):TEXT("МЕТКА"),H->PingMeters),9,Color,true);}
  if(C->CombatNoticeUntil>Now){Box(580,623,440,29,Ink);Text(598,631,C->CombatNotice,10,Lime,true);}
  // Custom line reticle expands with movement; a single precise dot remains for ADS.
  if(!C->bDead)
  {
   if(C->bAiming){if(C->EquippedWeapon<2&&C->Assembly(C->EquippedWeapon).Optic==ENROptic::Combat2x){Line({775,450},{790,450},Cyan,1);Line({810,450},{825,450},Cyan,1);Line({800,460},{800,475},Cyan,1);}Box(798.5,448.5,3,3,FLinearColor(1,.22,.07));}
   else
   {
    const float Gap=5+FMath::Clamp(C->GetVelocity().Size2D()/65.f,0.f,6.f);
    for(int Sign:{-1,1}){Line({800+Sign*Gap,450},{800+Sign*(Gap+7),450},Ink,3);Line({800,450+Sign*Gap},{800,450+Sign*(Gap+7)},Ink,3);Line({800+Sign*Gap,450},{800+Sign*(Gap+7),450},White,1);Line({800,450+Sign*Gap},{800,450+Sign*(Gap+7)},White,1);}
    Box(799,449,2,2,White);
   }
   const float Hit=FMath::Clamp(1-(Now-C->LastHitTime)*5,0.f,1.f);
   if(Hit>0){FLinearColor HC=C->bLastHitKilled?Red:White;HC.A=Hit;for(int X:{-1,1})for(int Y:{-1,1})Line({800+X*8.,450+Y*8.},{800+X*14.,450+Y*14.},HC,2);}
  }
  // Compact, stable bottom rail. Values are readable against either a bright or dark room.
  Box(36,754,298,112,Ink);Box(36,754,3,112,Lime);
  Text(54,764,TEXT("СОСТОЯНИЕ ОПЕРАТОРА"),9,Muted,true);
  Text(53,780,FString::Printf(TEXT("%03.0f"),C->Attributes->GetHealth()),34,C->Attributes->GetHealth()<30?Red:White,true);
  Text(159,807,TEXT("HP"),10,Muted,true);Text(224,792,FString::Printf(TEXT("%03.0f"),C->Attributes->GetArmorDurability()),20,Cyan,true);
  Text(224,817,TEXT("БРОНЯ"),8,Muted);
  Box(54,839,262,4,FLinearColor(.12,.16,.17));Box(54,839,145*FMath::Clamp(C->Attributes->GetHealth()/100.f,0.f,1.f),4,Lime);Box(224,839,92*FMath::Clamp(C->Attributes->GetArmorDurability()/100.f,0.f,1.f),4,Cyan);
  Text(55,850,FString::Printf(TEXT("%04.0f CR     %02d УБ / %02d СМ"),C->Attributes->GetHardwareTokens(),C->Kills,C->Deaths),8,Muted);
  Box(1298,754,266,112,Ink);Box(1561,754,3,112,Cyan);
  const TCHAR* Names[]={TEXT("MX–9  /  ИНЖЕНЕР"),TEXT("SPECTRE  /  РАЗВЕДЧИК"),TEXT("BR–74  /  ШТУРМОВИК"),TEXT("MX–9  /  МЕДИК")};
  Text(1316,766,C->EquippedWeapon==1?TEXT("P–12 / ПИСТОЛЕТ"):C->EquippedWeapon==2?TEXT("ROUTE / ТАКТИЧЕСКИЙ КЛИНОК"):NRWeaponModules::WeaponName(C->PrimaryKind()),10,Muted,true);
  Text(1314,782,C->EquippedWeapon==2?TEXT("65"):FString::Printf(TEXT("%02d"),C->Ammo),39,C->EquippedWeapon!=2&&C->Ammo<7?Red:White,true);
  Text(1402,805,C->EquippedWeapon==2?TEXT("УРОН"):FString::Printf(TEXT("/ %02d"),C->ReserveAmmo),18,C->ReserveAmmo==0?Red:Muted);Text(1490,809,C->EquippedWeapon==2?TEXT("1.5m"):C->WeaponStats(C->EquippedWeapon).bAutomatic?TEXT("AUTO"):TEXT("SEMI"),9,Cyan,true);
  for(int I=0;I<C->MagazineCapacity();++I)Box(1316+I*(228.f/FMath::Max(1,C->MagazineCapacity())),841,180.f/FMath::Max(1,C->MagazineCapacity()),6,I<C->Ammo?Cyan:FLinearColor(.12,.16,.17));
  Text(1316,853,C->EquippedWeapon==2?TEXT("ЛКМ  УДАР / БЕЗ ПАТРОНОВ"):C->bReloading?TEXT("ПЕРЕЗАРЯДКА…"):C->ReserveAmmo==0?TEXT("РЕЗЕРВ ИСЧЕРПАН"):TEXT("R  ПЕРЕЗАРЯДИТЬ / РЕЗЕРВ"),8,C->bReloading?Lime:Muted);
  const TCHAR* Slots[]={TEXT("1  ОСНОВНОЕ"),TEXT("2  P–12"),TEXT("3  ROUTE")};
  for(int I=0;I<3;++I){float X=1298+I*90;const bool Active=C->EquippedWeapon==I;Box(X,714,86,34,Ink);Box(X,745,86,3,Active?Lime:Muted*.3f);Text(X+8,725,Slots[I],8,Active?Lime:Muted,true);}
  if(C->EquippedWeapon<2){const auto B=C->Assembly(C->EquippedWeapon);Text(1306,693,FString::Printf(TEXT("%s / %s"),NRWeaponModules::Name(0,uint8(B.Optic)),NRWeaponModules::Name(1,uint8(B.Muzzle))),8,Muted);}
  if(C->bReloading){Box(1316,874,228,4,Ink);Box(1316,874,228*C->ReloadFraction(),4,Lime);}
  const float DeltaYaw=FMath::FindDeltaAngleDegrees(C->GetControlRotation().Yaw,H->GuideYaw);
  Box(610,100,380,39,Ink);Text(632,112,H->MissionGuide,10,Cyan,true);
  Text(891,110,FString::Printf(TEXT("%s %.0f м"),DeltaYaw<-12?TEXT("←"):DeltaYaw>12?TEXT("→"):TEXT("↑"),H->GuideMeters),11,White,true);
  if(H->InteractionProgress>0&&H->InteractionProgress<100){Box(643,750,314,26,Ink);Box(643,775,314*H->InteractionProgress/100.f,3,Cyan);Text(658,755,FString::Printf(TEXT("ПЕРЕХВАТ КАНАЛА  /  %d%%"),H->InteractionProgress),10,Cyan,true);}
  if(!C->Subtitle.IsEmpty()&&Now<C->SubtitleUntil){Box(355,674,890,40,Ink);Text(375,685,C->Subtitle,12,White);}
  Box(24,188,300,66,Ink);Text(38,201,Prep?TEXT("РАЗМИНКА / БЕЗОПАСНЫЙ ОГОНЬ"):TEXT("ДОПОЛНИТЕЛЬНЫЕ ЗАДАЧИ"),9,Cyan,true);
  if(Prep)
  {
   Box(24,264,352,116,Ink);Box(24,264,3,116,Attack?Lime:Cyan);
   const bool Medic=C->OperatorRole==ENRRole::Medic;
   Text(40,278,Medic?TEXT("ПОЛЕВАЯ ПОДДЕРЖКА"):Attack?TEXT("ПЛАН ШТУРМА"):TEXT("ПЛАН ОБОРОНЫ"),11,White,true);
   Text(40,303,Medic?TEXT("В бою Z: +40 HP союзнику до 6 м / себе"):Attack?TEXT("Проверьте оружие на мишенях"):TEXT("Z — печать узлов / подготовка проходов"),9,Muted);
   Text(40,326,Medic?TEXT("Восстановление устройства — 18 секунд"):Attack?TEXT("СКМ — отметьте точку входа"):TEXT("X — связи  /  T — логика сети"),9,Muted);
   Text(40,351,TEXT("На старте — полный боезапас"),9,Lime);
   Box(595,198,410,35,Ink);Text(615,208,FString::Printf(TEXT("БАРЬЕР ОТКРОЕТСЯ ЧЕРЕЗ %d С"),T),11,T<=5?Red:Cyan,true);
  }
  Text(38,224,H->ActivityGuide,8,Muted);
  Box(36,691,298,26,Ink);Text(55,698,C->IsQuietWalking()?TEXT("ALT  ТИХИЙ ШАГ АКТИВЕН"):TEXT("ALT  ТИХИЙ ШАГ  /  Q E  НАКЛОН"),9,C->IsQuietWalking()?Lime:Muted);
  if(FMath::Abs(C->GetLeanAmount())>.05f){Box(680,873,240,23,Ink);Text(702,879,C->GetLeanAmount()<0?TEXT("Q  ◀ НАКЛОН ВЛЕВО"):TEXT("НАКЛОН ВПРАВО ▶  E"),9,Cyan,true);}
  Box(36,723,298,26,Ink);Text(55,730,C->bIsCrouched?TEXT("CTRL  ПРИСЕД"):C->IsSprinting()?TEXT("СПРИНТ"):TEXT("SHIFT  СПРИНТ   /   CTRL  ПРИСЕСТЬ"),9,Muted);
  const float Cool=FMath::Max(0.f,C->AbilityReadyTime-GS->GetServerWorldTimeSeconds());
  Box(643,784,314,50,Ink);Box(653,793,31,31,Cool>0?FLinearColor(.16,.2,.2):Lime);Text(662,797,TEXT("Z"),15,Ink,true);
  const TCHAR* Skills[]={TEXT("ПЕЧАТЬ УЗЛА"),TEXT("АНАЛИЗ ПАКЕТОВ"),TEXT("НАПРАВЛЕННЫЙ ЗАРЯД"),TEXT("ПОЛЕВОЕ ЛЕЧЕНИЕ")};
  Text(696,792,Skills[FMath::Min(3,int(C->OperatorRole))],11,White,true);
  Text(696,814,Prep&&(C->OperatorRole==ENRRole::Medic||C->OperatorRole==ENRRole::Scout||(Attack&&C->OperatorRole==ENRRole::Assault))?TEXT("ДОСТУПНО С НАЧАЛА РАУНДА"):Cool>0?FString::Printf(TEXT("ВОССТАНОВЛЕНИЕ  %.1f с"),Cool):TEXT("ГОТОВО К ПРИМЕНЕНИЮ"),8,Cool>0?Muted:Lime);
  Text(608,850,H->CachedContext.ToString(),10,White);
  if(!H->CachedScan.IsEmpty()){Box(1245,330,319,158,Ink);Box(1245,330,3,158,Cyan);Text(1265,350,H->CachedScan.ToString(),12,Cyan);}
  float Damage=FMath::Clamp(1-(Now-C->LastDamageTime)*2,0.f,1.f);
  if(Damage>0){const float A=FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(C->GetControlRotation().Yaw,C->DamageSourceYaw)-90);
   FVector2D D(FMath::Cos(A),FMath::Sin(A)),P(-D.Y,D.X),Center(800,450),Tip=Center+D*83;
   Line(Tip,Center+D*72+P*7,Red,3);Line(Tip,Center+D*72-P*7,Red,3);
   Box(0,0,1600,5,FLinearColor(1,.08,.03,Damage));Box(0,895,1600,5,FLinearColor(1,.08,.03,Damage));}

  if(GS->Phase==ENRPhase::Waiting)
  {Box(495,280,610,92,Ink);Text(525,297,TEXT("ОЖИДАНИЕ КОМАНД"),22,White,true);Text(525,339,FString::Printf(TEXT("Подключено %d / %d. Подготовка начнётся от %d игроков."),GS->ConnectedPlayers,GS->MaximumPlayers,GS->MinimumPlayers),12,Cyan);}
  if(GS->Phase==ENRPhase::Resolution||GS->Phase==ENRPhase::MatchOver)
  {
   FString Reason=GS->Announcement;Reason=Reason.Replace(TEXT("TEAM ELIMINATED"),TEXT("КОМАНДА УСТРАНЕНА")).Replace(TEXT("TIME EXPIRED"),TEXT("ВРЕМЯ ИСТЕКЛО")).Replace(TEXT("ELARA EXTRACTED"),TEXT("ЭЛАРА ЭВАКУИРОВАНА"));
   Box(430,196,740,126,Ink);Box(430,196,740,3,Lime);
   Text(480,211,GS->Phase==ENRPhase::MatchOver?TEXT("МАТЧ ЗАВЕРШЁН"):TEXT("РАУНД ЗАВЕРШЁН"),23,White,true);
   Text(480,254,Reason,14,Lime);Text(480,293,FString::Printf(TEXT("ДРОНЫ: %d    /    БАЛАНС: %.0f CR    /    УБИЙСТВА: %d"),C->DroneKills,C->Attributes->GetHardwareTokens(),C->Kills),10,Muted);
  }
  if(C->bDead){Box(520,365,560,140,Ink);Text(586,385,TEXT("СИГНАЛ ПОТЕРЯН"),30,Red,true);Text(593,448,TEXT("Возвращение в следующем раунде"),14,Muted);}
  return Layer+2;
 }
private:TWeakObjectPtr<ANRHUD> HUD;
};
