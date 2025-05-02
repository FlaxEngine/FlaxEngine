#!/bin/bash
# Copyright (c) Wojciech Figat. All rights reserved

set -e

testfilesize=$(wc -c < 'Source/Logo.png')
if [ $testfilesize -le 1000 ]; then
    echo "CallBuildTool ERROR: Repository was not cloned using Git LFS" 1>&2
	exit 1
fi

# Compile the build tool.
dotnet msbuild /nologo /verbosity:quiet "Source/Tools/Flax.Build/Flax.Build.csproj" /property:Configuration=Release /target:Restore,Build /property:RestorePackagesConfig=True /p:RuntimeIdentifiers=osx-x64

# Run the build tool using the provided arguments.
Binaries/Tools/Flax.Build "$@"
