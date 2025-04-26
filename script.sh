#!/bin/bash

if [ "$1" = "build" ]; then
    gcc $(pkg-config --cflags gtk4) -o theme-manager theme-manager.c $(pkg-config --libs gtk4)

elif [ "$1" = "install" ]; then
    makepkg -si

elif [ "$1" = "run-act" ]; then
    # Run act with artifact server path
    act --artifact-server-path ./artifacts

else
    echo "Usage: $0 {build|install|run-act}"
    exit 1
fi
