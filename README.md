# Communicating between user space and kernel space in Windows

> Note: you may need to run `bcdedit /set testsigning on` in an Administrator Command Prompt and then restart your virtual machine if you haven't done so already.

Open x64 Native Tools Command Prompt for VS 2019 and run

```
cl testdriver.c "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\km\x64\ntoskrnl.lib" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\km" /link /subsystem:native /driver:wdm -entry:DriverEntry

ren testdriver.exe testdriver.sys
```

to compile the driver and rename the file to have the expected file extension. Run `make` in MSYS2 UCRT64 to build the user space code. Then open an Administrator Command Prompt and run

```
sc create testdriver type= kernel binPath= C:\msys64\home\chris\test-deviceioctl\testdriver.sys
sc start testdriver
```

to start the driver (you can use `driverquery` to check that it's running). You can see kernel debug output in DebugView (make sure to enable verbose kernel output). When finished run

```
sc stop testdriver
```

to stop the driver.
