from pathlib import Path
import difflib,json,tarfile
root=Path(__file__).resolve().parents[1];work=root/'BikeTrace_FreeRTOS'
manifest=json.loads((root/'docs/baseline_manifest.json').read_text());changes=[];patch=[]
with tarfile.open(root/'baseline/BikeTrace_original.tar.gz') as t:
    for p in sorted(work.rglob('*')):
        if not p.is_file() or p.suffix.lower() not in ['.c','.h','.s','.ioc','.uvprojx']:continue
        name=str(p.relative_to(work));new=p.read_bytes()
        if name in manifest:
            old=t.extractfile('BikeTrace_original/'+name).read()
            if old==new:continue
            changes.append(('MODIFIED',name))
        else:
            if name.startswith('Middlewares/'):continue
            old=b'';changes.append(('ADDED',name))
        patch.extend(difflib.unified_diff(old.decode('utf-8',errors='replace').splitlines(keepends=True),new.decode('utf-8',errors='replace').splitlines(keepends=True),fromfile='baseline/'+name,tofile='BikeTrace_FreeRTOS/'+name))
(root/'docs/changes.patch').write_text(''.join(patch))
(root/'docs/changed_files.md').write_text('# Firmware changes\n\n'+''.join('- '+status+': `'+name+'`\n' for status,name in changes)+'\nVendor FreeRTOS import is listed separately in Middlewares/FreeRTOS/PROVENANCE.md.\n')
print('Change report:',len(changes),'authored firmware files; vendor import separate')
