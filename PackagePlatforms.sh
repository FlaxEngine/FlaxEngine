#!/bin/sh
# Copyright (c) Wojciech Figat. All rights reserved.

set -e

echo Building and packaging platforms data...

# Change the path to the script root
cd "`dirname "$0"`"

# Run Flax.Build (also pass the arguments)
bash ./Development/Scripts/Linux/CallBuildTool.sh --deploy --deployPlatforms --verbose --log --logFile="Cache/Intermediate/PackageLog.txt" "$@"
