from pathlib import Path
import wave,json,hashlib
import numpy as np
from scipy.signal import resample_poly,butter,sosfilt
from math import gcd

root=Path(__file__).parent
out=root/'Audio';out.mkdir(exist_ok=True)
checks=[];preview=[]
for role in range(3):
    for cue in range(5):
        source=root/f'raw/V_R{role}_{cue}.wav'
        with wave.open(str(source),'rb') as w:
            assert w.getsampwidth()==2 and w.getnchannels()==1
            rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(np.float64)/32768
        assert np.all(np.isfinite(x)) and 0.5<len(x)/rate<15
        factor=gcd(rate,48000);x=resample_poly(x,48000//factor,rate//factor)
        # A light communications EQ keeps the speaker intelligible and recognisable.
        x=sosfilt(butter(2,[110,6600],btype='bandpass',fs=48000,output='sos'),x)
        active=np.flatnonzero(np.abs(x)>.006)
        assert len(active)>4000,source
        x=x[max(0,active[0]-3840):min(len(x),active[-1]+9600)]
        x-=np.mean(x)
        rms=np.sqrt(np.mean(x*x));gain=min(.105/max(rms,1e-9),.82/max(np.max(np.abs(x)),1e-9));x*=gain
        n=min(240,len(x)//8);x[:n]*=np.linspace(0,1,n);x[-n:]*=np.linspace(1,0,n)
        assert np.max(np.abs(x))<.83
        target=out/source.name
        with wave.open(str(target),'wb') as w:
            w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes((x*32767).astype('<i2').tobytes())
        checks.append({'file':target.name,'voice':['Piper Denis','Piper Dmitri','AI_Roman XTTS'][role],'seconds':round(len(x)/48000,3),'peak':float(np.max(np.abs(x))),'rms':float(np.sqrt(np.mean(x*x))),'sha256':hashlib.file_digest(target.open('rb'),'sha256').hexdigest()})
        if cue==0:preview.extend([x,np.zeros(24000)])
with wave.open(str(root/'VoicePreview.wav'),'wb') as w:
    w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes((np.concatenate(preview)*32767).astype('<i2').tobytes())
(root/'AudioChecks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf-8')
print('NR_VOICE_AUDIO_CHECKS',len(checks),'PASS')
