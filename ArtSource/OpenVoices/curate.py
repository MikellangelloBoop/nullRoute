from pathlib import Path
import json,re,wave,hashlib,sys
import numpy as np
root=Path(__file__).parent
transcripts=json.loads((root/'TranscriptionCheck-Audio.json').read_text(encoding='utf-8'))
expected=json.loads(Path(r'Z:\nullRoute\ArtSource\VoicePack\voice_source.json').read_text(encoding='utf-8'))['voices'][2]
def norm(s):return re.sub(r'[^а-яa-z0-9]','',s.lower().replace('ё','е'))
final='--final' in sys.argv
edits=json.loads((root/'RomanEdits.json').read_text(encoding='utf-8')) if final else []
targets=[('V_R0_4.wav','Связь установлена.'),('V_R2_3.wav','Патроны получены.'),('V_R2_4.wav','Двигаемся дальше.')] if final else [(f'V_R2_{cue}.wav',expected[cue]) for cue in [0,1,2,4]]
for name,line in targets:
    record=next(r for r in transcripts if r['file']==name)
    desired=[norm(w) for w in line.split()];words=record['words']
    matched=[];cursor=0
    for token in desired:
        index=next((i for i in range(cursor,len(words)) if norm(words[i]['text'])==token),None)
        assert index is not None,(name,token)
        matched.append(index);cursor=index+1
    # Keep complete matched phrases; remove only ASR-confirmed additions between/after them.
    groups=[]
    for i in matched:
        if not groups or i!=groups[-1][-1]+1:groups.append([i])
        else:groups[-1].append(i)
    path=root/'Audio'/name
    with wave.open(str(path),'rb') as w:rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(np.float64)/32768
    parts=[];ranges=[]
    for group in groups:
        start=max(0,words[group[0]]['start']-.015)
        end=words[group[-1]]['end']+(.015 if group[-1]+1<len(words) else .12)
        piece=x[round(start*rate):round(end*rate)].copy();fade=min(round(.005*rate),len(piece)//8)
        piece[:fade]*=np.linspace(0,1,fade);piece[-fade:]*=np.linspace(1,0,fade)
        if parts:parts.append(np.zeros(round(.18*rate)))
        parts.append(piece);ranges.append([start,end])
    y=np.concatenate(parts+[np.zeros(round(.10*rate))])
    with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate);w.writeframes((y*32767).astype('<i2').tobytes())
    edits.append({'file':name,'kept_seconds':ranges,'expected':line})
(root/'RomanEdits.json').write_text(json.dumps(edits,ensure_ascii=False,indent=2),encoding='utf-8')
checks=[];preview=[]
for path in sorted((root/'Audio').glob('V_R*.wav')):
    with wave.open(str(path),'rb') as w:
        assert w.getframerate()==48000 and w.getsampwidth()==2 and w.getnchannels()==1
        x=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(np.float64)/32768
    assert .5<len(x)/48000<12 and .1<abs(x).max()<.83,path
    checks.append({'file':path.name,'seconds':len(x)/48000,'peak':float(abs(x).max()),'rms':float(np.sqrt(np.mean(x*x))),'sha256':hashlib.file_digest(path.open('rb'),'sha256').hexdigest()})
    if path.stem.endswith('_0'):preview.extend([x,np.zeros(24000)])
(root/'AudioChecks.json').write_text(json.dumps(checks,indent=2),encoding='utf-8')
with wave.open(str(root/'VoicePreview.wav'),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes((np.concatenate(preview)*32767).astype('<i2').tobytes())
print('Curated four Roman takes; 15 audio checks PASS')
