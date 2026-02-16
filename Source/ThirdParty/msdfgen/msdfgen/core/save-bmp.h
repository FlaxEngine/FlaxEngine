
#pragma once

#include "BitmapRef.hpp"

namespace msdfgen {

/// Saves the bitmap as a BMP file.
bool saveBmp(BitmapConstSection<byte, 1> bitmap, const char *filename);
bool saveBmp(BitmapConstSection<byte, 3> bitmap, const char *filename);
bool saveBmp(BitmapConstSection<byte, 4> bitmap, const char *filename);
bool saveBmp(BitmapConstSection<float, 1> bitmap, const char *filename);
bool saveBmp(BitmapConstSection<float, 3> bitmap, const char *filename);
bool saveBmp(BitmapConstSection<float, 4> bitmap, const char *filename);

}
