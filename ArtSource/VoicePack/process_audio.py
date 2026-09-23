from pathlib import Path
import numpy as np,wave,json,zlib
r=Path(__file__).parent;out=r/'Audio';out.mkdir(exist_ok=True);sr=48000;stats={}
def save(name,x):
 x=np.nan_to_num(x);x-=np.mean(x);n=min(240,len(x)//8);x[:n]*=np.linspace(0,1,n);x[-n:]*=np.linspace(1,0,n);x*=.78/max(abs(x).max(),1e-9)
 with wave.open(str(out/(name+'.wav')),'wb')as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(sr);w.writeframes((x*32767).astype('<i2').tobytes())
 stats[name]={'duration':len(x)/sr,'peak':float(abs(x).max())}
for f in sorted((r/'RawVoices').glob('*.wav')):
 with wave.open(str(f),'rb')as w:
  rate=w.getframerate();assert w.getsampwidth()==2;x=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(float)/32768
 x=np.interp(np.arange(int(len(x)*sr/rate))*rate/sr,np.arange(len(x)),x)
 freq=np.fft.rfftfreq(len(x),1/sr);hi=7000 if 'Elara' in f.stem else 5000;low=100 if 'Elara' in f.stem else 240
 filt=(1-1/(1+(freq/low)**4))/(1+(freq/hi)**6);x=np.fft.irfft(np.fft.rfft(x)*filt,n=len(x))
 x=np.tanh(x*1.8)
 if 'Elara' in f.stem:
  delay=round(.047*sr);x[delay:]+=x[:-delay].copy()*.16
 else:
  # Brief radio key transient, no overlapping speech or continuous static.
  rng=np.random.default_rng(zlib.crc32(f.stem.encode()));n=min(800,len(x));x[:n]+=rng.normal(size=n)*np.exp(-np.arange(n)/120)*.03
 save(f.stem,x)
for name,dur,freq,decay in [('S_Pistol',.6,175,25),('S_Equip',.3,540,45),('S_Knife',.45,970,16)]:
 t=np.arange(int(dur*sr))/sr;rng=np.random.default_rng(zlib.crc32(name.encode()));noise=rng.normal(size=len(t));f=np.fft.rfftfreq(len(t),1/sr);noise=np.fft.irfft(np.fft.rfft(noise)/(1+(f/(5000 if name!='S_Pistol' else 11000))**6),n=len(t))
 env=(1-np.exp(-t*1200))*np.exp(-t*decay)
 if name=='S_Knife':env=np.sin(np.minimum(t/.38,1)*np.pi)**2*.5
 x=noise*env+np.sin(2*np.pi*freq*t)*np.exp(-t*(decay+5))*.22
 save(name,x)
t=np.arange(int(.48*sr))/sr
x=(np.sin(2*np.pi*(3600*t-2400*t*t))*.45+np.sin(2*np.pi*5700*t)*.12)*np.exp(-t*17)*(1-np.exp(-t*2500))
x+=np.random.default_rng(712).normal(size=len(t))*np.exp(-t*120)*.12
save('S_Ricochet',x)
assert len(stats)==22,len(stats)
(r/'AudioChecks.json').write_text(json.dumps(stats,indent=2))
print('Prepared',len(stats),'48kHz audio assets')
