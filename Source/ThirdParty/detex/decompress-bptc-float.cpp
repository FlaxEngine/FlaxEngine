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
#include "bits.h"
#include "bptc-tables.h"

static const int8_t map_mode_table[32] = {
	0, 1, 2, 10, -1, -1, 3, 11, -1, -1, 4, 12, -1, -1, 5, 13,
	-1, -1, 6, -1, -1, -1, 7, -1, -1, -1, 8, -1, -1, -1, 9, -1
};

static int ExtractMode(detexBlock128 *block) {
	uint32_t mode = detexBlock128ExtractBits(block, 2);
	if (mode < 2)
		return mode;
	return map_mode_table[mode | (detexBlock128ExtractBits(block, 3) << 2)];
}

static int GetPartitionIndex(int nu_subsets, int partition_set_id, int i) {
	if (nu_subsets == 1)
		return 0;
	// nu_subset == 2
	return detex_bptc_table_P2[partition_set_id * 16 + i];
}

static const uint8_t bptc_float_EPB[14] = {
	10, 7, 11, 11, 11, 9, 8, 8, 8, 6, 10, 11, 12, 16 };

static DETEX_INLINE_ONLY int GetAnchorIndex(int partition_set_id, int partition, int nu_subsets) {
	if (partition == 0)
		return 0;
	// nu_subsets = 2, partition = 1.
	return detex_bptc_table_anchor_index_second_subset[partition_set_id];
}

static uint32_t Unquantize(uint16_t x, int mode) {
	int32_t unq;
	if (mode == 13)
		unq = x;
	else if (x == 0)
		unq = 0;
	else if (x == (((int32_t)1 << bptc_float_EPB[mode]) - 1))
		unq = 0xFFFF;
	else
		unq = (((int32_t)x << 15) + 0x4000) >> (bptc_float_EPB[mode] - 1);
	return unq;
}

static int32_t UnquantizeSigned(int16_t x, int mode) {
	int s = 0;
	int32_t unq;
	if (bptc_float_EPB[mode] >= 16)
		unq = x;
	else {
		if (x < 0) {
			s = 1;
			x = -x;
		}
		if (x == 0)
			unq = 0;
		else
		if (x >= (((int32_t)1 << (bptc_float_EPB[mode] - 1)) - 1))
			unq = 0x7FFF;
		else
			unq = (((int32_t)x << 15) + 0x4000) >> (bptc_float_EPB[mode] - 1);
		if (s)
			unq = -unq;
	}
	return unq;
}

static int SignExtend(int value, int source_nu_bits, int target_nu_bits) {
	uint32_t sign_bit = value & (1 << (source_nu_bits - 1));
	if (!sign_bit)
		return value;
	uint32_t sign_extend_bits = 0xFFFFFFFF ^ ((1 << source_nu_bits) - 1);
	sign_extend_bits &= ((uint64_t)1 << target_nu_bits) - 1;
	return value | sign_extend_bits;
}

static int32_t InterpolateFloat(int32_t e0, int32_t e1, int16_t index, uint8_t indexprecision) {
	if (indexprecision == 2)
		return (((64 - detex_bptc_table_aWeight2[index]) * e0
			+ detex_bptc_table_aWeight2[index] * e1 + 32) >> 6);
	else
	if (indexprecision == 3)
		return (((64 - detex_bptc_table_aWeight3[index]) * e0
			+ detex_bptc_table_aWeight3[index] * e1 + 32) >> 6);
	else // indexprecision == 4
		return (((64 - detex_bptc_table_aWeight4[index]) * e0
			+ detex_bptc_table_aWeight4[index] * e1 + 32) >> 6);
}

