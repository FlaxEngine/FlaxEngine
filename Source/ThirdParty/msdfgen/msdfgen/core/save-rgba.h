
#pragma once

#include "BitmapRef.hpp"

namespace msdfgen {

/// Saves the bitmap as a simple RGBA file, which can be decoded trivially.
bool saveRgba(BitmapConstSection<byte, 1> bitmap, const char *filename);
bool saveRgba(BitmapConstSection<byte, 3> bitmap, const char *filename);
bool saveRgba(BitmapConstSection<byte, 4> bitmap, const char *filename);
bool saveRgba(BitmapConstSection<float, 1> bitmap, const char *filename);
bool saveRgba(BitmapConstSection<float, 3> bitmap, const char *filename);
bool saveRgba(BitmapConstSection<float, 4> bitmap, const char *filename);

}
