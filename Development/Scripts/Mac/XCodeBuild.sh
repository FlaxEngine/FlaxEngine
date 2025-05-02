#!/bin/sh
# Copyright (c) Wojciech Figat. All rights reserved

# Fix mono bin to be in a path
#export PATH=/Library/Frameworks/Mono.framework/Versions/Current/Commands:$PATH

echo "Running Flax.Build $*"
Binaries/Tools/Flax.Build "$@"
