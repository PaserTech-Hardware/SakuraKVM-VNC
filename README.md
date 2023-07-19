## 🌸 SakuraKVM VNC Server Daemon
This is the daemon process implemention of SakuraKVM VNC Server.  

Here is what the SakuraKVM Project:  

> SakuraKVM is an out-of-the-box & all-in-one keyboard mouse video over ip solution.  
> The KVM over IP, which means keyboard, video and mouse, controlled by a simple network port.  
> Allows you controll your device remotely instead of sitting in front of the device it self.

This is a IoT hardware project, 
we aimed to provide a simple, easy-to-use, and cross-platform solution for KVM over IP.

***This project is still under development, want's to contribute? Feel free to make an pull request!***

[![language](https://img.shields.io/badge/Language-C-orange)](#)
[![compiler](https://img.shields.io/badge/Compiler-GCC-green)](#)
[![license](https://img.shields.io/badge/LICENSE-GPLv2-blue)](#)

## ⚡ Features
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣High performance video streaming, up to 35+ FPS
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣Low latency, less than 100ms in local network
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣⁢⁣Capture HDMI video up to 1080p
- ✅ ⁠ ⁢⁣⁡⁠ ⁢Hardware H.264 encoding, super low CPU usage, no lag
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣Standard VNC RFB protocol, compatible with most VNC client which support H.264 encoding
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣USB gadget support, keyboard and mouse remote control from vnc client
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣BIOS level remote control, even your OS is crashed, you can still remote control your device
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣Works on local network, no account registration is required, even we provide a cloud service, but it's optional
- ✅ ⁠ ⁢⁣⁡⁠ ⁢⁣Fully open source, anythings you want to change, anythings you can change it

## ⭐ Build
 1. libvncserver-sakura, which we provided a forked version of libvncserver, you can find it in [here](https://github.com/PaserTech-Hardware/libvncserver-sakura), is required and have to be installed first. Follow the instructions in the README.md of libvncserver-sakura to install it.
 2. libcedarc, the user-space allwinner Cedar video engine library (This is closed source lib from allwinner, but we don't have other choice), is required and have to be installed. You can find it in [here](https://github.com/ssp97/libcedarc). Install it with the following command:
    ```bash
    $ sudo ./configure --host=arm-linux-gnueabi \
         LDFLAGS="-L/root/libcedarc/library/arm-linux-gnueabi" \
         CC=gcc \
         CCAS=gcc\
         CPP="gcc"\
         CXX=g++\
         CXXCPP="cpp"
    $ sudo make
    $ sudo make install
    $ sudo cp include/* /usr/include/
    $ sudo cp openmax/include/* /usr/include/
    ```
 3. Build this project with the following command:
    ```bash
    $ git clone https://github.com/PaserTech-Hardware/SakuraKVM-VNC
    $ make
    ```
 4. All done, you can find the binary file in `bin/` directory.

## 📋 License
Licensed under GPLv2, which same with linux kernel.  
Not including allwinner's libcedarc, which is closed source but we only use it as a dynamic library, 
so it's not a GPL compatible problem.

## 🎀 Special Thanks
Thanks everyone who helped with this project (in no particular order):
 - [libvncserver](https://github.com/LibVNC/libvncserver), the original libvncserver project, provide a great VNC server library.
 - [ssp97](https://github.com/ssp97), helped a lot with cedar video engine, android ion driver, etc.
 - [aodzip](https://github.com/aodzip/libcedarc), provided a great libcedarc library.
 - [TheSnowfield](https://github.com/TheSnowfield), helped a lot with the VNC protocol, hardware design, testing, etc.
 - ... And many others who helped with this project, but does not want to show their name here.
 - ... And ***YOU*** ❤, use this project, star this project, or even contribute to this project, all of these are the best support for us.
