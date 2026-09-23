#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NRElaraVoiceSubsystem.generated.h"
UCLASS()class NRGAMEPLAY_API UNRElaraVoiceSubsystem:public UWorldSubsystem {
 GENERATED_BODY()
public:
 virtual bool ShouldCreateSubsystem(UObject* O)const override;
 void Speak(const FString& Text,float Control,uint32 Seed);
 virtual void Deinitialize()override;
private:uint32 Generation=0;
};
