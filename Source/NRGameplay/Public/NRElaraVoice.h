#pragma once
#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
struct FNRElaraSpeech {FString Text;FName Voice;float Control=1;uint32 Seed=0;};
// Register an F5-TTS/XTTS backend with IModularFeatures under "NRElaraTTS".
// Models stay local; this interface never runs on the dedicated server.
class NRGAMEPLAY_API INRElaraTTS:public IModularFeature {
public:
 virtual ~INRElaraTTS()=default;
 virtual void Synthesize(FNRElaraSpeech Request,TFunction<void(TArray<float>&&,int32)> PCMCallback)=0;
 virtual void CancelAll()=0;
};
