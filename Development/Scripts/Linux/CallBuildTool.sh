#!/bin/bash
# Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

set -e

testfilesize=$(wc -c < 'Source/Logo.png')
if [ $testfilesize -le 1000 ]; then
    echo "CallBuildTool ERROR: Repository was not cloned using Git LFS" 1>&2
	exit 1
fi

# Compile the build tool.
dotnet msbuild /nologo /verbosity:quiet "Source/Tools/Flax.Build/Flax.Build.csproj" /target:Restore /property:RestorePackagesConfig=True /p:RuntimeIdentifier=linux-x64
dotnet msbuild /nologo /verbosity:quiet "Source/Tools/Flax.Build/Flax.Build.csproj" /property:Configuration=Release /property:Platform=AnyCPU /target:Clean
dotnet msbuild /nologo /verbosity:quiet "Source/Tools/Flax.Build/Flax.Build.csproj" /property:Configuration=Release /property:Platform=AnyCPU /target:Build,Publish /p:PublishDir=./Binaries/Tools /p:SelfContained=true /p:RuntimeIdentifier=linux-x64

# Run the build tool using the provided arguments.
#mono --debug --debugger-agent=transport=dt_socket,server=y,address=127.0.0.1:55555 Binaries/Tools/Flax.Build.exe "$@"
Binaries/Tools/Flax.Build "$@"
