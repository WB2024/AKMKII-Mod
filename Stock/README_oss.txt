
###HOW TO BUILD KERNEL.#####


  - get Toolchain
         From android git serveru, codesourcery and etc ..
                - gcc-cfp/gcc-cfp-single/aarch64-linux-android-4.9/bin/aarch64-linux-android-
                From Qualcomm developer network (https://developer.qualcomm.com/software/snapdragon-llvm-compiler-android/tools)
                - llvm-arm-toolchain-ship/8.0/

        - make output folder 
                EX) OUTPUT_DIR=out
                $ mkdir out

        - edit Makefile
                edit "CROSS_COMPILE" to right toolchain path(You downloaded).
		export CROSS_COMPILE=/android/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
        - to Build
                $ export ARCH=arm64
                $ make -C $(pwd) O=$(pwd)/out DTC_EXT=$(pwd)/tools/dtc CONFIG_BUILD_ARM64_DT_OVERLAY=y CLANG_TRIPLE=aarch64-linux-gnu-

 -Output files
        - Kernel : arch/arm64/boot/Image
        - module : drivers/*/*.ko




###HOW TO BUILD ANDROID.#####

How to build Module for Platform
- It is only for modules are needed to using Android build system.
- Please check its own install information under its folder for other module.

[Step to build]
1. Get android open source.
    : version info - Android 6.0
    ( Download site : http://source.android.com )

2. Copy module that you want to build - to original android open source
   If same module exist in android open source, you should replace it. (no overwrite)
   
  # It is possible to build all modules at once.
  
3. You should add module name to 'PRODUCT_PACKAGES' in 'build\target\product\core.mk' as following case.
	case 1) e2fsprog : should add 'e2fsck','resize2fs' to PRODUCT_PACKAGES
	case 2) libparanoia : should add 'cdparanoia' to PRODUCT_PACKAGES
	case 3) soxr : should add 'libsoxr' to PRODUCT_PACKAGES


ex.) [build\target\product\core.mk] - add all module name for case 1 ~ 5 at once
    
# e2fsprog
PRODUCT_PACKAGES += \
    e2fsck \
    libext2fs \
    libext2_blkid \
    ibext2_e2p \
    resize2fs
    
# libparanoia
PRODUCT_PACKAGES += \
    cdparanoia \
    
# soxr
PRODUCT_PACKAGES += \
    libsoxr
   
4. excute build command
   $ source build/envsetup.sh
   $ lunch aosp_arm-eng
   $ make -j16
