/*

Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include "detex.h"

/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC1 */
/* format. */
bool detexDecompressBlockBC1(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	uint32_t colors = *(uint32_t *)&bitstring[0];
#else
	uint32_t colors = ((uint32_t)bitstring[0] << 24) |
		((uint32_t)bitstring[1] << 16) |
		((uint32_t)bitstring[2] << 8) | bitstring[3];
#endif
	// Decode the two 5-6-5 RGB colors.
	int color_r[4], color_g[4], color_b[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	if ((colors & 0xFFFF) > ((colors & 0xFFFF0000) >> 16)) {
		color_r[2] = detexDivide0To767By3(2 * color_r[0] + color_r[1]);
		color_g[2] = detexDivide0To767By3(2 * color_g[0] + color_g[1]);
		color_b[2] = detexDivide0To767By3(2 * color_b[0] + color_b[1]);
		color_r[3] = detexDivide0To767By3(color_r[0] + 2 * color_r[1]);
		color_g[3] = detexDivide0To767By3(color_g[0] + 2 * color_g[1]);
		color_b[3] = detexDivide0To767By3(color_b[0] + 2 * color_b[1]);
	}
	else {
		color_r[2] = (color_r[0] + color_r[1]) / 2;
		color_g[2] = (color_g[0] + color_g[1]) / 2;
		color_b[2] = (color_b[0] + color_b[1]) / 2;
		color_r[3] = color_g[3] = color_b[3] = 0;
	}
	uint32_t pixels = *(uint32_t *)&bitstring[4];
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		*(uint32_t *)(pixel_buffer + i * 4) = detexPack32RGB8Alpha0xFF(
			color_r[pixel], color_g[pixel], color_b[pixel]);
	}
	return true;
}

uint32_t detexGetModeBC1(const uint8_t *bitstring) {
	uint32_t colors = *(uint32_t *)bitstring;
	if ((colors & 0xFFFF) > ((colors & 0xFFFF0000) >> 16))
		return 0;
	else
		return 1;
}

