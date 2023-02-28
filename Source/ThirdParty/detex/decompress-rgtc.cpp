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

// For each pixel, decode an 8-bit integer and store as follows:
// If shift and offset are zero, store each value in consecutive 8 bit values in pixel_buffer.
// If shift is one, store each value in consecutive 16-bit words in pixel_buffer; if offset
// is zero, store it in the first 8 bits, if offset is one store it in the last 8 bits of each
// 16-bit word.
static DETEX_INLINE_ONLY void DecodeBlockRGTC(const uint8_t * DETEX_RESTRICT bitstring, int shift,
int offset, uint8_t * DETEX_RESTRICT pixel_buffer) {
	// LSBFirst byte order only.
	uint64_t bits = (*(uint64_t *)&bitstring[0]) >> 16;
	int lum0 = bitstring[0];
	int lum1 = bitstring[1];
	for (int i = 0; i < 16; i++) {
		int control_code = bits & 0x7;
		uint8_t output;
		if (lum0 > lum1)
			switch (control_code) {
			case 0 : output = lum0; break;
			case 1 : output = lum1; break;
			case 2 : output = detexDivide0To1791By7(6 * lum0 + lum1); break;
			case 3 : output = detexDivide0To1791By7(5 * lum0 + 2 * lum1); break;
			case 4 : output = detexDivide0To1791By7(4 * lum0 + 3 * lum1); break;
			case 5 : output = detexDivide0To1791By7(3 * lum0 + 4 * lum1); break;
			case 6 : output = detexDivide0To1791By7(2 * lum0 + 5 * lum1); break;
			case 7 : output = detexDivide0To1791By7(lum0 + 6 * lum1); break;
			}
		else
			switch (control_code) {
			case 0 : output = lum0; break;
			case 1 : output = lum1; break;
			case 2 : output = detexDivide0To1279By5(4 * lum0 + lum1); break;
			case 3 : output = detexDivide0To1279By5(3 * lum0 + 2 * lum1); break;
			case 4 : output = detexDivide0To1279By5(2 * lum0 + 3 * lum1); break;
			case 5 : output = detexDivide0To1279By5(lum0 + 4 * lum1); break;
			case 6 : output = 0; break;
			case 7 : output = 0xFF; break;
			}
		pixel_buffer[(i << shift) + offset] = output;
		bits >>= 3;
	}
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* unsigned RGTC1 (BC4) format. */
bool detexDecompressBlockRGTC1(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	DecodeBlockRGTC(bitstring, 0, 0, pixel_buffer);
	return true;
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* unsigned RGTC2 (BC5) format. */
bool detexDecompressBlockRGTC2(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	DecodeBlockRGTC(bitstring, 1, 0, pixel_buffer);
	DecodeBlockRGTC(&bitstring[8], 1, 1, pixel_buffer);
	return true;
}

// For each pixel, decode an 16-bit integer and store as follows:
// If shift and offset are zero, store each value in consecutive 16 bit values in pixel_buffer.
// If shift is one, store each value in consecutive 32-bit words in pixel_buffer; if offset
// is zero, store it in the first 16 bits, if offset is one store it in the last 16 bits of each
// 32-bit word. Returns true if the compressed block is valid.
static DETEX_INLINE_ONLY bool DecodeBlockSignedRGTC(const uint8_t * DETEX_RESTRICT bitstring, int shift,
int offset, uint8_t * DETEX_RESTRICT pixel_buffer) {
	// LSBFirst byte order only.
	uint64_t bits = (*(uint64_t *)&bitstring[0]) >> 16;
	int lum0 = (int8_t)bitstring[0];
	int lum1 = (int8_t)bitstring[1];
	if (lum0 == - 127 && lum1 == - 128)
		// Not allowed.
		return false;
	if (lum0 == - 128)
		lum0 = - 127;
	if (lum1 == - 128)
		lum1 = - 127;
	// Note: values are mapped to a red value of -127 to 127.
	uint16_t *pixel16_buffer = (uint16_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int control_code = bits & 0x7;
		int32_t result;
		if (lum0 > lum1)
			switch (control_code) {
			case 0 : result = lum0; break;
			case 1 : result = lum1; break;
			case 2 : result = detexDivideMinus895To895By7(6 * lum0 + lum1); break;
			case 3 : result = detexDivideMinus895To895By7(5 * lum0 + 2 * lum1); break;
			case 4 : result = detexDivideMinus895To895By7(4 * lum0 + 3 * lum1); break;
			case 5 : result = detexDivideMinus895To895By7(3 * lum0 + 4 * lum1); break;
			case 6 : result = detexDivideMinus895To895By7(2 * lum0 + 5 * lum1); break;
			case 7 : result = detexDivideMinus895To895By7(lum0 + 6 * lum1); break;
			}
		else
			switch (control_code) {
			case 0 : result = lum0; break;
			case 1 : result = lum1; break;
			case 2 : result = detexDivideMinus639To639By5(4 * lum0 + lum1); break;
			case 3 : result = detexDivideMinus639To639By5(3 * lum0 + 2 * lum1); break;
			case 4 : result = detexDivideMinus639To639By5(2 * lum0 + 3 * lum1); break;
			case 5 : result = detexDivideMinus639To639By5(lum0 + 4 * lum1); break;
			case 6 : result = - 127; break;
			case 7 : result = 127; break;
			}
		// Map from [-127, 127] to [-32768, 32767].
		pixel16_buffer[(i << shift) + offset] = (uint16_t)(int16_t)
			((result + 127) * 65535 / 254 - 32768);
		bits >>= 3;
	}
	return true;
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* signed RGTC1 (signed BC4) format. */
bool detexDecompressBlockSIGNED_RGTC1(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	return DecodeBlockSignedRGTC(bitstring, 0, 0, pixel_buffer);
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* signed RGTC2 (signed BC5) format. */
bool detexDecompressBlockSIGNED_RGTC2(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	bool r = DecodeBlockSignedRGTC(bitstring, 1, 0, pixel_buffer);
	if (!r)
		return false;
	return DecodeBlockSignedRGTC(&bitstring[8], 1, 1, pixel_buffer);
}

