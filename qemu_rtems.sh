#!/bin/bash

# (a) rtems-boot.img provides grub.cfg that, in turn,
#     runs rtems-grub.cfg from (hd0,1). In our case this is  "-hda fat:."
# (b) To get grub working the "-sdl" or "-curses" options should be added.
# (c) this is not clear yet about "-serial" option in case of kvm - it
      does not work for me.
# (d) "-s" option is  shorthand for -gdb tcp::1234, i.e. open gdbserver
#     on TCP port 1234.
# (e) probably useless options: -nodefaults -cpu pentium

qemu -nographic -sdl -m 128 -boot a -fda rtems-boot.img \
     -hda fat:. -localtime \
     -drive file=/dev/sda,if=ide,media=disk,cache=none \
     -k en-us  -no-reboot -serial stdio
