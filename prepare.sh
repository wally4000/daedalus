#!/bin/bash

# This file is realistically for CI which keeps all the manual build steps out of there..
# It will work on your computer but results may be unpredictable.

# DaedalusX64 Team


#Handle macOS first
if [ "$(uname -s)" = "Darwin" ]; then
  ## Check if using brew
  if command -v brew &> /dev/null; then
    brew update
	brew install git bash cmake libpng minizip sdl2 sdl2_ttf glew
  fi

else
    TESTOS=$(cat /etc/os-release | grep -w "ID" | cut -d '=' -f2)

    case $TESTOS in

    ubuntu | debian)
        sudo apt-get update
        sudo apt-get -y install build-essential git bash cmake libpng-dev libz-dev libminizip-dev libsdl2-dev libsdl2-ttf-dev libglew-dev
    ;;
    *)
        echo "$TESTOS not supported here"
    ;;
    esac

fi
# Use for CI
if [[ ! -z "$DEVKITPRO" ]]; then
echo "3DS Build Prep"
    sudo apt-get update 
    sudo apt-get -y install g++ libyaml-dev libmbedtls-dev
    
    echo "Build MakeROM"
    git clone https://github.com/wally4000/Project_CTR --depth=1
    cd Project_CTR/
    make
    cp makerom/bin/makerom /usr/local/bin

    echo "Build PicaGL"
    git clone -b revamp https://github.com/masterfeizz/picaGL --depth=1
    cd picaGL
    DEVKITARM='/opt/devkitpro/devkitARM' make 
    DEVKITARM='/opt/devkitpro/devkitARM' make install

    echo "Build IMGUI-picaGL"
    git clone
    cd imgui-picagl
    DEVKITARM='/opt/devkitpro/devkitARM' make 
    DEVKITARM='/opt/devkitpro/devkitARM' make install
fi
