#!/bin/sh
mod=`lsmod | grep fjveincam | wc -l`
if [ $mod -lt 1 ]
then

	insmod /usr/local/bin/fjveincam.ko
	sleep 1
fi

for X in /sys/bus/usb/devices/*; do
    value=$(cat "$X/idProduct" 2>/dev/null)
    echo "value="$value
    if [ $value -eq "1526" ];
    then
            echo "$X"
            cat "$X/idVendor" 2>/dev/null
        cat "$X/idProduct" 2>/dev/null
        echo "found"
        palmDev=${X##*/}
    fi
done

echo $palmDev

idx=`ls -alh /sys/class/usbmisc/ | grep $palmDev | awk '/fjveincam/{print substr($9,10)}'`
echo fjveincam$idx

rm -Rf /dev/usb/fjveincam*
mkdir -p /dev/usb/
mknod -m 666 /dev/usb/fjveincam$idx c 180 $idx

killall java
