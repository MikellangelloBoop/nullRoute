import base64, hashlib, json, os, secrets, time, uuid
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlencode, urlparse
import httpx, jwt
from fastapi import FastAPI, HTTPException, Request, Response
from fastapi.responses import RedirectResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field
from sqlalchemy import create_engine, select, update, delete, func
from sqlalchemy.orm import sessionmaker
from sqlalchemy.exc import IntegrityError
from .models import Base, User, Session, LoginProof, Unlock, MatchResult, Submission, OAuthState, AccountLink
from .security import digest, validate_telegram, validate_service_signature

@dataclass
class Settings:
    database_url: str = os.getenv('NR_DATABASE_URL','sqlite:///./nullroute.db')
    public_origin: str = os.getenv('NR_PUBLIC_ORIGIN','http://localhost:8000')
    bot_token: str = os.getenv('NR_TELEGRAM_BOT_TOKEN','')
    server_secret: str = os.getenv('NR_MATCH_SERVER_SECRET','')
    n8n_secret: str = os.getenv('NR_N8N_SECRET','')
    secure_cookies: bool = os.getenv('NR_SECURE_COOKIES','1') != '0'
    issuer: str = os.getenv('NR_OIDC_ISSUER','')
    client_id: str = os.getenv('NR_OIDC_CLIENT_ID','')
    client_secret: str = os.getenv('NR_OIDC_CLIENT_SECRET','')

CATALOG = [
 {'id':'polymer-cyan','name':'Glacial polymer','cost':2,'kind':'cosmetic','description':'Cyan filament. No stat changes.'},
 {'id':'sensor-ghost','name':'Ghost sensor','cost':4,'kind':'sidegrade','description':'Lower visual signature; 1.5 second trigger delay.'},
 {'id':'extruder-brace','name':'Reinforced feed','cost':4,'kind':'sidegrade','description':'Faster reinforcement; longer filament change.'},
 {'id':'elara-fragment-01','name':'Elara / memory 01','cost':0,'kind':'arg','description':'Permanent ARG reward. No expiry.'},
]
PUZZLES={'elara-001':hashlib.sha256(b'NULL ROUTE').hexdigest()}
class TelegramLogin(BaseModel): init_data:str=Field(min_length=1,max_length=8192)
class Claim(BaseModel): pack_id:str=Field(max_length=80)
class PuzzleInput(BaseModel): puzzle_id:str=Field(max_length=80); answer:str=Field(min_length=1,max_length=200)
class MatchPlayer(BaseModel): telegram_id:int=Field(gt=0); disks:int=Field(ge=0,le=3)
class MatchInput(BaseModel): match_id:uuid.UUID; players:list[MatchPlayer]=Field(min_length=1,max_length=10)
class RewardInput(BaseModel): submission_id:uuid.UUID

