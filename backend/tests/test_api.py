import hashlib,hmac,json,time,uuid
from urllib.parse import urlencode
import pytest
from fastapi.testclient import TestClient
from app.main import Settings,create_app
from app.security import validate_telegram

TOKEN='123456:test-bot-secret'
SERVER='s'*40
def data(uid=42,age=0,extra=None):
    fields={'auth_date':str(int(time.time())+age),'user':json.dumps({'id':uid,'first_name':'Operator'}),'query_id':str(uuid.uuid4())}
    if extra:fields.update(extra)
    key=hmac.new(b'WebAppData',TOKEN.encode(),hashlib.sha256).digest()
    fields['hash']=hmac.new(key,'\n'.join(f'{k}={v}' for k,v in sorted(fields.items())).encode(),hashlib.sha256).hexdigest()
    return urlencode(fields)
@pytest.fixture
def client(tmp_path):
    cfg=Settings(database_url=f'sqlite:///{tmp_path}/test.db',bot_token=TOKEN,public_origin='http://testserver',server_secret=SERVER,n8n_secret='n'*40,secure_cookies=False)
    with TestClient(create_app(cfg)) as c:yield c
def login(c,uid=42):
    r=c.post('/auth/telegram',json={'init_data':data(uid)},headers={'Origin':'http://testserver'});assert r.status_code==200
    return {'Origin':'http://testserver','X-NR-CSRF':r.json()['csrf']}
def signed(c,payload):
    raw=json.dumps(payload,separators=(',',':')).encode();ts=str(int(time.time()));signature=hmac.new(SERVER.encode(),ts.encode()+b'.'+raw,hashlib.sha256).hexdigest()
    return c.post('/internal/matches',content=raw,headers={'content-type':'application/json','x-nr-timestamp':ts,'x-nr-signature':signature})
def test_telegram_identity():assert validate_telegram(data(),TOKEN)['id']==42
@pytest.mark.parametrize('age',[-301,40])
def test_stale_telegram(age):
    with pytest.raises(ValueError):validate_telegram(data(age=age),TOKEN)
def test_duplicate_fields():
    with pytest.raises(ValueError):validate_telegram(data()+'&auth_date=0',TOKEN)
def test_tamper():
    with pytest.raises(ValueError):validate_telegram(data().replace('Operator','Attacker'),TOKEN)
def test_login_replay(client):
    raw=data();assert client.post('/auth/telegram',json={'init_data':raw},headers={'Origin':'http://testserver'}).status_code==200
    assert client.post('/auth/telegram',json={'init_data':raw},headers={'Origin':'http://testserver'}).status_code==409
def test_auth_required(client):assert client.get('/me').status_code==401
def test_csrf_and_origin(client):
    h=login(client);assert client.post('/packs/claim',json={'pack_id':'polymer-cyan'}).status_code==403
    h['Origin']='https://attacker.example';assert client.post('/packs/claim',json={'pack_id':'polymer-cyan'},headers=h).status_code==403
def test_match_idempotency_and_claim(client):
    h=login(client);payload={'match_id':str(uuid.uuid4()),'players':[{'telegram_id':42,'disks':3}]}
    assert signed(client,payload).status_code==200;assert signed(client,payload).json()['duplicate']
    assert client.post('/packs/claim',json={'pack_id':'polymer-cyan'},headers=h).status_code==200
    assert client.post('/packs/claim',json={'pack_id':'polymer-cyan'},headers=h).status_code==200
    me=client.get('/me').json();assert me['disks']==1;assert me['unlocks']==['polymer-cyan']
    payload['players'][0]['disks']=2;assert signed(client,payload).status_code==409
def test_forged_result(client):assert client.post('/internal/matches',json={'players':[]}).status_code==401
def test_no_negative_balance(client):
    h=login(client);assert client.post('/packs/claim',json={'pack_id':'polymer-cyan'},headers=h).status_code==409;assert client.get('/me').json()['disks']==0
def test_arg_permanent_reward(client):
    h=login(client)
    for _ in range(2):assert client.post('/arg/submit',json={'puzzle_id':'elara-001','answer':'null route'},headers=h).json()['solved']
    assert client.get('/me').json()['unlocks']==['elara-fragment-01']
def test_arg_rate_limit(client):
    h=login(client)
    for _ in range(5):assert client.post('/arg/submit',json={'puzzle_id':'elara-001','answer':'wrong'},headers=h).status_code==200
    assert client.post('/arg/submit',json={'puzzle_id':'elara-001','answer':'wrong'},headers=h).status_code==429
def test_unauthorized_arg_reward(client):assert client.post('/internal/arg/complete',json={'submission_id':str(uuid.uuid4())}).status_code==401
def test_oauth_unconfigured(client):assert client.post('/oauth/start',json={}).status_code==503
def test_logout(client):
    h=login(client);assert client.post('/auth/logout',headers=h).status_code==200;assert client.get('/me').status_code==401
