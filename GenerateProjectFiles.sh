#!/bin/sh
# Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

set -e

echo Generating Flax Engine project files...

# Change the path to the script root
cd "`dirname "$0"`"

# Run Flax.Build to generate project files (also pass the arguments)
bash ./Development/Scripts/Linux/CallBuildTool.sh --genproject "$@"
