"""Original layered synthesis at 48 kHz; deterministic seeds; no external recordings."""
from pathlib import Path
import numpy as np
import wave, zlib, json
ROOT=Path(__file__).parent/'Audio'
ROOT.mkdir(exist_ok=True)
RATE=48000
stats={}
def timeline(seconds):return np.arange(round(seconds*RATE))/RATE
def noise(name,t,low=0,high=14000):
    rng=np.random.default_rng(zlib.crc32(name.encode()))
    x=rng.normal(size=len(t));freq=np.fft.rfftfreq(len(x),1/RATE)
    filt=1/(1+(freq/high)**6)
    if low:filt*=1-1/(1+(freq/low)**6)
    y=np.fft.irfft(np.fft.rfft(x)*filt,n=len(x))
    return y/(np.std(y)+1e-8)
def env(t,start,decay):
    q=np.maximum(t-start,0);return (t>=start)*(1-np.exp(-q*1800))*np.exp(-q*decay)
def save(name,x,peak=.78):
    x=np.nan_to_num(x);x-=np.mean(x)
    # A short fade eliminates discontinuities at the file boundaries.
    n=min(240,len(x)//10);x[:n]*=np.linspace(0,1,n);x[-n:]*=np.linspace(1,0,n)
    x=np.tanh(x*.8);x*=peak/max(np.max(np.abs(x)),1e-8)
    with wave.open(str(ROOT/(name+'.wav')),'wb')as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes((x*32767).astype('<i2').tobytes())
    stats[name]={'seconds':len(x)/RATE,'peak_dbfs':float(20*np.log10(np.max(np.abs(x)))),'rms_dbfs':float(20*np.log10(np.sqrt(np.mean(x*x))))}
for name,base,decay,crack in [('S_SMG',142,25,.9),('S_Carbine',92,17,1.15),('S_Marksman',69,15,.5)]:
    t=timeline(.85);n=noise(name,t,400,13000);body=noise(name+'body',t,70,1100)
    x=n*env(t,.004,155)*crack+body*env(t,.007,decay)*.24
    x+=np.sin(2*np.pi*(base*t+2*(1-np.exp(-t*60))))*env(t,.003,decay+5)*.7
    x+=noise(name+'metal',t,1800,6500)*env(t,.038,100)*.19
    for start,gain in [(.063,.15),(.127,.075),(.213,.035)]:x+=n*env(t,start,24)*gain
    save(name,x)
t=timeline(1.6);n=noise('reload',t,450,8000);x=np.zeros_like(t)
for i,(start,strength) in enumerate([(0,.55),(.22,.32),(.57,.45),(1.12,.72),(1.37,.48)]):
    x+=n*env(t,start,70)*strength
    x+=np.sin(2*np.pi*(420+i*170)*t)*env(t,start,95)*strength*.32
    x+=np.sin(2*np.pi*98*t)*env(t,start,45)*strength*.18
save('S_Reload',x,.65)
for surface in ['Concrete','Metal']:
    for i in range(1,5):
        t=timeline(.34);n=noise(surface+str(i),t,160,5200);base=74+i*5
        x=n*env(t,.008,45)*.3+np.sin(2*np.pi*base*t)*env(t,.012,33)*.7
        x+=noise('scuff'+surface+str(i),t,1000,8000)*env(t,.064,27)*.07
        if surface=='Metal':
            for hz,g in [(237,.12),(479,.10),(813,.04)]:x+=np.sin(2*np.pi*(hz+i*8)*t)*env(t,.009,17)*g
        save('S_Step'+surface+str(i),x,.55)
t=timeline(.22);save('S_DryFire',noise('dry',t,700,10000)*env(t,.005,110)+np.sin(2*np.pi*1900*t)*env(t,.008,160)*.2,.48)
t=timeline(.35);save('S_Impact',noise('impact',t,1700,13000)*env(t,.004,92)+np.sin(2*np.pi*3100*t)*env(t,.012,45)*.14,.68)
t=timeline(.45);save('S_Land',noise('land',t,80,2300)*env(t,.008,19)*.5+np.sin(2*np.pi*60*t)*env(t,.01,25)*.8,.68)
t=timeline(1.8);save('S_Breach',noise('breach',t,30,10000)*env(t,.006,11)+np.sin(2*np.pi*(48*t-7*t*t))*env(t,.007,5)*.8,.85)
for name,freq in [('S_UI',1080),('S_Confirm',810)]:
    t=timeline(.2);save(name,np.sin(2*np.pi*freq*t)*env(t,.008,35)+np.sin(2*np.pi*freq*1.5*t)*env(t,.055,45)*.35,.36)
# Integer frequencies and periodic filtered noise make the ambient loop seamless.
t=timeline(8);x=.3*np.sin(2*np.pi*50*t)+.12*np.sin(2*np.pi*100*t)+.04*np.sin(2*np.pi*151*t)+noise('room',t,80,950)*.11
with wave.open(str(ROOT/'S_RoomTone.wav'),'wb')as w:
    w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes((x*.35*32767).astype('<i2').tobytes())
(ROOT.parent/'AudioLevels.json').write_text(json.dumps(stats,indent=2))
print('Generated',len(stats)+1,'original sound files')
