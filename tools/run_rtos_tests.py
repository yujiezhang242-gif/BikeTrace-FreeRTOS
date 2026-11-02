from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1];k=root/'BikeTrace_FreeRTOS/Middlewares/FreeRTOS';a=root/'BikeTrace_FreeRTOS/Application/Inc';out=root/'tests/build/test_rtos'
args=['clang','-std=c99','-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-Wno-pointer-to-int-cast','-I'+str(root/'tests/freertos_host'),'-I'+str(k/'include'),'-I'+str(a)]+[str(k/p) for p in ['tasks.c','queue.c','list.c','stream_buffer.c','portable/MemMang/heap_4.c']]+[str(root/'tests/test_rtos_resources.c'),'-o',str(out)]
logs=[]
for cmd in [args,[str(out)]]:
 p=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);print(p.stdout,end='');logs.append(p.stdout)
 (root/'docs/evidence/rtos_resource_tests.txt').write_text('\n'.join(logs))
 if p.returncode:raise SystemExit(p.returncode)
