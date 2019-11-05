setenv bootargs init=/init.sh rw console=tty0 console=ttyS0,115200 earlycon=ec_imx21,0x02020000,115200n8 no_console_suspend panic=10 consoleblank=0 loglevel=1 PMOS_NO_OUTPUT_REDIRECT

printenv

echo Loading DTB
load mmc ${mmc_bootdev}:1 ${fdt_addr_r} imx6sll-evk-btwifi.dtb

echo Loading Initramfs
load mmc ${mmc_bootdev}:1 ${ramdisk_addr_r} uInitrd-kobo-clara

echo Loading Kernel
load mmc ${mmc_bootdev}:1 ${kernel_addr_r} vmlinuz-kobo-clara

echo Resizing FDT
fdt addr ${fdt_addr_r}
fdt resize

echo Booting kernel
booti ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}
