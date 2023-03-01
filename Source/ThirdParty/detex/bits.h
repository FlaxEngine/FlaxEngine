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

/* Data structure used to extract bits from 128-bit bitstring. */

typedef struct {
	uint64_t data0;
	uint64_t data1;
	int index;
} detexBlock128;

uint32_t detexBlock128ExtractBits(detexBlock128 *block, int nu_bits);

/* Return bitfield from bit0 to bit1 from 64-bit bitstring. */
static DETEX_INLINE_ONLY uint32_t detexGetBits64(uint64_t data, int bit0, int bit1) {
	return (data & (((uint64_t)1 << (bit1 + 1)) - 1)) >> bit0;
}

/* Return reversed bitfield (bit1 to bit0) from 64-bit bitstring. */
static DETEX_INLINE_ONLY uint32_t detexGetBits64Reversed(uint64_t data, int bit0, int bit1) {
	// Assumes bit0 > bit1.
	// Reverse the bits.
	uint32_t val = 0;
	for (int i = 0; i <= bit0 - bit1; i++) {
		int shift_right = bit0 - 2 * i;
		if (shift_right >= 0)
			val |= (data & ((uint64_t)1 << (bit0 - i))) >> shift_right;
		else
			val |= (data & ((uint64_t)1 << (bit0 - i))) << (- shift_right);
	}
	return val;
}

/* Clear bit0 to bit1 of 64-bit bitstring. */
static DETEX_INLINE_ONLY uint64_t detexClearBits64(uint64_t data, int bit0, int bit1) {
	uint64_t mask = ~(((uint64_t)1 << (bit1 + 1)) - 1);
	mask |= ((uint64_t)1 << bit0) - 1;
	return data & mask;
}

/* Set bit0 to bit1 of 64-bit bitstring. */
static DETEX_INLINE_ONLY uint64_t detexSetBits64(uint64_t data, int bit0, int bit1, uint64_t val) {
	uint64_t d = detexClearBits64(data, bit0, bit1);
	d |= val << bit0;
	return d;
}

