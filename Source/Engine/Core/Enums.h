// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Core.h"
#include "Types/BaseTypes.h"

// Macros for enums declaration with in-build ToString conversion

#define ENUM_TO_STR(x) TEXT(MACRO_TO_STR(x))
#define ENUM_TO_STR_FALLBACK TEXT("")

#define DECLARE_ENUM_1(name, t0) enum class name{t0=0};\
static const int name##_Count = 1;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_2(name, t0, t1) enum class name{t0=0,t1};\
static const int name##_Count = 2;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_3(name, t0, t1, t2) enum class name{t0=0,t1,t2};\
static const int name##_Count = 3;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_4(name, t0, t1, t2, t3) enum class name{t0=0,t1,t2,t3};\
static const int name##_Count = 4;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_5(name, t0, t1, t2, t3, t4) enum class name{t0=0,t1,t2,t3,t4};\
static const int name##_Count = 5;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_6(name, t0, t1, t2, t3, t4, t5) enum class name{t0=0,t1,t2,t3,t4,t5};\
static const int name##_Count = 6;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_7(name, t0, t1, t2, t3, t4, t5, t6) enum class name{t0=0,t1,t2,t3,t4,t5,t6};\
static const int name##_Count = 7;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_8(name, t0, t1, t2, t3, t4, t5, t6, t7) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7};\
static const int name##_Count = 8;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_9(name, t0, t1, t2, t3, t4, t5, t6, t7, t8) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8};\
static const int name##_Count = 9;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_10(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9};\
static const int name##_Count = 10;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_11(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10};\
static const int name##_Count = 11;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_12(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11};\
static const int name##_Count = 12;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_13(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12};\
static const int name##_Count = 13;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11),ENUM_TO_STR(t12)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_14(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13};\
static const int name##_Count = 14;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11),ENUM_TO_STR(t12),ENUM_TO_STR(t13)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_15(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14};\
static const int name##_Count = 15;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11),ENUM_TO_STR(t12),ENUM_TO_STR(t13),ENUM_TO_STR(t14)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_16(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15};\
static const int name##_Count = 16;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11),ENUM_TO_STR(t12),ENUM_TO_STR(t13),ENUM_TO_STR(t14),ENUM_TO_STR(t15)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_17(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16};\
static const int name##_Count = 17;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11),ENUM_TO_STR(t12),ENUM_TO_STR(t13),ENUM_TO_STR(t14),ENUM_TO_STR(t15),ENUM_TO_STR(t16)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

#define DECLARE_ENUM_18(name, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17) enum class name{t0=0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17};\
static const int name##_Count = 17;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11),ENUM_TO_STR(t12),ENUM_TO_STR(t13),ENUM_TO_STR(t14),ENUM_TO_STR(t15),ENUM_TO_STR(t16),ENUM_TO_STR(t17)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e];}

// ..and those with base type and base value

#define DECLARE_ENUM_EX_1(name, baseType, baseVal, t0) enum class name:baseType{t0=baseVal};\
static const int name##_Count = 1;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_2(name, baseType, baseVal, t0, t1) enum class name:baseType{t0=baseVal,t1};\
static const int name##_Count = 2;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_3(name, baseType, baseVal, t0, t1, t2) enum class name:baseType{t0=baseVal,t1,t2};\
static const int name##_Count = 3;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_4(name, baseType, baseVal, t0, t1, t2, t3) enum class name:baseType{t0=baseVal,t1,t2,t3};\
static const int name##_Count = 4;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_5(name, baseType, baseVal, t0, t1, t2, t3, t4) enum class name:baseType{t0=baseVal,t1,t2,t3,t4};\
static const int name##_Count = 5;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_6(name, baseType, baseVal, t0, t1, t2, t3, t4, t5) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5};\
static const int name##_Count = 6;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_7(name, baseType, baseVal, t0, t1, t2, t3, t4, t5, t6) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5,t6};\
static const int name##_Count = 7;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_8(name, baseType, baseVal, t0, t1, t2, t3, t4, t5, t6, t7) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5,t6,t7};\
static const int name##_Count = 8;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_9(name, baseType, baseVal, t0, t1, t2, t3, t4, t5, t6, t7, t8) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5,t6,t7,t8};\
static const int name##_Count = 9;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_10(name, baseType, baseVal, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5,t6,t7,t8,t9};\
static const int name##_Count = 10;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_11(name, baseType, baseVal, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10};\
static const int name##_Count = 11;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

#define DECLARE_ENUM_EX_12(name, baseType, baseVal, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11) enum class name:baseType{t0=baseVal,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11};\
static const int name##_Count = 11;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9),ENUM_TO_STR(t10),ENUM_TO_STR(t11)};\
return str;}static inline const Char*ToString(name e){return name##_Str()[(int)e - baseVal];}

// ..and those for flags

#define DECLARE_ENUM_FLAGS_1(name, baseType, t0, v0) \
enum class name:baseType{t0=v0};\
static const int name##_Count = 1;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_2(name, baseType, t0, v0, t1, v1) \
enum class name:baseType{t0=v0,t1=v1};\
static const int name##_Count = 2;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_3(name, baseType, t0, v0, t1, v1, t2, v2) \
enum class name:baseType{t0=v0,t1=v1,t2=v2};\
static const int name##_Count = 3;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_4(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3};\
static const int name##_Count = 4;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_5(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3,t4=v4};\
static const int name##_Count = 5;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3), (baseType)(v4)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_6(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3,t4=v4,t5=v5};\
static const int name##_Count = 6;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3), (baseType)(v4), (baseType)(v5)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_7(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3,t4=v4,t5=v5,t6=v6};\
static const int name##_Count = 7;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3), (baseType)(v4), (baseType)(v5), (baseType)(v6)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_8(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3,t4=v4,t5=v5,t6=v6,t7=v7};\
static const int name##_Count = 8;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3), (baseType)(v4), (baseType)(v5), (baseType)(v6), (baseType)(v7)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_9(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3,t4=v4,t5=v5,t6=v6,t7=v7,t8=v8};\
static const int name##_Count = 9;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3), (baseType)(v4), (baseType)(v5), (baseType)(v6), (baseType)(v7), (baseType)(v8)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }

#define DECLARE_ENUM_FLAGS_10(name, baseType, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9) \
enum class name:baseType{t0=v0,t1=v1,t2=v2,t3=v3,t4=v4,t5=v5,t6=v6,t7=v7,t8=v8,t9=v9};\
static const int name##_Count = 10;static const Char**name##_Str(){static const Char* str[]={\
ENUM_TO_STR(t0),ENUM_TO_STR(t1),ENUM_TO_STR(t2),ENUM_TO_STR(t3),ENUM_TO_STR(t4),ENUM_TO_STR(t5),ENUM_TO_STR(t6),ENUM_TO_STR(t7),ENUM_TO_STR(t8),ENUM_TO_STR(t9)};\
return str;}static inline const Char*ToString(name e){static baseType val[]={\
(baseType)(v0), (baseType)(v1), (baseType)(v2), (baseType)(v3), (baseType)(v4), (baseType)(v5), (baseType)(v6), (baseType)(v7), (baseType)(v8), (baseType)(v9)}; \
for (baseType i = 0; i < name##_Count; i++) { if (val[i] == (baseType)e) return name##_Str()[(int)i]; } return ENUM_TO_STR_FALLBACK; }
