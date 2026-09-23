from pathlib import Path
import numpy as np,wave,json
out=Path(__file__).parent/'Audio';out.mkdir(parents=True,exist_ok=True);sr=48000;stats={}
for name,seconds in [('S_Hurt',.38),('S_HurtCritical',.76)]:
 t=np.arange(round(sr*seconds))/sr;rng=np.random.default_rng(3821 if name=='S_Hurt' else 927)
 noise=rng.normal(size=len(t));freq=np.fft.rfftfreq(len(t),1/sr)
 band=(1-1/(1+(freq/450)**4))/(1+(freq/5400)**6)
 grit=np.fft.irfft(np.fft.rfft(noise)*band,n=len(t))
 # Short cloth/body impact with a midrange crack audible on small speakers.
 x=.7*np.sin(2*np.pi*(155*t-75*t*t))*np.exp(-t*18)
 x+=.45*np.sin(2*np.pi*(690*t-290*t*t))*np.exp(-t*34)
 x+=grit*(.62*np.exp(-t*45)+.09*np.exp(-t*8))
 if name=='S_HurtCritical':
  for start in [.17,.39]:
   q=np.maximum(0,t-start);env=(t>=start)*(1-np.exp(-q*180))*np.exp(-q*20)
   x+=env*(.35*np.sin(2*np.pi*110*q)+.2*np.sin(2*np.pi*830*q))
 x=np.tanh(x*1.25);x-=x.mean();x[:96]*=np.linspace(0,1,96);x[-480:]*=np.linspace(1,0,480);x*=.9/max(abs(x).max(),1e-9)
 with wave.open(str(out/(name+'.wav')),'wb')as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(sr);w.writeframes((x*32767).astype('<i2').tobytes())
 stats[name]={'seconds':seconds,'sample_rate':sr,'peak':float(abs(x).max()),'rms':float(np.sqrt(np.mean(x*x)))}
(out/'AudioChecks.json').write_text(json.dumps(stats,indent=2),encoding='utf-8');print(json.dumps(stats,indent=2))
