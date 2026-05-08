# #powerforall GUI 

## A user interface for EE318 project. 
this is NEEDED to use demo

## HOWTO use the app

you MUST be on a windows 11 or linux PC using the AMD64 architecture. if you have an Intel or AMD sticker on your PC this should run

you MUST use the DEBUG port on the NXP the usb peripheral port needed drivers and that just introdices yet more headache ontop the that already induced by that IDE. 


## Detailed Guides

### (Windows)
1. download the WIN_REL_1_0_0.zip file from here: https://drive.google.com/file/d/1dAqEIT6xJCiBl2aGFIWUwGef23DyerY0/view?usp=sharing
2. extract the arhive
3. run the EXE it contains.

### (Linux)
1. Download the REL_LINX64_1_0_0.tar.xz file from releases.
2. Extract the archive
3. in a terminal, same directory as the extracted contents of the archive, run `sh launch.sh`
   3.1.    if the launch fails, you likely need to set permissions for the serial port. (OS dependant) 
           run `sudo adduser $USER dialout` if you are using ubuntu or similar. to fix this
   3.2.    (LAZY METHOD): run `sudo ./GridSimUserControl` to bypass permissions.


### (unsupported PCs)
Qt is a cross platform toolkit. only thing is that you need said platform to build the program. 
You can probably build a release yourself. (Make your own build file)

### (unsupported devices):
devices like smart watches, phones, tablets etc... are NOT supported and cannot be used to run this software. 


## BUild Guide:
download the repo and run the .bat or .sh file. the release and needed libs are in the resulting folder. 
You NEED to have Qt 6.9.0 build tools present. (NO SCRIPT FOR MAC!)

