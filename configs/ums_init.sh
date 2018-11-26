#!/system/bin/sh
BB=/system/xbin/busybox
LUN_NUM=2
LUN_PATH="/sys/class/android_usb/f_mass_storage/lun%d/file"
VOLD_MANAGED=$( $BB mount | $BB grep "vold" | $BB cut -d " " -f1 |\
$BB sort -u | $BB head -n $LUN_NUM | $BB tr "\n" " " )
i=0
for VOLUME in $VOLD_MANAGED
do
	LUN=$($BB printf $LUN_PATH $i)
	$BB echo $VOLUME > $LUN
	i=$(($i + 1))
done
