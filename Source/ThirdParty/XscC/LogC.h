/*
 * LogC.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_LOG_C_H
#define XSC_LOG_C_H


#include "ReportC.h"


#ifdef __cplusplus
extern "C" {
#endif



/**
\brief Function callback interface for handling reports.
\param[in] report Specifies the compiler report.
\param[in] indent Specifies the current indentation string.
*/
typedef void (*XSC_PFN_HANDLE_REPORT)(const struct XscReport* report, const char* indent);


//! Output log structure.
struct XscLog
{
    //! Function pointer to handle compiler reports.
    XSC_PFN_HANDLE_REPORT handleReportPfn;
};


//! Default log.
#define XSC_DEFAULT_LOG ((const struct XscLog*)(1))


#ifdef __cplusplus
} // /extern "C"
#endif


#endif



// ================================================================================