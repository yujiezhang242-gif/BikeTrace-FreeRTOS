from pathlib import Path
import copy
import xml.etree.ElementTree as E
root_dir=Path(__file__).resolve().parents[1]/'BikeTrace_FreeRTOS'
t=E.parse(root_dir/'MDK-ARM/BikeTrace.uvprojx');root=t.getroot();target=root.find('./Targets/Target');target.find('TargetName').text='BikeTrace_C8_LITE';groups=target.find('Groups')
for g in list(groups):
    files=g.find('Files')
    if files is None:
        groups.remove(g)
        continue
    for f in list(files):
        p=f.findtext('FilePath','').replace('\\','/').lower()
        if any(x in p for x in ['/app/','/bsp/','/mid/','/system/']) or p.endswith('/iic.c'):
            files.remove(f)
    if not len(files):groups.remove(g)
def group(name,paths):
    g=E.SubElement(groups,'Group');E.SubElement(g,'GroupName').text=name;files=E.SubElement(g,'Files')
    for p in paths:
        f=E.SubElement(files,'File');E.SubElement(f,'FileName').text=Path(p).name;E.SubElement(f,'FileType').text='1';E.SubElement(f,'FilePath').text=p
app=sorted(root_dir.glob('Application/**/*.c'))
group('Application_RTOS',['../'+str(p.relative_to(root_dir)) for p in app])
group('FreeRTOS_10_6_2',['../Middlewares/FreeRTOS/'+p for p in ['tasks.c','queue.c','list.c','stream_buffer.c','portable/MemMang/heap_4.c','portable/RVDS/ARM_CM3/port.c']])
c=target.find('./TargetOption/TargetArmAds/Cads/VariousControls')
c.find('IncludePath').text=';'.join(['../Core/Inc','../Drivers/STM32F1xx_HAL_Driver/Inc','../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy','../Drivers/CMSIS/Device/ST/STM32F1xx/Include','../Drivers/CMSIS/Include','../Application/Inc','../Application/Services','../Application/System','../Middlewares/FreeRTOS/include','../Middlewares/FreeRTOS/portable/RVDS/ARM_CM3'])
target.find('./TargetOption/TargetArmAds/Cads/uC99').text='1'
target.find('./TargetOption/TargetArmAds/Cads/Optim').text='2'
beat=copy.deepcopy(target);beat.find('TargetName').text='BikeTrace_P1_Heartbeat';beat.find('./TargetOption/TargetArmAds/Cads/VariousControls/Define').text='USE_HAL_DRIVER,STM32F103xB,APP_HEARTBEAT_ONLY=1';root.find('Targets').append(beat)
for tg in root.findall('./Targets/Target'):
    common=tg.find('./TargetOption/TargetCommonOption');common.find('OutputDirectory').text='.\\'+tg.findtext('TargetName')+'\\';common.find('OutputName').text=tg.findtext('TargetName');common.find('ListingPath').text='.\\'+tg.findtext('TargetName')+'\\'
E.indent(t,space='  ');t.write(root_dir/'MDK-ARM/BikeTrace_FreeRTOS.uvprojx',encoding='UTF-8',xml_declaration=True)
print('Generated two C8 targets;',len(app),'application C files')
