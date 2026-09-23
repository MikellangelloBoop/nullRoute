#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NRAlgorithms.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNRGraphTest,"NullRoute.Graph.CycleAndCascade",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNRGraphTest::RunTest(const FString&){FGuid A(1,0,0,0),B(2,0,0,0),C(3,0,0,0),D(4,0,0,0);TMap<FGuid,TArray<FGuid>> G;G.Add(A,{B});G.Add(B,{C});G.Add(C,{A});TestTrue(TEXT("Cycle found without recursion"),NR::Reachable(G,B,A));TestFalse(TEXT("Disconnected target rejected"),NR::Reachable(G,A,D));const auto Victims=NR::Closure(G,A);TestEqual(TEXT("Cascade visits every node once"),Victims.Num(),3);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNRDebrisTest,"NullRoute.Physics.DebrisThreshold",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNRDebrisTest::RunTest(const FString&){float Quiet=0;TestFalse(TEXT("Speed 5 cm/s resets streak"),NR::UpdateDebris(25,3,Quiet));TestFalse(TEXT("Two seconds insufficient"),NR::UpdateDebris(24,2,Quiet));TestFalse(TEXT("Motion resets accumulation"),NR::UpdateDebris(100,.1f,Quiet));TestEqual(TEXT("Streak reset"),Quiet,0.f);TestTrue(TEXT("Three quiet seconds retire particle"),NR::UpdateDebris(0,3,Quiet));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNRSeedTest,"NullRoute.BUP.DeterministicSequence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNRSeedTest::RunTest(const FString&){FRandomStream A(137),B(137),C(138);bool Different=false;for(int32 I=0;I<100;++I){int32 X=A.RandRange(0,100000);TestEqual(TEXT("Same seed reproduces sequence"),X,B.RandRange(0,100000));Different|=X!=C.RandRange(0,100000);}TestTrue(TEXT("Different seeds diverge"),Different);return true;}
#endif
