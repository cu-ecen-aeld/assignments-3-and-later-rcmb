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
echo $FINDER_APP_DIR
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

    echo "Deep clean existing kernel configuration files"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    echo "Create default kernel configuration"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    echo "Build Kernel image"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    echo "Build Kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    echo "Build Kernel devicetree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

    echo "Finished building kernel image"
fi

echo "Copy image to outdir root"
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]
then
 echo "Missing kernel image"
 exit 1
else
 cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/
fi


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

echo "Create necessary base directories"
mkdir -p "${OUTDIR}/rootfs"
cd "${OUTDIR}/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Configure busybox"
    make distclean
    make defconfig
else
    cd busybox
fi

echo "Make and install busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install


echo "Library dependencies"
sudo env "PATH=$PATH" ${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
sudo env "PATH=$PATH" ${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

echo "Add library dependencies to rootfs"
cd "${OUTDIR}/rootfs"
export SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp -a $SYSROOT/lib64/ld-2.31.so lib64
cp -a $SYSROOT/lib64/libm.so.6 lib64
cp -a $SYSROOT/lib64/libm-2.31.so lib64
cp -a $SYSROOT/lib64/libresolv.so.2 lib64
cp -a $SYSROOT/lib64/libresolv-2.31.so lib64
cp -a $SYSROOT/lib64/libc.so.6 lib64
cp -a $SYSROOT/lib64/libc-2.31.so lib64

echo "Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

echo "Clean and build the writer utility"
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE

echo "Copy the finder related scripts and executables to the /home directory - on the target rootfs"
if [ -f writer ]
then
 cp writer $OUTDIR/rootfs/home/
 cp finder.sh $OUTDIR/rootfs/home/
 mkdir $OUTDIR/rootfs/home/conf/
 cp conf/* $OUTDIR/rootfs/home/conf/
 cp finder-test.sh $OUTDIR/rootfs/home/
 cp autorun-qemu.sh $OUTDIR/rootfs/home/
else
 echo "Failed to cross-compile writer utility."
 exit 1
fi

echo "Chown the root directory"
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *

echo "Create initramfs.cpio.gz"
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio

