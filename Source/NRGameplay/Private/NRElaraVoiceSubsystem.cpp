#include "NRElaraVoiceSubsystem.h"
#include "NRElaraVoice.h"
#include "Features/IModularFeatures.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
bool UNRElaraVoiceSubsystem::ShouldCreateSubsystem(UObject* O)const{auto* W=Cast<UWorld>(O);return W&&W->IsGameWorld()&&!IsRunningDedicatedServer();}
void UNRElaraVoiceSubsystem::Speak(const FString& Text,float Control,uint32 Seed){if(GetWorld()->GetNetMode()==NM_DedicatedServer)return;auto& Features=IModularFeatures::Get();if(!Features.IsModularFeatureAvailable(TEXT("NRElaraTTS")))return;
 auto& Backend=Features.GetModularFeature<INRElaraTTS>(TEXT("NRElaraTTS"));FNRElaraSpeech R;R.Text=Text.Left(500);R.Voice=TEXT("Elara");R.Control=FMath::Clamp(Control,0.f,1.f);R.Seed=Seed;const uint32 Request=++Generation;TWeakObjectPtr<UNRElaraVoiceSubsystem> Self=this;
 Backend.Synthesize(R,[Self,Request,Control,Seed](TArray<float>&& Samples,int32 Rate){if(Rate<8000||Rate>96000||Samples.Num()>Rate*30)return;TArray<uint8> PCM;PCM.SetNumUninitialized(Samples.Num()*2);FRandomStream Noise(Seed);auto* Dest=reinterpret_cast<int16*>(PCM.GetData());
 for(int32 I=0;I<Samples.Num();++I){const float Gate=(I/(Rate/24))%11==0?Control:1.f;Dest[I]=int16(FMath::Clamp(Samples[I]*Gate+(1-Control)*Noise.FRandRange(-.025f,.025f),-1.f,1.f)*32767);}
 AsyncTask(ENamedThreads::GameThread,[Self,Request,Rate,PCM=MoveTemp(PCM)](){auto* S=Self.Get();if(!S||S->Generation!=Request)return;auto* Wave=NewObject<USoundWaveProcedural>(S);Wave->SetSampleRate(Rate);Wave->NumChannels=1;Wave->Duration=INDEFINITELY_LOOPING_DURATION;Wave->QueueAudio(PCM.GetData(),PCM.Num());UGameplayStatics::SpawnSound2D(S->GetWorld(),Wave);});});}
void UNRElaraVoiceSubsystem::Deinitialize(){++Generation;Super::Deinitialize();}
