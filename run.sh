#!/bin/bash


export QT_IM_MODULE=fcitx

export XMODIFIERS=@im=fcitx

export GTK_IM_MODULE=fcitx

fcitx5 &


sleep 2

export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH



./build/VehicleTerminal