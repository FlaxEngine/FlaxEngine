#!/bin/bash
# Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

set -e

# Compile the build tool.
xbuild /nologo /verbosity:quiet "Source/Tools/Flax.Build/Flax.Build.csproj" /property:Configuration=Release /property:Platform=AnyCPU /target:Build

# Run the build tool using the provided arguments.
#mono --debug --debugger-agent=transport=dt_socket,server=y,address=127.0.0.1:55555 Binaries/Tools/Flax.Build.exe "$@"
mono Binaries/Tools/Flax.Build.exe "$@"