def create_app(settings: Settings | None=None):
    cfg=settings or Settings()
    engine=create_engine(cfg.database_url,connect_args={'check_same_thread':False,'timeout':15} if cfg.database_url.startswith('sqlite:') else {},pool_pre_ping=True)
    Base.metadata.create_all(engine)
    DB=sessionmaker(engine,expire_on_commit=False)
    app=FastAPI(title='Null Route / Syndicate Terminal',version='0.1.0')
    app.state.db=DB;app.state.settings=cfg

    @contextmanager
    def transaction():
        with DB() as db:
            try:
                yield db
                db.commit()
            except:
                db.rollback();raise

    def identity(request,db,csrf=False):
        token=request.cookies.get('nr_session','')
        session=db.get(Session,digest(token))
        if not token or not session or session.expires<int(time.time()): raise HTTPException(401,'Authentication required')
        if csrf:
            if request.headers.get('origin')!=cfg.public_origin:raise HTTPException(403,'Origin rejected')
            if not secrets.compare_digest(session.csrf_hash,digest(request.headers.get('x-nr-csrf',''))):raise HTTPException(403,'CSRF rejected')
        user=db.get(User,session.user_id)
        if not user:raise HTTPException(401,'Account missing')
        return user,session

    @app.middleware('http')
    async def bounds(request,call_next):
        try: size=int(request.headers.get('content-length','0'))
        except ValueError:return Response(status_code=400)
        if size>16384:return Response(status_code=413)
        response=await call_next(request)
        response.headers['X-Content-Type-Options']='nosniff'
        response.headers['Referrer-Policy']='no-referrer'
        response.headers['Cache-Control']='no-store'
        return response

    @app.get('/health')
    def health():return {'ok':True,'version':'0.1.0','telegram_configured':bool(cfg.bot_token),'oauth_configured':bool(cfg.issuer)}

    @app.post('/auth/telegram')
    def login(body:TelegramLogin,request:Request,response:Response):
        if request.headers.get('origin')!=cfg.public_origin:raise HTTPException(403,'Origin rejected')
        if not cfg.bot_token:raise HTTPException(503,'Configure Telegram bot token')
        try:telegram=validate_telegram(body.init_data,cfg.bot_token)
        except (ValueError,TypeError,KeyError,json.JSONDecodeError):raise HTTPException(401,'Invalid Telegram authentication')
        now=int(time.time());token=secrets.token_urlsafe(32);csrf=secrets.token_urlsafe(32)
        try:
            with transaction() as db:
                db.execute(delete(LoginProof).where(LoginProof.expires<now))
                db.execute(delete(Session).where(Session.expires<now))
                db.add(LoginProof(proof=telegram['proof'],expires=now+330));db.flush()
                user=db.scalar(select(User).where(User.telegram_id==telegram['id']))
                if user is None:user=User(telegram_id=telegram['id'],name=telegram['name']);db.add(user);db.flush()
                db.add(Session(token_hash=digest(token),user_id=user.id,csrf_hash=digest(csrf),expires=now+3600))
        except IntegrityError:raise HTTPException(409,'Authentication proof already consumed; reopen Mini App')
        response.set_cookie('nr_session',token,max_age=3600,httponly=True,secure=cfg.secure_cookies,samesite='lax',path='/')
        return {'csrf':csrf,'expires_in':3600}

    @app.get('/me')
    def me(request:Request):
        with transaction() as db:
            user,s=identity(request,db)
            # Rotate CSRF when restoring a browser session; never persist tokens in localStorage.
            csrf=secrets.token_urlsafe(32);s.csrf_hash=digest(csrf)
            return {'name':user.name,'disks':user.disks,'unlocks':list(db.scalars(select(Unlock.pack_id).where(Unlock.user_id==user.id))),'linked':db.get(AccountLink,user.id) is not None,'csrf':csrf}

    @app.post('/auth/logout')
    def logout(request:Request,response:Response):
        with transaction() as db:user,s=identity(request,db,True);db.delete(s)
        response.delete_cookie('nr_session',path='/');return {'ok':True}

    @app.get('/catalog')
    def catalog():return {'packs':CATALOG,'expires':False}

    @app.post('/packs/claim')
    def claim(body:Claim,request:Request):
        item=next((p for p in CATALOG if p['id']==body.pack_id and p['kind']!='arg'),None)
        if not item:raise HTTPException(404,'Pack unavailable')
        try:
            with transaction() as db:
                user,s=identity(request,db,True)
                if db.get(Unlock,(user.id,item['id'])):return {'owned':True}
                result=db.execute(update(User).where(User.id==user.id,User.disks>=item['cost']).values(disks=User.disks-item['cost']))
                if result.rowcount!=1:raise HTTPException(409,'Not enough extracted disks')
                db.add(Unlock(user_id=user.id,pack_id=item['id'],source='disks'));db.flush()
        except IntegrityError:raise HTTPException(409,'Pack already claimed; refresh inventory')
        return {'owned':True}

    @app.post('/arg/submit')
    def puzzle(body:PuzzleInput,request:Request):
        expected=PUZZLES.get(body.puzzle_id)
        if not expected:raise HTTPException(404,'Unknown fragment')
        with transaction() as db:
            user,s=identity(request,db,True);now=int(time.time())
            count=db.scalar(select(func.count()).select_from(Submission).where(Submission.user_id==user.id,Submission.created_at>now-60))
            if count>=5:raise HTTPException(429,'Try again in one minute')
            solved=secrets.compare_digest(hashlib.sha256(body.answer.strip().upper().encode()).hexdigest(),expected)
            submission=Submission(id=str(uuid.uuid4()),user_id=user.id,puzzle_id=body.puzzle_id,created_at=now,completed=int(solved));db.add(submission)
            if solved and not db.get(Unlock,(user.id,'elara-fragment-01')):db.add(Unlock(user_id=user.id,pack_id='elara-fragment-01',source='arg'))
            return {'submission_id':submission.id,'solved':solved}

    async def service(request,key):
        body=await request.body()
        if len(body)>16384:raise HTTPException(413,'Body too large')
        try:validate_service_signature(body,request.headers.get('x-nr-timestamp',''),request.headers.get('x-nr-signature',''),key)
        except (ValueError,TypeError):raise HTTPException(401,'Service authentication failed')
        return body

    @app.post('/internal/matches')
    async def matches(request:Request):
        raw=await service(request,cfg.server_secret)
        try:body=MatchInput.model_validate_json(raw)
        except ValueError:raise HTTPException(422,'Invalid match result')
        if len({p.telegram_id for p in body.players})!=len(body.players):raise HTTPException(422,'Duplicate participants')
        fingerprint=hashlib.sha256(raw).hexdigest()
        try:
            with transaction() as db:
                previous=db.get(MatchResult,str(body.match_id))
                if previous:
                    if previous.body_hash!=fingerprint:raise HTTPException(409,'Conflicting match replay')
                    return {'duplicate':True}
                db.add(MatchResult(id=str(body.match_id),body_hash=fingerprint));db.flush()
                for player in body.players:
                    db.execute(update(User).where(User.telegram_id==player.telegram_id).values(disks=User.disks+player.disks))
        except IntegrityError:raise HTTPException(409,'Concurrent result; retry identical request')
        return {'accepted':True}

    @app.post('/internal/arg/complete')
    async def arg_complete(request:Request):
        raw=await service(request,cfg.n8n_secret)
        try:body=RewardInput.model_validate_json(raw)
        except ValueError:raise HTTPException(422,'Invalid submission')
        with transaction() as db:
            submission=db.get(Submission,str(body.submission_id))
            if not submission or not submission.completed:raise HTTPException(409,'Submission not verified by game backend')
            # n8n can acknowledge verified work, never invent arbitrary rewards/users.
            return {'acknowledged':True,'reward':'elara-fragment-01'}

    async def discovery():
        if not cfg.issuer or not cfg.client_id or not cfg.issuer.startswith('https://'):raise HTTPException(503,'Release identity provider not configured')
        async with httpx.AsyncClient(timeout=8,follow_redirects=False) as client:
            r=await client.get(cfg.issuer.rstrip('/')+'/.well-known/openid-configuration');r.raise_for_status();data=r.json()
        if data.get('issuer')!=cfg.issuer:raise HTTPException(502,'Issuer mismatch')
        for field in ('authorization_endpoint','token_endpoint','jwks_uri'):
            if not data.get(field,'').startswith('https://'):raise HTTPException(502,'Invalid identity provider')
        return data

    @app.post('/oauth/start')
    async def oauth_start(request:Request):
        metadata=await discovery();state=secrets.token_urlsafe(32);verifier=secrets.token_urlsafe(48);nonce=secrets.token_urlsafe(32)
        with transaction() as db:
            user,s=identity(request,db,True)
            if db.get(AccountLink,user.id):raise HTTPException(409,'Account already linked')
            db.execute(delete(OAuthState).where(OAuthState.user_id==user.id))
            db.add(OAuthState(state_hash=digest(state),user_id=user.id,verifier=verifier,nonce=nonce,expires=int(time.time())+300))
        challenge=base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b'=').decode()
        return {'url':metadata['authorization_endpoint']+'?'+urlencode({'client_id':cfg.client_id,'response_type':'code','scope':'openid','redirect_uri':cfg.public_origin+'/oauth/callback','state':state,'nonce':nonce,'code_challenge':challenge,'code_challenge_method':'S256'})}

    @app.get('/oauth/callback')
    async def oauth_callback(request:Request,state:str,code:str):
        with transaction() as db:
            user,s=identity(request,db);record=db.get(OAuthState,digest(state))
            if not record or record.user_id!=user.id or record.expires<int(time.time()):raise HTTPException(400,'Invalid OAuth state')
            verifier,nonce,user_id=record.verifier,record.nonce,user.id
            # Consume state before external requests. Failed login starts a new flow.
            db.delete(record)
        metadata=await discovery()
        async with httpx.AsyncClient(timeout=8,follow_redirects=False) as client:
            r=await client.post(metadata['token_endpoint'],data={'grant_type':'authorization_code','code':code,'client_id':cfg.client_id,'client_secret':cfg.client_secret,'redirect_uri':cfg.public_origin+'/oauth/callback','code_verifier':verifier});r.raise_for_status()
            token=r.json().get('id_token','');keys=(await client.get(metadata['jwks_uri'])).json()
        try:
            head=jwt.get_unverified_header(token)
            if head.get('alg')!='RS256':raise ValueError('Algorithm rejected')
            key=next(k for k in keys['keys'] if k.get('kid')==head.get('kid'))
            claims=jwt.decode(token,jwt.PyJWK.from_dict(key).key,algorithms=['RS256'],audience=cfg.client_id,issuer=cfg.issuer,options={'require':['exp','iat','sub','iss','aud','nonce']})
            if not secrets.compare_digest(str(claims['nonce']),nonce):raise ValueError('Nonce mismatch')
            if isinstance(claims['aud'],list) and len(claims['aud'])>1 and claims.get('azp')!=cfg.client_id:raise ValueError('Authorized party mismatch')
        except (ValueError,KeyError,StopIteration,jwt.PyJWTError):raise HTTPException(401,'Release identity rejected')
        try:
            with transaction() as db:db.add(AccountLink(user_id=user_id,issuer=cfg.issuer,subject=claims['sub']));db.flush()
        except IntegrityError:raise HTTPException(409,'Identity already linked')
        return RedirectResponse('/',status_code=303)

    static=Path(__file__).parents[1]/'web'
    if static.exists():app.mount('/',StaticFiles(directory=static,html=True),name='web')
    return app

app=create_app()
