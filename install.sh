#!/bin/sh
umount /media/hdd
#mdel -i /dev/disk/by-id/usb-Kingston_DataTraveler_II+_5B6C0E811A91-0:0-part1 ::/vmlinux
mdel -i /dev/disk/by-id/usb-Kingston_DataTraveler_II+_5B6C0E811A91-0:0-part1 ::/uImage

#mcopy -i /dev/disk/by-id/usb-Kingston_DataTraveler_II+_5B6C0E811A91-0:0-part1 vmlinux.strip ::/vmlinux
mcopy -i /dev/disk/by-id/usb-Kingston_DataTraveler_II+_5B6C0E811A91-0:0-part1 uImage ::/uImage

sync
