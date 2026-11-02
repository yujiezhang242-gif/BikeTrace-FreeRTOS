from pathlib import Path
import xml.etree.ElementTree as E
root=Path(__file__).resolve().parents[1];r=root/'BikeTrace_FreeRTOS'
t=E.parse(r/'MDK-ARM/BikeTrace_FreeRTOS.uvprojx');report=[]
for tg in t.findall('./Targets/Target'):
    name=tg.findtext('TargetName');paths=[f.findtext('FilePath').replace('\\','/') for f in tg.findall('./Groups/Group/Files/File')]
    assert len(paths)==len(set(paths))
    for p in paths:assert (r/'MDK-ARM'/p).is_file(),p
    assert len([p for p in paths if p.endswith('/port.c')])==1
    assert len([p for p in paths if 'heap_' in p])==1
    assert all(not any(p.startswith('../'+name+'/') for name in ['APP','BSP','Mid','System']) for p in paths)
    assert tg.findtext('./TargetOption/TargetCommonOption/Device')=='STM32F103C8'
    for source in r.glob('Application/**/*.c'):
        assert '../'+str(source.relative_to(r)) in paths,source
    report.append(name+': PASS, '+str(len(paths))+' sources, C8 unchanged, one RVDS port/heap, all new C units included, legacy business excluded')
it=(r/'Core/Src/stm32f1xx_it.c').read_text()
for n in ['SysTick_Handler','PendSV_Handler','SVC_Handler']:assert 'void '+n+'(' not in it
uart=(r/'Application/System/app_uart.c').read_text();isr=uart[uart.index('void HAL_UART_RxCpltCallback'):uart.index('void HAL_UART_ErrorCallback')]
for banned in ['printf','HAL_Delay','Erase','malloc','parse']:assert banned not in isr
config=(r/'Application/Inc/app_config.h').read_text();assert '#define APP_STORAGE_WRITE_ENABLED 0' in config and '#define APP_NETWORK_ENABLED 0' in config
report.append('IRQ source ownership / UART bounded ingress / default write+network guards: PASS')
text='\n'.join(report)+'\n';print(text,end='');(root/'docs/evidence/project_audit.txt').write_text(text)
