"""Original deterministic procedural gun reports. No sampled or third-party recordings."""
from pathlib import Path
import random,math,wave,struct
out=Path(__file__).resolve().parents[1]/'ArtSource/Audio';out.mkdir(parents=True,exist_ok=True)
rate=48000
for name,duration,base,seed in [('S_Shotgun',.8,58,1204),('S_LMG',.38,97,6012)]:
 rng=random.Random(seed);slow=0;data=[]
 for i in range(int(rate*duration)):
  t=i/rate;n=rng.uniform(-1,1);slow=.87*slow+.13*n
  attack=1-math.exp(-t*8000)
  crack=n*math.exp(-t*(90 if name=='S_LMG' else 55))
  body=math.sin(2*math.pi*(base*t+12*(1-math.exp(-t*24))/24))*math.exp(-t*14)
  tail=slow*math.exp(-t*5)
  mech=math.sin(2*math.pi*1800*t)*math.exp(-abs(t-.095)*180)*.17
  v=math.tanh((crack*.9+body*.75+tail*.55+mech)*attack)*.8
  data.append(struct.pack('<h',int(v*32767)))
 with wave.open(str(out/(name+'.wav')),'wb') as f:f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate);f.writeframes(b''.join(data))
 print(name,round(duration,2),'seconds')
