from pathlib import Path
import subprocess,tarfile,hashlib,json,concurrent.futures
root=Path(__file__).parent/'models'
root.mkdir(parents=True,exist_ok=True)
def download(voice):
    url=f'https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-ru_RU-{voice}-medium.tar.bz2'
    archive=root/f'{voice}.tar.bz2'
    subprocess.run(['curl.exe','--fail','--location','--retry','1','--connect-timeout','20','--max-time','240','--silent','--show-error','--output',str(archive),url],check=True)
    with tarfile.open(archive) as t:t.extractall(root,filter='data')
    model=root/f'vits-piper-ru_RU-{voice}-medium'/f'ru_RU-{voice}-medium.onnx'
    assert model.is_file() and model.stat().st_size>10_000_000
    # Small upstream configuration/card files do not require the failing model CDN.
    for suffix,target in [('MODEL_CARD',f'{voice}_MODEL_CARD'),(f'ru_RU-{voice}-medium.onnx.json',f'ru_RU-{voice}-medium.onnx.json')]:
        configurl=f'https://huggingface.co/rhasspy/piper-voices/raw/c10ece1aade47bb51c153c893d14e5bf8e5b7117/ru/ru_RU/{voice}/medium/{suffix}'
        subprocess.run(['curl.exe','--fail','--location','--connect-timeout','15','--max-time','40','--silent','--show-error','--output',str(root/target),configurl],check=True)
    assert 'License: CC0' in (root/f'{voice}_MODEL_CARD').read_text()
    record={'name':voice,'distribution_url':url,'archive_sha256':hashlib.file_digest(archive.open('rb'),'sha256').hexdigest(),'model_sha256':hashlib.file_digest(model.open('rb'),'sha256').hexdigest(),'upstream':'rhasspy/piper-voices','dataset_license':'CC0'}
    print(json.dumps(record),flush=True)
    return record
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as e:
    results=list(e.map(download,['denis','dmitri']))
(root/'manifest.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
