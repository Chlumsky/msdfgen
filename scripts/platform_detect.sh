#!/bin/bash

# Detect current build platform

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    platform_suffix="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    platform_suffix="mac"
elif [[ "$OSTYPE" == "cygwin" ]]; then
    platform_suffix="win"
elif [[ "$OSTYPE" == "msys" ]]; then
    platform_suffix="win"
elif [[ "$OSTYPE" == "win32" ]]; then
    platform_suffix="win"
elif [[ "$OSTYPE" == "freebsd"* ]]; then
    platform_suffix="freebsd"
else
    echo "Unknown operating system: $OSTYPE"
    exit 1
fi