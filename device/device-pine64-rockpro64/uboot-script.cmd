setenv bootargs earlycon=uart8250,mmio32,0xff1a0000 init=/init.sh console=tty0 console=ttyS2,1500000n8 PMOS_NO_OUTPUT_REDIRECT

printenv

echo Loading DTB
load mmc ${devnum}:1 ${fdt_addr_r} rk3399-rockpro64.dtb

echo Loading Initramfs
load mmc ${devnum}:1 ${ramdisk_addr_r} uInitrd-postmarketos-mainline

echo Loading Kernel
load mmc ${devnum}:1 ${kernel_addr_r} vmlinuz-postmarketos-mainline

echo Resizing FDT
fdt addr ${fdt_addr_r}
fdt resize

echo Booting kernel
booti ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}
