#!/bin/bash
# Copyright (c) Wojciech Figat. All rights reserved.

set -e

testfilesize=$(wc -c < 'Source/Logo.png')
if [ $testfilesize -le 1000 ]; then
    echo "CallBuildTool ERROR: Repository was not cloned using Git LFS" 1>&2
	exit 1
fi

case "$(uname -m)" in
    "x86_64") ARCH=x64;;
    "aarch64"|"arm64") ARCH=arm64;;
    *)
        echo "Unsupported architecture $ARCH"
        exit 2
        ;;
esac

# Compile the build tool.
dotnet msbuild /nologo /verbosity:quiet "Source/Tools/Flax.Build/Flax.Build.csproj" /property:Configuration=Release /target:Restore,Build /property:RestorePackagesConfig=True /p:RuntimeIdentifiers=linux-$ARCH

# Run the build tool using the provided arguments.
Binaries/Tools/Flax.Build "$@"
