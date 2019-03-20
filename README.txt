# RZ/A2M DRP Linux driver and Demonstration


================================================================================
Installation
================================================================================

This software requires that the RZ/A Linux-4.19 BSP is already installed.
Please follow the instructions here to install that BSP.

	https://elinux.org/RZ-A/Boards/RZA2MEVB


Once the 4.19 BSP has been configured and built for RZ/A2M, please follow
enter the commands below:

	$ cd rza_linux-4.19_bsp
	$ git clone https://github.com/renesas-rz/rza2m_drp


================================================================================
Install the driver into the kernel
================================================================================
Instead of copying all the DRP files under the kernel source tree, we will
make a symbolic link to point to our driver directory in rza2m_drp package.

	$ cd rza_linux-4.19_bsp

Create a symbolic link under the 'drivers' directory in the kernel
	$ cd output/linux-4.19/drivers
	$ ln -s ../../../rza2m_drp/driver/drp
	$ cd ..                                          # back up to the kernel directory
	
Now we will patch the kernel to add our driver (add to menuconfig)
	$ patch -p1 -i ../../rza2m_drp/kernel_patch/*.patch

Finally we will enable the driver in the kernel's menuconfig
	$ cd ../..     # back to root of BSP

	$ ./build.sh kernel menuconfig

	* PRESS the '/' key on the keyboard (open search window)
	* Type "DRP", press Enter
	* Press '1'
	* Press 'y'
	* Press ESC + ESC
	* Press ESC + ESC
	* Press ESC + ESC
	* Press Enter (select "YES" to save new configuration)

Now rebuild the kernel
	$ ./build.sh kernel xipImage

Program the new kernel
	(RESET your board into u-boot)
	$ ./build.sh jlink xipImage 0x20200000


================================================================================
Device Tree with DRP enabled
================================================================================
The following steps are needed to build and program a Device Tree that will
enable the DRP driver.

	$ cd rza_linux-4.19_bsp
	$ ./build.sh env
	$ export ROOTDIR=$(pwd) ; source ./setup_env.sh

	$ cd rza2m_drp
	$ cd devicetree
	$ make

	(Connect your J-Link to your board)
	(RESET your board into u-boot)

	$ make program


	$ cd rza_linux-4.19_bsp
	$ ./build.sh env
	$ export ROOTDIR=$(pwd) ; source ./setup_env.sh

	$ cd rza2m_drp
	$ cd devicetree
	$ make


================================================================================
Build the DRP User API library
================================================================================
The DRP user library will allow you to call the standard Renesas DRP driver
API calls for RTOS systems in your Linux application.

Also, you can select what DRP images you plan to use in your application by
editing the file "config_flash_create.sh"

By default, the config_flash_create.sh is already configured for the DRP
demo that is included in this package.

	$ cd rza_linux-4.19_bsp
	$ ./build.sh env
	$ export ROOTDIR=$(pwd) ; source ./setup_env.sh

	$ cd rza2m_drp
	$ cd drp-lib

Create the binary (drplib-config.bin) with all the DRP images you need. A header
file (drplib-config-api.h) will also be created that your application will use
to reference the data inside the binary.

	$ ./config_flash_create.sh

Program this binary image into QSPI flash (at the last 4MB of QSPI flash area)

	(Connect your J-Link to your board)
	(RESET your board into u-boot)

	$ ./config_flash_program.sh

Build the API library that will be used by the user application.
A libdrp.a library will be created for your user application to statically link
against. This is the recommended method. Also, a libdrp.so runtime library is
created that can be copied to the target file system for your user application to
dynamically  link against at runtime. By default, the demo application's Makefile
will statically link against the libdrp.a library, so copying the libdrp.so file
is not required to run the demo.

	$ make

================================================================================
Build the Demo
================================================================================

	$ cd rza_linux-4.19_bsp
	$ ./build.sh env
	$ export ROOTDIR=$(pwd) ; source ./setup_env.sh

	$ cd rza2m_drp
	$ cd DRP_sample

	$ make
	$ make install

Rebuild and reprogram the file system.

	$ cd ../..
	$ ./build.sh buildroot

	(Connect your J-Link to your board)
	(RESET your board into u-boot)

	$ ./build.sh jlink rootfs_axfs 0x20800000

Boot your board (=> run xha_boot) and log into the kernel as root.
To run the demo, enter:

	$ /root/drp/DRP_sample -mipi
