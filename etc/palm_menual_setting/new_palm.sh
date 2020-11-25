#!/bin/bash

echo "Start new palm update"


echo "Copy rules file"
sudo cp -r /home/bacs/factorydefault/palm_menual_setting/89-iris_vein_cam.rules_new /etc/udev/rules.d/89-iris_vein_cam.rules
sudo cp -r /home/bacs/factorydefault/palm_menual_setting/palmvein_add_new.sh /usr/local/bin/palmvein_add_new.sh
sudo chmod 777 /usr/local/bin/palmvein_add_new.sh

echo "Sync"
sync

echo "Finish and reboot"
sudo reboot





