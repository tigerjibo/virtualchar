virtualchar
===========

A sample char device driver,which generates the two device based of the same driver for now.

Compile:
make
virtualchar.ko will be generated.

Insert module:
sudo insmod virtualchar.ko

Verify:
cat /proc/devices
virtualchar will be shown up with major number.

Create node:
sudo mknod /dev/virtualchar c 148 0
sudo chmod 666 /dev/virtualchar

Run:
echo 'Hello world!' > /dev/virtualchar
cat /dev/virtualchar

Also see dmesg for printk message.
