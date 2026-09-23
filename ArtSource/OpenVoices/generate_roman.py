from pathlib import Path
import os,sys,json,shutil,logging

stage=Path(__file__).parent
project=Path(r'C:\Users\ПК\Desktop\AI_Teacher\AI_Roman')
os.environ['PYTHONDONTWRITEBYTECODE']='1'
os.environ['NUMBA_CACHE_DIR']=str(stage/'numba_cache')
os.environ['HF_HUB_OFFLINE']='1'
os.environ['TRANSFORMERS_OFFLINE']='1'
sys.dont_write_bytecode=True
sys.path.insert(0,str(project/'src'))
logging.basicConfig(level=logging.INFO)
from roma_ai.audio.xtts import XTTSSpeechSynthesizer
import torch

out=stage/('raw_base' if '--base' in sys.argv else 'raw')
out.mkdir(exist_ok=True)
lines=json.loads(Path(r'Z:\nullRoute\ArtSource\VoicePack\voice_source.json').read_text(encoding='utf-8'))['voices'][2]
lines[3]='Патроны получены.';lines[4]='Двигаемся дальше.'
# Reuse only the explicitly requested local voice model/reference; no chat logs or bot credentials.
model_dir=project/'xtts-finetune-webui/base_models/v2.0.2' if '--base' in sys.argv else project/'trained_roma'
tts=XTTSSpeechSynthesizer(model_dir,project/'roma_ref.wav',out,device='cuda' if torch.cuda.is_available() else 'cpu',pronunciation_path=project/'config/pronunciation.json',speed=1.02,temperature=.55)
torch.manual_seed(2409)
for i,line in enumerate(lines):
    if '--repair' in sys.argv and i!=3:continue
    generated=tts._synthesize_sync(line)
    target=out/f'V_R2_{i}.wav'
    shutil.copy2(generated,target)
    print(json.dumps({'file':target.name,'text':line},ensure_ascii=False),flush=True)
print('NR_ROMAN_VOICE_COMPLETE',flush=True)
