
#pragma once

#include "BitmapRef.hpp"

namespace msdfgen {

/// Saves the bitmap as an uncompressed floating-point FL32 file, which can be decoded trivially.
template <int N>
bool saveFl32(BitmapConstSection<float, N> bitmap, const char *filename);

}
