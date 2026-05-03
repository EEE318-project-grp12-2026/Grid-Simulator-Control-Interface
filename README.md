# "power for all" Conttol interface. 


See WIN_X64_BUILD_x.zip for the windows software

See LIN_X64_BUILD_x.tar.xz for the linux software. 

If you must run this on a Mac then the source code is here. Build it yourself, it is supported by Qt. Nobody here own a Mac to facilitate this.
The project uses the community version of Qt creator.

 → note that for Linux, you have to enable serial port access. either run as root or give your user permissions. This is distribution dependant.

The application is also just a binary. there is NO installer. the DB file MUST be placed in the same folder as the executable!


## Obviously the USB MUST be connected for the project to work. 
## USE "MCU_LINK" USB port! the other one requires MCU USB drivers which are NOT used here. 
 if it does not show up in device manager or with `lsusb` then the USB cable is probably broken. 

The baud rate is 115200. this is hard fixed by the application you cannot change it. 

