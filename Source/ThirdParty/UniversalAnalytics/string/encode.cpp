/******************************************************************************
* Universal Analytics for C 
*  -- URL encoding module for UTF-8 compatibility with Google Analytics
* Copyright (c) 2013, Analytics Pros
* 
* This project is free software, distributed under the BSD license. 
* Analytics Pros offers consulting and integration services if your firm needs 
* assistance in strategy, implementation, or auditing existing work.
******************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <wctype.h>

#include "encode.h"

/* The following constants are symbolized for readability in later encoding phases */
#define ASCII_HIGH 0x7F
#define UTF8_INVALID 0xFFFF
#define UTF8_SWAPCHAR 0xFFFE
#define UTF8_HIGH_TWO_BYTES 0x7FF
#define UTF8_HIGH_THREE_BYTES 0xFFFD
#define UTF8_HIGH_FOUR_BYTES 0x1FFFFF
#define UTF8_HIGH_FIVE_BYTES 0x3FFFFFF
#define UTF8_HIGH_SIX_BYTES 0x7FFFFFFF


#define _minimum(a, b) ((a < b) ? a : b)


/* Mapping for hexadecimal conversion */
static const char _hexchar[] = "0123456789abcdef";


// Write a hexidecimal value (32 bit) to a character buffer
unsigned int hexadecimal(char* output, unsigned int value){

	assert(NULL != output);

	__uint a = (value >> 28) & 0xF;
	__uint b = (value >> 24) & 0xF;
	__uint c = (value >> 20) & 0xF;
	__uint d = (value >> 16) & 0xF;
	__uint e = (value >> 12) & 0xF;
	__uint f = (value >> 8) & 0xF;
	__uint g = (value >> 4) & 0xF;
	__uint h = (value & 0xF);

	__uint i = 0;

	if(a) output[i++] = _hexchar[ (int) a ];
	if(b || a) output[i++] = _hexchar[ (int) b ];
	if(c || b || a) output[i++] = _hexchar[ (int) c ];
	if(d || c || b || a) output[i++] = _hexchar[ (int) d ];
	if(e || d || c || b || a) output[i++] = _hexchar[ (int) e ];
	if(f || e || d || c || b || a) output[i++] = _hexchar[ (int) f ];
	if(g || f || e || d || c || b || a) output[i++] = _hexchar[ (int) g ];
	output[i++] = _hexchar[ (int) h ];

	return i;

}

// Primarily intended to aid the translation of binary MD5 digests
unsigned int hexdigest(char* hex_output, unsigned char* binary, unsigned int binary_len){
	unsigned int i;
	unsigned int o = 0;
	for(i = 0; i < binary_len; i++){
		o += hexadecimal(hex_output + o, (unsigned int) binary[i]);
	}	
	return o;
}





static int isASCIIcompat_char(char char_val){
  return (char_val == 0x09 || char_val == 0x0A || char_val == 0x0D || (0x20 <= char_val && char_val <= 0x7F));
}



size_t urlencode_put(char* result, size_t result_max, const char *multibyte_input, size_t input_len){

	assert(NULL != result);
	assert(NULL != multibyte_input);

	unsigned int i = 0;
	unsigned int r = 0;
	unsigned int offset = 0;
	wchar_t current;

	mbtowc(NULL, NULL, 0); // Reset multibyte state

	do {
		// Convert the current multi-byte character into a wide representation (i.e. unsigned long int)
		offset += mbtowc(& current, (multibyte_input) + (offset), MB_CUR_MAX);

		if(current == 0){
			break; // Stop on NULL termination
		}

		// Spaces are encoded as "plus" (+)
		else if(current == ' ' && r < result_max){ 
			result[r++] = '+';
			continue;
		} 

		// These characters are allowed as literals
		else if((iswalnum(current) || current == '-' || current == '.' || current == '~') && r < result_max){ 
			result[r++] = (char)current;
		}

		// Standard ASCII characters are encoded simply
		else if(isASCIIcompat_char((char)current) && r < result_max){
			result[r++] = '%';
			r += hexadecimal(result + r, (unsigned int) (current & 0xFF));
		}

		// The method |mbtowc| (above) takes care of splitting UTF8 bytes,
		// so this can run directly...
		else if(current >= ASCII_HIGH && current <= UTF8_HIGH_SIX_BYTES && (r +2) < result_max){
			result[r++] = '%';
			r+= hexadecimal(result + r, (unsigned int) current);
		}


		// This would seem to be an encoding error.
		// Considering fall-back to "hexdigest" representation.
		else if(result_max > r) {
			result[r++] = '*';
			break;
		}

	} while((i++) < input_len && (r < result_max));

	return r;

}

size_t urlencode_put_limit(const char* mb_input, char* output, size_t output_limit){
	assert(NULL != mb_input);
	assert(NULL != output);

	size_t input_len = mbstowcs(NULL, mb_input, 0) +1;
	memset(output, 0, output_limit);

	return urlencode_put(output, output_limit, mb_input, input_len);
}

/* Create a new character buffer, and write the given input as URL-encoded (UTF-8) */
char* urlencode(const char* mb_input){
	assert(NULL != mb_input);

	size_t input_len = mbstowcs(NULL, mb_input, 0) +1;
	
	// Prepare the output buffer; in some cases each character input could result 
	// in 12 characters encoded output ['%' + XX (hex), for up to 4 bytes]
	unsigned long int output_allocation = sizeof(char) * ((unsigned long int)input_len * 12);
	char* output = (char*)malloc(output_allocation);
	memset(output, 0, output_allocation);

	urlencode_put(output, output_allocation, mb_input, input_len);
	return output;
}


/* For compatibility with our former encoding model... */
unsigned int encodeURIComponent(const char input[], char output[], const unsigned int input_len, const unsigned int output_max){
	return (unsigned int)urlencode_put(output, output_max, input, input_len);
}




