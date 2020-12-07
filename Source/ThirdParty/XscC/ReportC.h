/*
 * ReportC.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_REPORT_C_H
#define XSC_REPORT_C_H


#ifdef __cplusplus
extern "C" {
#endif


//! Report types enumeration.
enum XscReportType
{
    XscEReportInfo,     //!< Standard information.
    XscEReportWarning,  //!< Warning message.
    XscEReportError,    //!< Error message.
};

//! Report structure for warning and error messages.
struct XscReport
{
    //! Specifies the report type.
    enum XscReportType  type;

    //! Specifies the context description string (e.g. a function name where the report occured). This may also be NULL.
    const char*         context;

    //! Specifies the message string.
    const char*         message;

    //! Specifies the line string where the report occured. This line never has new-line characters at its end. This may also be NULL.
    const char*         line;

    //! Specifies the line marker string to highlight the area where the report occured. This may also be NULL.
    const char*         marker;

    //! Specifies the list of optional hints of the report. This may also be NULL.
    const char**        hints;

    //! Specifies the number of hints. If 'hints' is NULL, this is 0.
    size_t              hintsCount;
};


#ifdef __cplusplus
} // /extern "C"
#endif


#endif



// ================================================================================