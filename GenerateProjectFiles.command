#!/bin/sh
# Copyright (c) Wojciech Figat. All rights reserved.

set -e

echo Generating Flax Engine project files...

# Change the path to the script root
cd "`dirname "$0"`"

# Run Flax.Build to generate project files (also pass the arguments)
bash ./Development/Scripts/Mac/CallBuildTool.sh --genproject "$@"

# Build bindings for all editor configurations
echo Building C# bindings...
# TODO: Detect the correct architecture here
Binaries/Tools/Flax.Build -build -BuildBindingsOnly -arch=ARM64 -platform=Mac --buildTargets=FlaxEditor