static bool DecompressBlockBPTCFloatShared(const uint8_t * DETEX_RESTRICT bitstring,
uint32_t mode_mask, uint32_t flags, bool signed_flag,
const uint8_t * DETEX_RESTRICT pixel_buffer) {
	detexBlock128 block;
	block.data0 = *(uint64_t *)&bitstring[0];
	block.data1 = *(uint64_t *)&bitstring[8];
	block.index = 0;
	uint32_t mode = ExtractMode(&block);
	if (mode == - 1)
		return false;
	// Allow compression tied to specific modes (according to mode_mask).
	if (!(mode_mask & ((int)1 << mode)))
		return false;
	int32_t r[4], g[4], b[4];
	int partition_set_id = 0;
	int delta_bits_r, delta_bits_g, delta_bits_b;
	uint64_t data0 = block.data0;
	uint64_t data1 = block.data1;
	switch (mode) {
	case 0 :
		// m[1:0],g2[4],b2[4],b3[4],r0[9:0],g0[9:0],b0[9:0],r1[4:0],g3[4],g2[3:0],
		// g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
		g[2] = detexGetBits64(data0, 2, 2) << 4;
		b[2] = detexGetBits64(data0, 3, 3) << 4;
		b[3] = detexGetBits64(data0, 4, 4) << 4;
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 39);
		g[3] = detexGetBits64(data0, 40, 40) << 4;
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 49);
		b[3] |= detexGetBits64(data0, 50, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 59);
		b[3] |= detexGetBits64(data0, 60, 60) << 1;
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 5);
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 11);
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = delta_bits_b = 5;
		break;
	case 1 :
		// m[1:0],g2[5],g3[4],g3[5],r0[6:0],b3[0],b3[1],b2[4],g0[6:0],b2[5],b3[2],
		// g2[4],b0[6:0],b3[3],b3[5],b3[4],r1[5:0],g2[3:0],g1[5:0],g3[3:0],b1[5:0],
		// b2[3:0],r2[5:0],r3[5:0]
		g[2] = detexGetBits64(data0, 2, 2) << 5;
		g[3] = detexGetBits64(data0, 3, 3) << 4;
		g[3] |= detexGetBits64(data0, 4, 4) << 5;
		r[0] = detexGetBits64(data0, 5, 11);
		b[3] = detexGetBits64(data0, 12, 12);
		b[3] |= detexGetBits64(data0, 13, 13) << 1;
		b[2] = detexGetBits64(data0, 14, 14) << 4;
		g[0] = detexGetBits64(data0, 15, 21);
		b[2] |= detexGetBits64(data0, 22, 22) << 5;
		b[3] |= detexGetBits64(data0, 23, 23) << 2;
		g[2] |= detexGetBits64(data0, 24, 24) << 4;
		b[0] = detexGetBits64(data0, 25, 31);
		b[3] |= detexGetBits64(data0, 32, 32) << 3;
		b[3] |= detexGetBits64(data0, 33, 33) << 5;
		b[3] |= detexGetBits64(data0, 34, 34) << 4;
		r[1] = detexGetBits64(data0, 35, 40);
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 60);
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 6);
		r[3] = detexGetBits64(data1, 7, 12);
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = delta_bits_b = 6;
		break;
	case 2 :
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[4:0],r0[10],g2[3:0],g1[3:0],g0[10],
		// b3[0],g3[3:0],b1[3:0],b0[10],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 39);
		r[0] |= detexGetBits64(data0, 40, 40) << 10;
		g[2] = detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 48);
		g[0] |= detexGetBits64(data0, 49, 49) << 10;
		b[3] = detexGetBits64(data0, 50, 50);
		g[3] = detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 58);
		b[0] |= detexGetBits64(data0, 59, 59) << 10;
		b[3] |= detexGetBits64(data0, 60, 60) << 1;
		b[2] = detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 5);
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 11);
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = 5;
		delta_bits_g = delta_bits_b = 4;
		break;
	case 3 :	// Original mode 6.
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10],g3[4],g2[3:0],g1[4:0],
		// g0[10],g3[3:0],b1[3:0],b0[10],b3[1],b2[3:0],r2[3:0],b3[0],b3[2],r3[3:0],
		// g2[4],b3[3]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 38);
		r[0] |= detexGetBits64(data0, 39, 39) << 10;
		g[3] = detexGetBits64(data0, 40, 40) << 4;
		g[2] = detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 49);
		g[0] |= detexGetBits64(data0, 50, 50) << 10;
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 58);
		b[0] |= detexGetBits64(data0, 59, 59) << 10;
		b[3] = detexGetBits64(data0, 60, 60) << 1;
		b[2] = detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 4);
		b[3] |= detexGetBits64(data1, 5, 5);
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 10);
		g[2] |= detexGetBits64(data1, 11, 11) << 4;
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_b = 4;
		delta_bits_g = 5;
		break;
	case 4 :	// Original mode 10.
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10],b2[4],g2[3:0],g1[3:0],
		// g0[10],b3[0],g3[3:0],b1[4:0],b0[10],b2[3:0],r2[3:0],b3[1],b3[2],r3[3:0],
		// b3[4],b3[3]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 38);
		r[0] |= detexGetBits64(data0, 39, 39) << 10;
		b[2] = detexGetBits64(data0, 40, 40) << 4;
		g[2] = detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 48);
		g[0] |= detexGetBits64(data0, 49, 49) << 10;
		b[3] = detexGetBits64(data0, 50, 50);
		g[3] = detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 59);
		b[0] |= detexGetBits64(data0, 60, 60) << 10;
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 4);
		b[3] |= detexGetBits64(data1, 5, 5) << 1;
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 10);
		b[3] |= detexGetBits64(data1, 11, 11) << 4;
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = 4;
		delta_bits_b = 5;
		break;
	case 5 :	// Original mode 14
		// m[4:0],r0[8:0],b2[4],g0[8:0],g2[4],b0[8:0],b3[4],r1[4:0],g3[4],g2[3:0],
		// g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
		r[0] = detexGetBits64(data0, 5, 13);
		b[2] = detexGetBits64(data0, 14, 14) << 4;
		g[0] = detexGetBits64(data0, 15, 23);
		g[2] = detexGetBits64(data0, 24, 24) << 4;
		b[0] = detexGetBits64(data0, 25, 33);
		b[3] = detexGetBits64(data0, 34, 34) << 4;
		r[1] = detexGetBits64(data0, 35, 39);
		g[3] = detexGetBits64(data0, 40, 40) << 4;
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 49);
		b[3] |= detexGetBits64(data0, 50, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 59);
		b[3] |= detexGetBits64(data0, 60, 60) << 1;
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 5);
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 11);
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = delta_bits_b = 5;
		break;
	case 6 :	// Original mode 18
		// m[4:0],r0[7:0],g3[4],b2[4],g0[7:0],b3[2],g2[4],b0[7:0],b3[3],b3[4],
		// r1[5:0],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[5:0],r3[5:0]
		r[0] = detexGetBits64(data0, 5, 12);
		g[3] = detexGetBits64(data0, 13, 13) << 4;
		b[2] = detexGetBits64(data0, 14, 14) << 4;
		g[0] = detexGetBits64(data0, 15, 22);
		b[3] = detexGetBits64(data0, 23, 23) << 2;
		g[2] = detexGetBits64(data0, 24, 24) << 4;
		b[0] = detexGetBits64(data0, 25, 32);
		b[3] |= detexGetBits64(data0, 33, 33) << 3;
		b[3] |= detexGetBits64(data0, 34, 34) << 4;
		r[1] = detexGetBits64(data0, 35, 40);
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 49);
		b[3] |= detexGetBits64(data0, 50, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 59);
		b[3] |= detexGetBits64(data0, 60, 60) << 1;
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 6);
		r[3] = detexGetBits64(data1, 7, 12);
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = 6;
		delta_bits_g = delta_bits_b = 5;
		break;
	case 7 :	// Original mode 22
		// m[4:0],r0[7:0],b3[0],b2[4],g0[7:0],g2[5],g2[4],b0[7:0],g3[5],b3[4],
		// r1[4:0],g3[4],g2[3:0],g1[5:0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],
		// b3[2],r3[4:0],b3[3]
		r[0] = detexGetBits64(data0, 5, 12);
		b[3] = detexGetBits64(data0, 13, 13);
		b[2] = detexGetBits64(data0, 14, 14) << 4;
		g[0] = detexGetBits64(data0, 15, 22);
		g[2] = detexGetBits64(data0, 23, 23) << 5;
		g[2] |= detexGetBits64(data0, 24, 24) << 4;
		b[0] = detexGetBits64(data0, 25, 32);
		g[3] = detexGetBits64(data0, 33, 33) << 5;
		b[3] |= detexGetBits64(data0, 34, 34) << 4;
		r[1] = detexGetBits64(data0, 35, 39);
		g[3] |= detexGetBits64(data0, 40, 40) << 4;
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 59);
		b[3] |= detexGetBits64(data0, 60, 60) << 1;
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 5);
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 11);
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_b = 5;
		delta_bits_g = 6;
		break;
	case 8 :	// Original mode 26
		// m[4:0],r0[7:0],b3[1],b2[4],g0[7:0],b2[5],g2[4],b0[7:0],b3[5],b3[4],
		// r1[4:0],g3[4],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[5:0],b2[3:0],r2[4:0],
		// b3[2],r3[4:0],b3[3]
		r[0] = detexGetBits64(data0, 5, 12);
		b[3] = detexGetBits64(data0, 13, 13) << 1;
		b[2] = detexGetBits64(data0, 14, 14) << 4;
		g[0] = detexGetBits64(data0, 15, 22);
		b[2] |= detexGetBits64(data0, 23, 23) << 5;
		g[2] = detexGetBits64(data0, 24, 24) << 4;
		b[0] = detexGetBits64(data0, 25, 32);
		b[3] |= detexGetBits64(data0, 33, 33) << 5;
		b[3] |= detexGetBits64(data0, 34, 34) << 4;
		r[1] = detexGetBits64(data0, 35, 39);
		g[3] = detexGetBits64(data0, 40, 40) << 4;
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 49);
		b[3] |= detexGetBits64(data0, 50, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 60);
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 5);
		b[3] |= detexGetBits64(data1, 6, 6) << 2;
		r[3] = detexGetBits64(data1, 7, 11);
		b[3] |= detexGetBits64(data1, 12, 12) << 3;
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = 5;
		delta_bits_b = 6;
		break;
	case 9 :	// Original mode 30
		// m[4:0],r0[5:0],g3[4],b3[0],b3[1],b2[4],g0[5:0],g2[5],b2[5],b3[2],
		// g2[4],b0[5:0],g3[5],b3[3],b3[5],b3[4],r1[5:0],g2[3:0],g1[5:0],g3[3:0],
		// b1[5:0],b2[3:0],r2[5:0],r3[5:0]
		r[0] = detexGetBits64(data0, 5, 10);
		g[3] = detexGetBits64(data0, 11, 11) << 4;
		b[3] = detexGetBits64(data0, 12, 13);
		b[2] = detexGetBits64(data0, 14, 14) << 4;
		g[0] = detexGetBits64(data0, 15, 20);
		g[2] = detexGetBits64(data0, 21, 21) << 5;
		b[2] |= detexGetBits64(data0, 22, 22) << 5;
		b[3] |= detexGetBits64(data0, 23, 23) << 2;
		g[2] |= detexGetBits64(data0, 24, 24) << 4;
		b[0] = detexGetBits64(data0, 25, 30);
		g[3] |= detexGetBits64(data0, 31, 31) << 5;
		b[3] |= detexGetBits64(data0, 32, 32) << 3;
		b[3] |= detexGetBits64(data0, 33, 33) << 5;
		b[3] |= detexGetBits64(data0, 34, 34) << 4;
		r[1] = detexGetBits64(data0, 35, 40);
		g[2] |= detexGetBits64(data0, 41, 44);
		g[1] = detexGetBits64(data0, 45, 50);
		g[3] |= detexGetBits64(data0, 51, 54);
		b[1] = detexGetBits64(data0, 55, 60);
		b[2] |= detexGetBits64(data0, 61, 63);
		b[2] |= detexGetBits64(data1, 0, 0) << 3;
		r[2] = detexGetBits64(data1, 1, 6);
		r[3] = detexGetBits64(data1, 7, 12);
		partition_set_id = detexGetBits64(data1, 13, 17);
		block.index = 64 + 18;
