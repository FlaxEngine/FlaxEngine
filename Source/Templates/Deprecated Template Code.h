//template code for Deprecated code at some version
//-----------------------------------------------------------------------------------
//                         find and replace, legend / info
//-----------------------------------------------------------------------------------
// <depMAJOR> major version (0-...)
// <depMINOR> minor version (0-9)
//-----------------------------------------------------------------------------------
//                                 requerd inclide
//-----------------------------------------------------------------------------------
// FlaxEngine.Gen.h
//-----------------------------------------------------------------------------------


#pragma region Deprecated
#if (FLAXENGINE_VERSION_MAJOR != <depMAJOR> && (FLAXENGINE_VERSION_MINOR >= <depMINOR>))
//<Deprecated code here>

#else
#ifndef STRING2
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#endif
#pragma message ( __FILE__ "(" STRING(__LINE__) "):" "[Code Mantening] Remove Deprecated code")
#endif
#pragma endregion