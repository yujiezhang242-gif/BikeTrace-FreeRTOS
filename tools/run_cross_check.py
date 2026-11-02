"""Clang Cortex-M3 object check, NOT an ARMCC5 build/link or hardware validation.
Uses official GCC port for verification only; firmware uvprojx uses RVDS port.
Minimal libc declaration headers are check-only, never firmware include paths.
"""
from pathlib import Path
import subprocess,xml.etree.ElementTree as E
root=Path(__file__).resolve().parents[1];r=root/'BikeTrace_FreeRTOS';out=root/'tests/build/cross';out.mkdir(parents=True,exist_ok=True)
t=E.parse(r/'MDK-ARM/BikeTrace_FreeRTOS.uvprojx');tg=t.find('./Targets/Target');incs=tg.findtext('./TargetOption/TargetArmAds/Cads/VariousControls/IncludePath').split(';')
includes=['-I'+str(root/'tools/cross'),'-isystem',str(root/'tools/cross/include')]+['-I'+str(r/'MDK-ARM'/p) for p in incs if 'RVDS' not in p]
base=['clang','--target=arm-none-eabi','-mcpu=cortex-m3','-mthumb','-std=c99','-Os','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Wno-unused-function','-Wno-unused-parameter','-Wno-misleading-indentation','-Werror=implicit-function-declaration','-DSTM32F103xB','-DUSE_HAL_DRIVER']+includes
import os
profile=os.environ.get('BIKETRACE_CHECK_PROFILE','lite')
if profile=='heartbeat':base+=['-DAPP_HEARTBEAT_ONLY=1']
if profile=='isolated':base+=['-DAPP_STORAGE_WRITE_ENABLED=1','-DAPP_FLASH_REGION_APPROVED=1','-DAPP_FLASH_REGION_START=0x200000','-DAPP_FLASH_REGION_END=0x210000','-DAPP_MAGNETOMETER_HMC_CONFIRMED=1']
out=out/profile;out.mkdir(exist_ok=True)
logs=[];failed=False
for e in tg.findall('./Groups/Group/Files/File'):
    p=(r/'MDK-ARM'/e.findtext('FilePath').replace('\\','/')).resolve()
    if p.suffix!='.c':continue
    if 'RVDS' in str(p):p=root/'tools/cross/port.c'
    result=subprocess.run(base+['-c',str(p),'-o',str(out/(p.stem+'.o'))],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    logs.append(str(p.relative_to(root))+': '+str(result.returncode)+'\n'+result.stdout)
    if result.returncode:failed=True;print(logs[-1])
(root/'docs/evidence'/('clang_cortex_m3_'+profile+'.txt')).write_text('\n'.join(logs))
print('Clang Cortex-M3 C objects ('+profile+'):', 'FAIL' if failed else 'PASS','(Keil linker and RVDS assembly NOT_RUN)')
raise SystemExit(1 if failed else 0)
