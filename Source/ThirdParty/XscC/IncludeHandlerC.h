/*
 * IncludeHandlerC.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_INCLUDE_HANDLER_C_H
#define XSC_INCLUDE_HANDLER_C_H


#ifdef __cplusplus
extern "C" {
#endif


/**
\brief Function callback interface for handling '#include'-directives.
\param[in] filename Specifies the include filename.
\param[in] searchPaths Specifies an array of include paths. The last entry in this array is NULL.
\param[in] useSearchPathsFirst Specifies whether search paths are to be used first, to find the include file.
\return Pointer to the source code of the included file, or NULL to ignore this include directive.
*/
typedef const char* (*XSC_PFN_HANDLE_INCLUDE)(const char* filename, const char** searchPaths, bool useSearchPathsFirst);


//! Include handler structure.
struct XscIncludeHandler
{
    //! Function pointer to handle the '#include'-directives.
    XSC_PFN_HANDLE_INCLUDE  handleIncludePfn;

    //! Pointer to an array of search paths. This must be either NULL, point to an array where the last entry is always NULL.
    const char**            searchPaths;
};


#ifdef __cplusplus
} // /extern "C"
#endif


#endif



// ================================================================================