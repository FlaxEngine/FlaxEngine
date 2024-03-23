#!/bin/sh
# Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

set -e

echo Building and packaging platforms data...

# Change the path to the script root
cd "`dirname "$0"`"

# Run Flax.Build (also pass the arguments)
bash ./Development/Scripts/Mac/CallBuildTool.sh --deploy --deployPlatforms --dotnet=8 --verbose --log --logFile="Cache/Intermediate/PackageLog.txt" "$@"
