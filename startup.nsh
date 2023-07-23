@echo -off
mode 80 25

cls
if exist .\EFI\BOOT\BOOTX64.EFI then
 .\EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs0:\EFI\BOOT\BOOTX64.EFI then
 fs0:
 echo Booting system from fs0...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs1:\EFI\BOOT\BOOTX64.EFI then
 fs1:
 echo Booting system from fs1...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs2:\EFI\BOOT\BOOTX64.EFI then
 fs2:
 echo Booting system from fs2...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs3:\EFI\BOOT\BOOTX64.EFI then
 fs3:
 echo Booting system from fs3...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs4:\EFI\BOOT\BOOTX64.EFI then
 fs4:
 echo Booting system from fs4...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs5:\EFI\BOOT\BOOTX64.EFI then
 fs5:
 echo Booting system from fs5...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs6:\EFI\BOOT\BOOTX64.EFI then
 fs6:
 echo Booting system from fs6...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

if exist fs7:\EFI\BOOT\BOOTX64.EFI then
 fs7:
 echo Booting system from fs7...
 EFI\BOOT\BOOTX64.EFI
 goto END
endif

 echo "Unable to find bootloader".
 
:END
