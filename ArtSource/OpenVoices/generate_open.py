from pathlib import Path
import json,wave,sys
from piper import PiperVoice,SynthesisConfig

root=Path(__file__).parent
raw=root/'raw';raw.mkdir(exist_ok=True)
lines=json.loads(Path(r'Z:\nullRoute\ArtSource\VoicePack\voice_source.json').read_text(encoding='utf-8'))['voices']
lines[0][1]='Перезарядка.';lines[0][4]='Связи установлены.'
for role,name in [(0,'denis'),(1,'dmitri')]:
    voice=PiperVoice.load(str(root/f'models/vits-piper-ru_RU-{name}-medium/ru_RU-{name}-medium.onnx'),config_path=str(root/f'models/ru_RU-{name}-medium.onnx.json'),espeak_data_dir='Z:/nullRoute/ArtSource/OpenVoices/espeak-ng-data')
    for cue,text in enumerate(lines[role]):
        if '--repair' in sys.argv and (role!=0 or cue not in [1,4]):continue
        with wave.open(str(raw/f'V_R{role}_{cue}.wav'),'wb') as out:
            voice.synthesize_wav(text,out,syn_config=SynthesisConfig(length_scale=.94 if role==1 else 1.0,noise_scale=.55,noise_w_scale=.65))
        print(f'V_R{role}_{cue}: {text}',flush=True)
print('NR_OPEN_VOICE_COMPLETE')