//		delta_bits_r = delta_bits_g = delta_bits_b = 6;
		break;
	case 10 :	// Original mode 3
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[9:0],g1[9:0],b1[9:0]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 44);
		g[1] = detexGetBits64(data0, 45, 54);
		b[1] = detexGetBits64(data0, 55, 63);
		b[1] |= detexGetBits64(data1, 0, 0) << 9;
		partition_set_id = 0;
		block.index = 65;
//		delta_bits_r = delta_bits_g = delta_bits_b = 10;
		break;
	case 11 :	// Original mode 7
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[8:0],r0[10],g1[8:0],g0[10],b1[8:0],b0[10]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 43);
		r[0] |= detexGetBits64(data0, 44, 44) << 10;
		g[1] = detexGetBits64(data0, 45, 53);
		g[0] |= detexGetBits64(data0, 54, 54) << 10;
		b[1] = detexGetBits64(data0, 55, 63);
		b[0] |= detexGetBits64(data1, 0, 0) << 10;
		partition_set_id = 0;
		block.index = 65;
		delta_bits_r = delta_bits_g = delta_bits_b = 9;
		break;
	case 12 :	// Original mode 11
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[7:0],r0[10:11],g1[7:0],g0[10:11],
		// b1[7:0],b0[10:11]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 42);
		r[0] |= detexGetBits64Reversed(data0, 44, 43) << 10;	// Reversed.
		g[1] = detexGetBits64(data0, 45, 52);
		g[0] |= detexGetBits64Reversed(data0, 54, 53) << 10;	// Reversed.
		b[1] = detexGetBits64(data0, 55, 62);
		b[0] |= detexGetBits64(data0, 63, 63) << 11;	// MSB
		b[0] |= detexGetBits64(data1, 0, 0) << 10;	// LSB
		partition_set_id = 0;
		block.index = 65;
		delta_bits_r = delta_bits_g = delta_bits_b = 8;
		break;
	case 13 :	// Original mode 15
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10:15],g1[3:0],g0[10:15],
		// b1[3:0],b0[10:15]
		r[0] = detexGetBits64(data0, 5, 14);
		g[0] = detexGetBits64(data0, 15, 24);
		b[0] = detexGetBits64(data0, 25, 34);
		r[1] = detexGetBits64(data0, 35, 38);
		r[0] |= detexGetBits64Reversed(data0, 44, 39) << 10;	// Reversed.
		g[1] = detexGetBits64(data0, 45, 48);
		g[0] |= detexGetBits64Reversed(data0, 54, 49) << 10;	// Reversed.
		b[1] = detexGetBits64(data0, 55, 58);
		b[0] |= detexGetBits64Reversed(data0, 63, 59) << 11;	// Reversed.
		b[0] |= detexGetBits64(data1, 0, 0) << 10;
		partition_set_id = 0;
		block.index = 65;
		delta_bits_r = delta_bits_g = delta_bits_b = 4;
		break;
	}
	int nu_subsets;
	if (mode >= 10)
		nu_subsets = 1;
	else
		nu_subsets = 2;
	if (signed_flag) {
		r[0] = SignExtend(r[0], bptc_float_EPB[mode], 32);
		g[0] = SignExtend(g[0], bptc_float_EPB[mode], 32);
		b[0] = SignExtend(b[0], bptc_float_EPB[mode], 32);
	}
	if (mode != 9 && mode != 10) {
		// Transformed endpoints.
		for (int i = 1; i < nu_subsets * 2; i++) {
			r[i] = SignExtend(r[i], delta_bits_r, 32);
			r[i] = (r[0] + r[i]) & (((uint32_t)1 << bptc_float_EPB[mode]) - 1);
			g[i] = SignExtend(g[i], delta_bits_g, 32);
			g[i] = (g[0] + g[i]) & (((uint32_t)1 << bptc_float_EPB[mode]) - 1);
			b[i] = SignExtend(b[i], delta_bits_b, 32);
			b[i] = (b[0] + b[i]) & (((uint32_t)1 << bptc_float_EPB[mode]) - 1);
			if (signed_flag) {
				r[i] = SignExtend(r[i], bptc_float_EPB[mode], 32);
				g[i] = SignExtend(g[i], bptc_float_EPB[mode], 32);
				b[i] = SignExtend(b[i], bptc_float_EPB[mode], 32);
			}
		}
	}
	else	// Mode 9 or 10, no transformed endpoints.
	if (signed_flag)
		for (int i = 1; i < nu_subsets * 2; i++) {
			r[i] = SignExtend(r[i], bptc_float_EPB[mode], 32);
			g[i] = SignExtend(g[i], bptc_float_EPB[mode], 32);
			b[i] = SignExtend(b[i], bptc_float_EPB[mode], 32);
		}

	// Unquantize endpoints.
	if (signed_flag)
		for (int i = 0; i < 2 * nu_subsets; i++) {
			r[i] = UnquantizeSigned(r[i], mode);
			g[i] = UnquantizeSigned(g[i], mode);
			b[i] = UnquantizeSigned(b[i], mode);
		}
	else
		for (int i = 0; i < 2 * nu_subsets; i++) {
			r[i] = Unquantize(r[i], mode);
			g[i] = Unquantize(g[i], mode);
			b[i] = Unquantize(b[i], mode);
		}

	uint8_t subset_index[16];
	for (int i = 0; i < 16; i++) {
		// subset_index[i] is a number from 0 to 1, depending on the number of subsets.
		subset_index[i] = GetPartitionIndex(nu_subsets, partition_set_id, i);
	}
	uint8_t anchor_index[4];	// Only need max. 2 elements
	for (int i = 0; i < nu_subsets; i++)
		anchor_index[i] = GetAnchorIndex(partition_set_id, i, nu_subsets);
	uint8_t color_index[16];
	// Extract index bits.
	int color_index_bit_count = 3;
	if ((bitstring[0] & 3) == 3)	// This defines original modes 3, 7, 11, 15
		color_index_bit_count = 4;
	// Because the index bits are all in the second 64-bit word, there is no need to use
	// block_extract_bits().
	data1 >>= (block.index - 64);
	uint8_t mask1 = (1 << color_index_bit_count) - 1;
	uint8_t mask2 = (1 << (color_index_bit_count - 1)) - 1;
	for (int i = 0; i < 16; i++) {
		if (i == anchor_index[subset_index[i]]) {
			// Highest bit is zero.
//			color_index[i] = block_extract_bits(&block, color_index_bit_count - 1);
			color_index[i] = data1 & mask2;
			data1 >>= color_index_bit_count - 1;
		}
		else {
//			color_index[i] = block_extract_bits(&block, color_index_bit_count);
			color_index[i] = data1 & mask1;	
			data1 >>= color_index_bit_count;
		}
	}

	for (int i = 0; i < 16; i++) {
		int32_t endpoint_start_r, endpoint_start_g, endpoint_start_b;
		int32_t endpoint_end_r, endpoint_end_g, endpoint_end_b;
		endpoint_start_r = r[2 * subset_index[i]];
		endpoint_end_r = r[2 * subset_index[i] + 1];
		endpoint_start_g = g[2 * subset_index[i]];
		endpoint_end_g = g[2 * subset_index[i] + 1];
		endpoint_start_b = b[2 * subset_index[i]];
		endpoint_end_b = b[2 * subset_index[i] + 1];
		uint64_t output;
		if (signed_flag) {
			int32_t r16 = InterpolateFloat(endpoint_start_r, endpoint_end_r, color_index[i],
				color_index_bit_count);
			if (r16 < 0)
				r16 = - (((- r16) * 31) >> 5);
			else
				r16 = (r16 * 31) >> 5;
			int s = 0;
			if (r16 < 0) {
				s = 0x8000;
				r16 = - r16;
			}
			r16 |= s;
			int32_t g16 = InterpolateFloat(endpoint_start_g, endpoint_end_g, color_index[i],
				color_index_bit_count);
			if (g16 < 0)
				g16 = - (((- g16) * 31) >> 5);
			else
				g16 = (g16 * 31) >> 5;
			s = 0;
			if (g16 < 0) {
				s = 0x8000;
				g16 = - g16;
			}
			g16 |= s;
			int32_t b16 = InterpolateFloat(endpoint_start_b, endpoint_end_b, color_index[i],
				color_index_bit_count);
			if (b16 < 0)
				b16 = - (((- b16) * 31) >> 5);
			else
				b16 = (b16 * 31) >> 5;
			s = 0;
			if (b16 < 0) {
				s = 0x8000;
				b16 = - b16;
			}
			b16 |= s;
			output = detexPack64RGB16(r16, g16, b16);
		}
		else {
			output = detexPack64R16(InterpolateFloat(endpoint_start_r, endpoint_end_r, color_index[i],
				color_index_bit_count) * 31 / 64);
			output |= detexPack64G16(InterpolateFloat(endpoint_start_g, endpoint_end_g, color_index[i],
				color_index_bit_count) * 31 / 64);
			output |= detexPack64B16(InterpolateFloat(endpoint_start_b, endpoint_end_b, color_index[i],
				color_index_bit_count) * 31 / 64);
		}
		*(uint64_t *)&pixel_buffer[i * 8] = output;
	}
	return true;
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* BPTC_FLOAT (BC6H) format. The output format is */
/* DETEX_PIXEL_FORMAT_FLOAT_RGBX16. */
bool detexDecompressBlockBPTC_FLOAT(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	return DecompressBlockBPTCFloatShared(bitstring, mode_mask, flags, false,
		pixel_buffer);
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* BPTC_FLOAT (BC6H_FLOAT) format. The output format is */
/* DETEX_PIXEL_FORMAT_SIGNED_FLOAT_RGBX16. */
bool detexDecompressBlockBPTC_SIGNED_FLOAT(const uint8_t * DETEX_RESTRICT bitstring,
uint32_t mode_mask, uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	return DecompressBlockBPTCFloatShared(bitstring, mode_mask, flags, true,
		pixel_buffer);
}

/* Return the internal mode of the BPTC_FLOAT block. */
uint32_t detexGetModeBPTC_FLOAT(const uint8_t *bitstring) {
	detexBlock128 block;
	block.data0 = *(uint64_t *)&bitstring[0];
	block.data1 = *(uint64_t *)&bitstring[8];
	block.index = 0;
	int mode = ExtractMode(&block);
	return mode;
}

uint32_t detexGetModeBPTC_SIGNED_FLOAT(const uint8_t *bitstring) {
	return detexGetModeBPTC_FLOAT(bitstring);
}

static const uint8_t bptc_float_set_mode_table[14] = {
	0, 1, 2, 6, 10, 14, 18, 22, 26, 30, 3, 7, 11, 15
};

void detexSetModeBPTC_FLOAT(uint8_t *bitstring, uint32_t mode, uint32_t flags,
uint32_t *colors) {
	if (mode <= 1) {
		// Set mode 0 or 1.
		bitstring[0] = (bitstring[0] & 0xFC) | mode;
		return;
	}
	uint8_t byte0 = bitstring[0];
	byte0 &= 0xE0;
	byte0 |= bptc_float_set_mode_table[mode];
	bitstring[0] = byte0;
}

