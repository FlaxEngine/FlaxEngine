#!/bin/sh
# Copyright (c) Wojciech Figat. All rights reserved.

set -e

echo Generating Flax Engine project files...

# Change the path to the script root
cd "`dirname "$0"`"

# Run Flax.Build to generate project files (also pass the arguments)
bash ./Development/Scripts/Linux/CallBuildTool.sh --genproject "$@"

# Build bindings for all editor configurations
echo Building C# bindings...

case "$(uname -m)" in
    "x86_64") ARCH=x64;;
    "aarch64"|"arm64") ARCH=arm64;;
    *)
        echo "Unsupported architecture $ARCH"
        exit 2
        ;;
esac

Binaries/Tools/Flax.Build -build -BuildBindingsOnly -arch=$ARCH -platform=Linux --buildTargets=FlaxEditor
