$ErrorActionPreference='Stop'
$Source=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'voice_source.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$Raw=Join-Path $PSScriptRoot 'RawVoices'
New-Item -ItemType Directory -Path $Raw -Force | Out-Null
for($Role=0;$Role -lt 4;$Role++) {
 $Voice=New-Object -ComObject SAPI.SpVoice
 $Token=New-Object -ComObject SAPI.SpObjectToken
 $TokenName=if($Role -eq 1 -or $Role -eq 3){'MSTTS_V110_ruRU_IrinaM'}else{'MSTTS_V110_ruRU_PavelM'}
 $Token.SetId('HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech_OneCore\Voices\Tokens\'+$TokenName)
 $Voice.Voice=$Token
 $Voice.Rate=if($Role -eq 1){1}elseif($Role -eq 2){-1}else{0}
 $Lines=if($Role -eq 3){$Source.elara}else{$Source.voices[$Role]}
 for($Cue=0;$Cue -lt $Lines.Count;$Cue++) {
  $Name=if($Role -eq 3){"V_Elara_$Cue"}else{"V_R$($Role)_$Cue"}
  $Stream=New-Object -ComObject SAPI.SpFileStream
  $Stream.Format.Type=22
  $Stream.Open((Join-Path $Raw ($Name+'.wav')),3,$false)
  $Voice.AudioOutputStream=$Stream
  $Pitch=if($Role -eq 2){-4}elseif($Role -eq 0){-1}elseif($Role -eq 3){2}else{0}
  [void]$Voice.Speak(('<pitch middle="'+$Pitch+'">'+$Lines[$Cue]+'</pitch>'),8)
  $Stream.Close()
  Write-Output $Name
 }
}
