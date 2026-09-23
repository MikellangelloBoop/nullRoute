from pathlib import Path
import numpy as np,wave,json
r=Path(__file__).parent/'Audio';r.mkdir(exist_ok=True);sr=48000;rng=np.random.default_rng(928)
def save(name,x):
 x=np.asarray(x,dtype=float);n=min(480,len(x)//4);x[:n]*=np.linspace(0,1,n);x[-n:]*=np.linspace(1,0,n)
 x=x/max(abs(x).max(),1e-9)*.82
 with wave.open(str(r/(name+'.wav')),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(sr);w.writeframes((x*32767).astype('<i2').tobytes())
 return {'file':name+'.wav','seconds':len(x)/sr,'peak':float(abs(x).max()),'rms':float(np.sqrt(np.mean(x*x)))}
out=[]
t=np.arange(int(.86*sr))/sr
out.append(save('S_DroneCharge',(np.sin(2*np.pi*(310*t+560*t*t))*.55+np.sin(2*np.pi*155*t)*.18)*(0.3+.7*t/.86)*(0.6+.4*np.sin(2*np.pi*12*t)**2)))
t=np.arange(int(.22*sr))/sr;n=rng.normal(0,1,len(t));n=np.convolve(n,np.ones(5)/5,mode='same')
out.append(save('S_DroneShot',(n*.65+np.sin(2*np.pi*(130*t-120*t*t))*.45)*np.exp(-t*24)))
t=np.arange(int(.6*sr))/sr;n=np.convolve(rng.normal(0,1,len(t)),np.ones(15)/15,mode='same')
out.append(save('S_DroneDown',(n+np.sin(2*np.pi*(330*t-220*t*t))*.15)*np.exp(-t*7)))
t=np.arange(int(.07*sr))/sr
out.append(save('S_HitConfirm',(np.sin(2*np.pi*1420*t)+.3*np.sin(2*np.pi*2130*t))*np.exp(-t*60)))
t=np.arange(int(.28*sr))/sr
out.append(save('S_KillConfirm',(np.sin(2*np.pi*740*t)+np.sin(2*np.pi*1110*t)*.45)*np.exp(-t*14)))
t=np.arange(int(.4*sr))/sr
out.append(save('S_TacticalPing',(np.sin(2*np.pi*960*t)*np.exp(-t*16)+np.sin(2*np.pi*1440*t)*np.maximum(0,t-.09)*40*np.exp(-np.maximum(0,t-.09)*30))))
(r/'AudioChecks.json').write_text(json.dumps(out,indent=2),encoding='utf-8');print('Generated',len(out),'original sound cues')
