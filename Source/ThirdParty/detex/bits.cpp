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

uint32_t detexBlock128ExtractBits(detexBlock128 *block, int nu_bits) {
	uint32_t value = 0;
	for (int i = 0; i < nu_bits; i++) {
		if (block->index < 64) {
			int shift = block->index - i;
			if (shift < 0)
				value |= (block->data0 & ((uint64_t)1 << block->index)) << (- shift);
			else
				value |= (block->data0 & ((uint64_t)1 << block->index)) >> shift;
		}
		else {
			int shift = ((block->index - 64) - i);
			if (shift < 0)
				value |= (block->data1 & ((uint64_t)1 << (block->index - 64))) << (- shift);
			else
				value |= (block->data1 & ((uint64_t)1 << (block->index - 64))) >> shift;
		}
		block->index++;
	}
//	if (block->index > 128)
//		printf("Block overflow (%d)\n", block->index);
	return value;
}

