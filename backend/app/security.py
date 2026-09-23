import hashlib, hmac, json, time
from urllib.parse import parse_qsl

def digest(value: str) -> str:
    return hashlib.sha256(value.encode()).hexdigest()

def validate_telegram(init_data: str, bot_token: str, now: int | None = None) -> dict:
    if not bot_token:
        raise ValueError('Telegram login is not configured')
    if not init_data or len(init_data) > 8192:
        raise ValueError('Invalid initData size')
    pairs = parse_qsl(init_data, keep_blank_values=True, strict_parsing=True, max_num_fields=32)
    fields = dict(pairs)
    if len(fields) != len(pairs):
        raise ValueError('Duplicate initData fields')
    signature = fields.pop('hash', '')
    if len(signature) != 64:
        raise ValueError('Invalid hash')
    data = '\n'.join(f'{k}={fields[k]}' for k in sorted(fields))
    key = hmac.new(b'WebAppData', bot_token.encode(), hashlib.sha256).digest()
    expected = hmac.new(key, data.encode(), hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected, signature):
        raise ValueError('Invalid Telegram signature')
    timestamp = int(fields.get('auth_date', '0'))
    now = int(time.time()) if now is None else now
    if not now-300 <= timestamp <= now+30:
        raise ValueError('Expired Telegram authentication')
    user = json.loads(fields.get('user', '{}'))
    if not isinstance(user, dict) or type(user.get('id')) is not int or not 0 < user['id'] < 2**53:
        raise ValueError('Invalid Telegram identity')
    if user.get('is_bot'):
        raise ValueError('Human account required')
    return {'id': user['id'], 'name': str(user.get('first_name', 'Operator'))[:100], 'proof': signature}

def validate_service_signature(body: bytes, timestamp: str, signature: str, key: str, now: int | None = None):
    if len(key) < 32:
        raise ValueError('Service authentication is not configured')
    now = int(time.time()) if now is None else now
    if abs(now - int(timestamp)) > 60:
        raise ValueError('Expired service request')
    expected = hmac.new(key.encode(), timestamp.encode()+b'.'+body, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected, signature):
        raise ValueError('Invalid service signature')
