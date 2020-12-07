/******************************************************************************
* Universal Analytics for C 
*  -- URL encoding module for UTF-8 compatibility with Google Analytics
* Copyright (c) 2013, Analytics Pros
* 
* This project is free software, distributed under the BSD license. 
* Analytics Pros offers consulting and integration services if your firm needs 
* assistance in strategy, implementation, or auditing existing work.
******************************************************************************/


#ifndef UA_ENCODE_H
#define UA_ENCODE_H

#include <string.h>

typedef unsigned int __uint;

char* urlencode(const char* input);
size_t urlencode_put(char* result, size_t result_max, const char *mb_input, size_t input_len);

// For compatibility..
unsigned int encodeURIComponent(char input[], char output[], const unsigned int input_len, const unsigned int output_max);

unsigned int hexdigest(char* hex_output, unsigned char* md5_binary, unsigned int binary_len);


#endif /* UA_ENCODE_H  */