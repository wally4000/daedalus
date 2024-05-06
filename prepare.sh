#!/bin/bash

#This prepares the OS for installation

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
