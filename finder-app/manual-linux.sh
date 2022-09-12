#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
   wget https://github.com/torvalds/linux/commit/e33a814e772cdc36436c8c188d8c42d019fda639.diff
    git apply e33a814e772cdc36436c8c188d8c42d019fda639.diff 
    # TODO: Add your kernel build steps here
  
  
    
     #deep clean the kernel 
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    #configure the default config
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    # creates the image
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image 
    
    #build a kernel image
    make -j12 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    
    #build the kernel modules
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    
    #build the device tree
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    
    


fi

echo "Adding the Image in outdir"

#Access the image
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR} 

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
	# Adding an image 
	mkdir -p ${OUTDIR}/rootfs
	cd ${OUTDIR}/rootfs
	# making a series of directories
	mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
	mkdir -p usr/bin/lib usr/sbin
	mkdir -p var/log
	

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    
    
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    
else
    cd busybox
fi

# TODO: Make and install busybox
   
    
    make -j12 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
    
    
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
	export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
	cp $SYSROOT/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
	
	cp $SYSROOT/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
	
	cp $SYSROOT/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
	
	cp $SYSROOT/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64
	
# TODO: Make device nodes
	# make null  node
	sudo mknod -m 666 dev/null c 1 3
	# make console node
	sudo mknod -m 600 dev/console c 5 1 
# TODO: Clean and build the writer utility
	cd ${FINDER_APP_DIR}
	
	make clean
	make CROSS_COMPILE=${CROSS_COMPILE} all
	

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
	cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home

	cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home

	cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home

	cp ${FINDER_APP_DIR}/conf/ -r ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
	cd ${OUTDIR}/rootfs
	sudo chown -R root:root *
	
# TODO: Create initramfs.cpio.gz
	cd ${OUTDIR}/rootfs
	find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
	# How to use gzip?
	
	cd ..
	# going back to previous directory to zip all the file in the folder 
	gzip -f initramfs.cpio
