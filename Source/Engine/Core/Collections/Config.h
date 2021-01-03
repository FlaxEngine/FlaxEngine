// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

/// <summary>
/// Default capacity for the dictionaries (amount of space for the elements)
/// </summary>
#define DICTIONARY_DEFAULT_CAPACITY 256

/// <summary>
/// Function for dictionary that tells how change hash index during iteration (size param is a buckets table size)
/// </summary>
#define DICTIONARY_PROB_FUNC(size, numChecks) (numChecks)
//#define DICTIONARY_PROB_FUNC(size, numChecks) (1)
