#!/bin/bash
./build/linuxdeployqt ./build/track -appimage
rm ./build/colorGroup.txt
scp -r ./build/ user@10.0.0.34:/home/user/ 

