// Copyright (c) Wojciech Figat. All rights reserved.

#include "ManagedDictionary.h"

#if USE_CSHARP
Dictionary<ManagedDictionary::KeyValueType, MTypeObject*> ManagedDictionary::CachedDictionaryTypes;
#if !USE_MONO_AOT
ManagedDictionary::MakeGenericTypeThunk ManagedDictionary::MakeGenericType;
ManagedDictionary::CreateInstanceThunk ManagedDictionary::CreateInstance;
ManagedDictionary::AddDictionaryItemThunk ManagedDictionary::AddDictionaryItem;
ManagedDictionary::GetDictionaryKeysThunk ManagedDictionary::GetDictionaryKeys;
#else
MMethod* ManagedDictionary::MakeGenericType;
MMethod* ManagedDictionary::CreateInstance;
MMethod* ManagedDictionary::AddDictionaryItem;
MMethod* ManagedDictionary::GetDictionaryKeys;
#endif
#endif