void detexSetModeBC1(uint8_t *bitstring, uint32_t mode, uint32_t flags,
uint32_t *colors) {
	uint32_t colorbits = *(uint32_t *)bitstring;
	uint32_t current_mode;
	if ((colorbits & 0xFFFF) > ((colorbits & 0xFFFF0000) >> 16))
		current_mode = 0;
	else
		current_mode = 1;
	if (current_mode != mode) {
		colorbits = ((colorbits & 0xFFFF) << 16) | (colorbits >> 16);
		*(uint32_t *)bitstring = colorbits;
	}
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC1A */
/* format. */
bool detexDecompressBlockBC1A(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	uint32_t colors = *(uint32_t *)&bitstring[0];
#else
	uint32_t colors = ((uint32_t)bitstring[0] << 24) |
		((uint32_t)bitstring[1] << 16) |
		((uint32_t)bitstring[2] << 8) | bitstring[3];
#endif
	bool opaque = ((colors & 0xFFFF) > ((colors & 0xFFFF0000) >> 16));
	if (opaque && (flags & DETEX_DECOMPRESS_FLAG_NON_OPAQUE_ONLY))
		return false;
	if (!opaque && (flags & DETEX_DECOMPRESS_FLAG_OPAQUE_ONLY))
		return false;
	// Decode the two 5-6-5 RGB colors.
	int color_r[4], color_g[4], color_b[4], color_a[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	color_a[0] = color_a[1] = color_a[2] = color_a[3] = 0xFF;
	if (opaque) {
		color_r[2] = detexDivide0To767By3(2 * color_r[0] + color_r[1]);
		color_g[2] = detexDivide0To767By3(2 * color_g[0] + color_g[1]);
		color_b[2] = detexDivide0To767By3(2 * color_b[0] + color_b[1]);
		color_r[3] = detexDivide0To767By3(color_r[0] + 2 * color_r[1]);
		color_g[3] = detexDivide0To767By3(color_g[0] + 2 * color_g[1]);
		color_b[3] = detexDivide0To767By3(color_b[0] + 2 * color_b[1]);
	}
	else {
		color_r[2] = (color_r[0] + color_r[1]) / 2;
		color_g[2] = (color_g[0] + color_g[1]) / 2;
		color_b[2] = (color_b[0] + color_b[1]) / 2;
		color_r[3] = color_g[3] = color_b[3] = color_a[3] = 0;
	}
	uint32_t pixels = *(uint32_t *)&bitstring[4];
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		*(uint32_t *)(pixel_buffer + i * 4) = detexPack32RGBA8(
			color_r[pixel], color_g[pixel], color_b[pixel],
			color_a[pixel]);
	}
	return true;
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC2 */
/* format. */
bool detexDecompressBlockBC2(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	uint32_t colors = *(uint32_t *)&bitstring[8];
#else
	uint32_t colors = ((uint32_t)bitstring[8] << 24) |
		((uint32_t)bitstring[9] << 16) |
		((uint32_t)bitstring[10] << 8) | bitstring[11];
#endif
	if ((colors & 0xFFFF) <= ((colors & 0xFFFF0000) >> 16) &&
	(flags & DETEX_DECOMPRESS_FLAG_ENCODE))
		// GeForce 6 and 7 series produce wrong result in this case.
		return false;
	int color_r[4], color_g[4], color_b[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	color_r[2] = detexDivide0To767By3(2 * color_r[0] + color_r[1]);
	color_g[2] = detexDivide0To767By3(2 * color_g[0] + color_g[1]);
	color_b[2] = detexDivide0To767By3(2 * color_b[0] + color_b[1]);
	color_r[3] = detexDivide0To767By3(color_r[0] + 2 * color_r[1]);
	color_g[3] = detexDivide0To767By3(color_g[0] + 2 * color_g[1]);
	color_b[3] = detexDivide0To767By3(color_b[0] + 2 * color_b[1]);
	uint32_t pixels = *(uint32_t *)&bitstring[12];
	uint64_t alpha_pixels = *(uint64_t *)&bitstring[0];
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		int alpha = ((alpha_pixels >> (i * 4)) & 0xF) * 255 / 15;
		*(uint32_t *)(pixel_buffer + i * 4) = detexPack32RGBA8(
			color_r[pixel], color_g[pixel], color_b[pixel], alpha);
	}
	return true;
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC3 */
/* format. */
bool detexDecompressBlockBC3(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	int alpha0 = bitstring[0];
	int alpha1 = bitstring[1];
	if (alpha0 > alpha1 && (flags & DETEX_DECOMPRESS_FLAG_OPAQUE_ONLY))
		return false;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	uint32_t colors = *(uint32_t *)&bitstring[8];
#else
	uint32_t colors = ((uint32_t)bitstring[8] << 24) |
		((uint32_t)bitstring[9] << 16) |
		((uint32_t)bitstring[10] << 8) | bitstring[11];
#endif
	if ((colors & 0xFFFF) <= ((colors & 0xFFFF0000) >> 16) &&
	(flags & DETEX_DECOMPRESS_FLAG_ENCODE))
		// GeForce 6 and 7 series produce wrong result in this case.
		return false;
	int color_r[4], color_g[4], color_b[4];
	// color_x[] has a value between 0 and 248 with the lower three bits zero.
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	color_r[2] = detexDivide0To767By3(2 * color_r[0] + color_r[1]);
	color_g[2] = detexDivide0To767By3(2 * color_g[0] + color_g[1]);
	color_b[2] = detexDivide0To767By3(2 * color_b[0] + color_b[1]);
	color_r[3] = detexDivide0To767By3(color_r[0] + 2 * color_r[1]);
	color_g[3] = detexDivide0To767By3(color_g[0] + 2 * color_g[1]);
	color_b[3] = detexDivide0To767By3(color_b[0] + 2 * color_b[1]);
	uint32_t pixels = *(uint32_t *)&bitstring[12];
	uint64_t alpha_bits = (uint32_t)bitstring[2] |
		((uint32_t)bitstring[3] << 8) |
		((uint64_t)*(uint32_t *)&bitstring[4] << 16);
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		int code = (alpha_bits >> (i * 3)) & 0x7;
		int alpha;
		if (alpha0 > alpha1)
			switch (code) {
			case 0 : alpha = alpha0; break;
			case 1 : alpha = alpha1; break;
			case 2 : alpha = detexDivide0To1791By7(6 * alpha0 + 1 * alpha1); break;
			case 3 : alpha = detexDivide0To1791By7(5 * alpha0 + 2 * alpha1); break;
			case 4 : alpha = detexDivide0To1791By7(4 * alpha0 + 3 * alpha1); break;
			case 5 : alpha = detexDivide0To1791By7(3 * alpha0 + 4 * alpha1); break;
			case 6 : alpha = detexDivide0To1791By7(2 * alpha0 + 5 * alpha1); break;
			case 7 : alpha = detexDivide0To1791By7(1 * alpha0 + 6 * alpha1); break;
			}
		else
			switch (code) {
			case 0 : alpha = alpha0; break;
			case 1 : alpha = alpha1; break;
			case 2 : alpha = detexDivide0To1279By5(4 * alpha0 + 1 * alpha1); break;
			case 3 : alpha = detexDivide0To1279By5(3 * alpha0 + 2 * alpha1); break;
			case 4 : alpha = detexDivide0To1279By5(2 * alpha0 + 3 * alpha1); break;
			case 5 : alpha = detexDivide0To1279By5(1 * alpha0 + 4 * alpha1); break;
			case 6 : alpha = 0; break;
			case 7 : alpha = 0xFF; break;
			}
		*(uint32_t *)(pixel_buffer + i * 4) = detexPack32RGBA8(
			color_r[pixel], color_g[pixel], color_b[pixel], alpha);
	}
	return true;
}

