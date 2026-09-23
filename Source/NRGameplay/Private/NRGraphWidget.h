#pragma once
#include "Widgets/SLeafWidget.h"
#include "NRGraphSubsystem.h"
#include "NRCharacter.h"
#include "NRNode.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

// A single Slate surface edits the DAG; world cables remain instanced SDF meshes.
class SNRGraphWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SNRGraphWidget){} SLATE_ARGUMENT(TWeakObjectPtr<ANRCharacter>, Character) SLATE_END_ARGS()
    void Construct(const FArguments& Args){Character=Args._Character;}
    virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(920,510);}
    virtual bool SupportsKeyboardFocus()const override{return true;}
    virtual void Tick(const FGeometry& Geometry,double Time,float Dt)override
    {
        Accum+=Dt;if(Accum<.2f)return;Accum=0;Nodes.Reset();
        auto* C=Character.Get();if(!C)return;
        if(auto* Graph=C->GetWorld()->GetSubsystem<UNRGraphSubsystem>())for(auto W:Graph->GetNodes())if(auto* N=W.Get())if(N->Team==C->Team&&N->NodeState!=ENRNodeState::Destroyed)Nodes.Add(N);
        Nodes.Sort([](const auto& A,const auto& B){return A->NodeID.ToString()<B->NodeID.ToString();});
        if(Nodes.Num()>20)Nodes.SetNum(20);Invalidate(EInvalidateWidgetReason::Paint);
    }
    FVector2D Position(int32 I)const{return FVector2D(28+(I%4)*220,48+(I/4)*86);}
    virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override
    {
        auto* C=Character.Get();if(!C)return FReply::Unhandled();const FVector2D P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
        for(int32 I=0;I<Nodes.Num();++I)
        {
            if(!Nodes[I].IsValid())continue;const FVector2D N=Position(I);if(P.X<N.X||P.Y<N.Y||P.X>N.X+186||P.Y>N.Y+60)continue;
            if(E.GetEffectingButton()==EKeys::RightMouseButton)C->ProgramNode(Nodes[I].Get(),ENRNodeOp((uint8(Nodes[I]->Operation)+1)%7));
            else if(E.GetEffectingButton()==EKeys::LeftMouseButton)
            {
                if(Selected.IsValid()&&Selected!=Nodes[I]){C->ConnectNodes(Selected.Get(),Nodes[I].Get());Selected.Reset();}
                else Selected=Nodes[I];
            }
            return FReply::Handled();
        }
        Selected.Reset();return FReply::Handled();
    }
    virtual int32 OnPaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled)const override
    {
        const FSlateBrush* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
        FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),Brush,ESlateDrawEffect::None,FLinearColor(.012,.021,.03,.98));
        auto Text=[&](FVector2D P,FString T,FLinearColor Color,int32 Size){FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(190,25),FSlateLayoutTransform(P)),T,FCoreStyle::GetDefaultFontStyle("Regular",Size),ESlateDrawEffect::None,Color);};
        Text({28,12},TEXT("ЛОКАЛЬНАЯ СЕТЬ / НАПРАВЛЕННЫЙ ГРАФ"),FLinearColor(.5,.7,.8),12);
        if(Nodes.IsEmpty())Text({28,85},TEXT("Создайте узел клавишей Z, затем откройте консоль."),FLinearColor::White,15);
        for(int32 I=0;I<Nodes.Num();++I)if(auto* N=Nodes[I].Get())for(const auto& Link:N->OutputPins)
            for(int32 J=0;J<Nodes.Num();++J)if(Nodes[J].IsValid()&&Nodes[J]->NodeID==Link.Target)
            {
                FVector2D Start=G.LocalToAbsolute(Position(I)+FVector2D(186,30)),End=G.LocalToAbsolute(Position(J)+FVector2D(0,30));
                FSlateDrawElement::MakeDrawSpaceSpline(Out,Layer+1,Start,FVector2D(80,0),End,FVector2D(80,0),2,ESlateDrawEffect::None,FLinearColor(.04,.85,.7));
            }
        const TCHAR* Names[]={TEXT("ДАТЧИК"),TEXT("И"),TEXT("ИЛИ"),TEXT("НЕ"),TEXT("ЗАДЕРЖКА"),TEXT("ТУРЕЛЬ"),TEXT("РЕЛЕ")};
        for(int32 I=0;I<Nodes.Num();++I)if(auto* N=Nodes[I].Get())
        {
            auto P=Position(I);const bool Chosen=Selected.Get()==N;
            FSlateDrawElement::MakeBox(Out,Layer+2,G.ToPaintGeometry(FVector2D(186,60),FSlateLayoutTransform(P)),Brush,ESlateDrawEffect::None,Chosen?FLinearColor(.04,.23,.23):FLinearColor(.045,.072,.09));
            Text(P+FVector2D(12,8),Names[uint8(N->Operation)],Chosen?FLinearColor(.2,1,.8):FLinearColor(.8,.91,1),14);
            Text(P+FVector2D(12,33),FString::Printf(TEXT("%s   OUT %d   %s"),*N->NodeID.ToString().Left(4),N->OutputPins.Num(),N->bOutput?TEXT("1"):TEXT("0")),FLinearColor(.4,.6,.65),10);
        }
        return Layer+4;
    }
private:
    TWeakObjectPtr<ANRCharacter> Character;
    TArray<TWeakObjectPtr<ANodeBase>> Nodes;
    TWeakObjectPtr<ANodeBase> Selected;
    float Accum=1;
};
