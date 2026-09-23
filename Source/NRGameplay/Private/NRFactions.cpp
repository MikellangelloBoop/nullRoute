#include "NRFactions.h"
#include "Misc/PackageName.h"
#include "Kismet/GameplayStatics.h"

FNRMapFactions UNRFactionSettings::Resolve(const FString& MapPackage)const
{
 FString Short=FPackageName::GetShortName(MapPackage);
 // PIE adds UEDPIE_<instance>_ to the actual map package.
 if(Short.StartsWith(TEXT("UEDPIE_"))){int32 End=Short.Find(TEXT("_"),ESearchCase::CaseSensitive,ESearchDir::FromStart,7);if(End!=INDEX_NONE)Short=Short.Mid(End+1);}
 for(const auto& Pair:Maps)if(Pair.Map==FName(*Short)&&uint8(Pair.Attackers)<NRFactions::Count&&uint8(Pair.Defenders)<NRFactions::Count&&Pair.Attackers!=Pair.Defenders)return Pair;
 return FNRMapFactions();
}
const TCHAR* NRFactions::Key(ENRFaction F){static const TCHAR* Keys[]={TEXT("Chronos"),TEXT("Police"),TEXT("Rebel"),TEXT("Ascended"),TEXT("RustHounds")};return Keys[uint8(F)<Count?uint8(F):0];}
FNRMapFactions NRFactions::WithOptions(FNRMapFactions Base,const FString& Options)
{
 auto Parse=[&](const TCHAR* Option,ENRFaction Fallback){const FString Value=UGameplayStatics::ParseOption(Options,Option);for(uint8 I=0;I<Count;++I)if(Value.Equals(Key(ENRFaction(I)),ESearchCase::IgnoreCase))return ENRFaction(I);return Fallback;};
 const auto Attack=Parse(TEXT("AttackerFaction"),Base.Attackers),Defend=Parse(TEXT("DefenderFaction"),Base.Defenders);
 if(Attack!=Defend){Base.Attackers=Attack;Base.Defenders=Defend;}return Base;
}
const TCHAR* NRFactions::Name(ENRFaction F){static const TCHAR* Names[]={TEXT("CHRONOS SECURITY"),TEXT("CYBERNETIC POLICE"),TEXT("REBEL SYNDICATE"),TEXT("КУЛЬТ СИНГУЛЯРНОСТИ"),TEXT("РЖАВЫЕ ПСЫ")};return Names[uint8(F)<Count?uint8(F):0];}
