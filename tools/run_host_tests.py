from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
a=root/'BikeTrace_FreeRTOS/Application'
out=root/'tests/build';out.mkdir(exist_ok=True)
cmd=['clang','-std=c99','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-I'+str(a/'Inc'),'-I'+str(a/'Services')]+[str(p) for p in sorted((a/'Services').glob('*.c'))]+[str(root/'tests/test_services.c'),'-lm','-o',str(out/'test_services')]
log=[]
for args in [cmd,[str(out/'test_services')]]:
    p=subprocess.run(args,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    print(p.stdout,end='');log.append(p.stdout)
    (root/'docs/evidence/host_tests.txt').write_text('\n'.join(log))
    if p.returncode:raise SystemExit(p.returncode)
