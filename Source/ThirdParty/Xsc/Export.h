/*
 * Export.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_EXPORT_H
#define XSC_EXPORT_H

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(_MSC_VER) && defined(XSC_SHARED_LIB)
#   define XSC_EXPORT __declspec(dllexport)
#else
#   define XSC_EXPORT
#endif

#define XSC_THREAD_LOCAL thread_local

#endif



// ================================================================================