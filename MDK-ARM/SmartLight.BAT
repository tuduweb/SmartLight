SET PATH=D:\Keil_v5\ARM\ARMCC\Bin;C:\Program Files (x86)\Common Files\Oracle\Java\javapath;c:\program files\microsoft mpi\bin\;c:\program files\python\python36\scripts\;c:\program files\python\python36\;c:\windows\system32;c:\windows;c:\windows\system32\wbem;c:\windows\system32\windowspowershell\v1.0\;c:\programdata\composersetup\bin;c:\wamp64\bin\php\php5.6.25;c:\program files\putty\;d:\program files\matlab\r2017a\runtime\win64;d:\program files\matlab\r2017a\bin;d:\qt\qt5.9.2\5.9.2\msvc2017_64\bin;c:\windows\system32;c:\windows;c:\windows\system32\wbem;c:\windows\system32\windowspowershell\v1.0\;c:\program files\git\cmd;c:\windows\system32\openssh\;c:\program files\nodejs\;d:\altera\90\quartus\bin;c:\program files\widcomm\bluetooth software\;c:\program files\widcomm\bluetooth software\syswow64;d:\Program Files\Git\cmd;d:\Program Files (x86)\Atmel\sam-ba_2.12\drv\;d:\Program Files (x86)\Atmel\sam-ba_2.12;C:\Users\Administrator\AppData\Roaming\Composer\vendor\bin;C:\Users\Administrator\AppData\Local\Microsoft\WindowsApps;D:\Qt\Qt5.9.2\5.9.2\msvc2017_64\bin;D:\opencv340\opencv\build\x64\vc15\bin;C:\Users\Administrator\AppData\Roaming\npm;D:\Program Files\Microsoft VS Code\bin
SET CPU_TYPE=STM32F407ZGTx
SET CPU_VENDOR=STMicroelectronics
SET UV2_TARGET=SmartLight
SET CPU_CLOCK=0x044AA200
"D:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via "smartlight\startup_stm32f407xx._ia"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "smartlight\main.__i"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "smartlight\stm32f4xx_it.__i"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "smartlight\stm32f4xx_hal_msp.__i"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "smartlight\stm32f4xx_hal_tim.__i"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "smartlight\stm32f4xx_hal_tim_ex.__i"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "smartlight\stm32f4xx_hal_uart.__i"
"D:\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via "SmartLight\SmartLight.lnp"
"D:\Keil_v5\ARM\ARMCC\Bin\fromelf.exe" "SmartLight\SmartLight.axf" --i32combined --output "SmartLight\SmartLight.hex"
