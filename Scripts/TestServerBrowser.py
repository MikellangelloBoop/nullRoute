"""Real UDP discovery and four-client connection tests. Stops only its own processes."""
from pathlib import Path
import argparse,json,socket,uuid,time,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--packaged',action='store_true');p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);a=p.parse_args()
root=a.root;port=17797;logs=root/'Saved';logs.mkdir(exist_ok=True);runs=[];prefix=[]
exe=root/'Releases/Windows/NullRoute/Binaries/Win64/NullRoute.exe'
if not a.packaged:exe=Path(r'Z:\games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe');prefix=[str(root/'NullRoute.uproject')]
def launch(name,url,flags):
    log=logs/(name+'.log')
    if log.exists():log.rename(log.with_name(name+'-'+str(time.time_ns())+'.old.log'))
    run=subprocess.Popen([str(exe),*prefix,url,*flags,'-nullrhi','-nosound','-unattended','-NoSplash',f'-abslog={log}'],creationflags=0x08000000,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    runs.append(run);return run,log
def wait_log(run,log,pattern,seconds=100):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        data=log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
        if pattern in data:return
        if ' FAIL' in data or 'Fatal error:' in data or run.poll() is not None:raise RuntimeError(f'{log.name} failed ({run.poll()}): '+data[-2000:])
        time.sleep(.25)
    raise TimeoutError(f'{log.name}: {pattern}')
def query(timeout=1):
    nonce=uuid.uuid4().hex
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
        s.settimeout(timeout);s.sendto(('NRQ2 '+nonce).encode(),('127.0.0.1',port+1));data,_=s.recvfrom(1024)
    value=json.loads(data);assert value['nonce']==nonce and value['protocol']==2;return value
def wait_query(predicate,seconds=100):
    end=time.monotonic()+seconds;last={}
    while time.monotonic()<end:
        try:
            last=query()
            if predicate(last):return last
        except (TimeoutError,OSError):pass
        time.sleep(.3)
    raise TimeoutError('query condition: '+str(last))
try:
    server,log=launch('ServerLifecycle','/Game/Maps/NR_Arcology?Training=0?MinPlayers=2?MaxPlayers=3?PrepSeconds=15?ServerName=QA', ['-server','-NRServerLifecycle',f'-port={port}','-MULTIHOME=127.0.0.1'])
    wait_log(server,log,f'listening on port {port}');assert wait_query(lambda q:q['players']==0)['phase']=='Ожидание игроков';print('PASS empty server waits',flush=True)
    clients=[]
    for i in range(2):
        c,l=launch('BrowserJoin'+str(i),'/Game/Maps/NR_Arcology?Training=1',['-game','-NRServerJoinTest',f'-NRJoinAddress=127.0.0.1:{port}']);clients.append((c,l));wait_log(c,l,'NR_SERVERJOIN CONNECTED PASS')
        if i==0:assert wait_query(lambda q:q['players']==1)['phase']=='Ожидание игроков';print('PASS one player cannot start round',flush=True)
    assert wait_query(lambda q:q['players']==2 and q['phase']=='Подготовка')['capacity']==3;print('PASS two real clients start preparation',flush=True)
    wait_query(lambda q:q['phase']=='Раунд идёт');print('PASS preparation timer starts combat',flush=True)
    c,l=launch('BrowserLate','/Game/Maps/NR_Arcology?Training=1',['-game','-NRServerJoinTest','-NRExpectLate',f'-NRJoinAddress=127.0.0.1:{port}']);wait_log(c,l,'NR_SERVERJOIN LATE_QUEUED PASS');wait_query(lambda q:q['players']==3);print('PASS late player queues without extra life',flush=True)
    c,l=launch('BrowserFull','/Game/Maps/NR_Arcology?Training=1',['-game','-NRServerJoinTest','-NRExpectFull',f'-NRJoinAddress=127.0.0.1:{port}']);wait_log(c,l,'NR_SERVERJOIN FULL_REJECTED PASS');c.wait(timeout=20);assert c.returncode==0;print('PASS fourth client receives SERVER_FULL',flush=True)
    assert query()['players']==3;print('SERVER BROWSER + DEDICATED MATCH LIFECYCLE PASSED',flush=True)
finally:
    for run in reversed(runs):
        if run.poll() is None:run.terminate()
    for run in runs:
        try:run.wait(timeout=10)
        except subprocess.TimeoutExpired:run.kill()
