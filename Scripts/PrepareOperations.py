from pathlib import Path
import runpy,unreal
root=Path(unreal.Paths.project_dir()).resolve()
runpy.run_path(str(root/'Scripts/ImportOperationsAudio.py'),run_name='__main__')
runpy.run_path(str(root/'Scripts/OperationsArena.py'),run_name='__main__')
