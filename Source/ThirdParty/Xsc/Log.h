/*
 * Log.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_LOG_H
#define XSC_LOG_H


#include "IndentHandler.h"
#include "Report.h"
#include <vector>
#include <string>


namespace Xsc
{


/* ===== Public classes ===== */

//! Log base class.
class XSC_EXPORT Log
{
    
    public:
        
        //! Submits the specified report.
        virtual void SubmitReport(const Report& report) = 0;

        //! Sets the next indentation string. By default two spaces.
        inline void SetIndent(const std::string& indent)
        {
            indentHandler_.SetIndent(indent);
        }

        //! Increments the indentation.
        inline void IncIndent()
        {
            indentHandler_.IncIndent();
        }

        //! Decrements the indentation.
        inline void DecIndent()
        {
            indentHandler_.DecIndent();
        }

    protected:

        Log() = default;

        //! Returns the current full indentation string.
        inline const std::string& FullIndent() const
        {
            return indentHandler_.FullIndent();
        }

    private:

        IndentHandler indentHandler_;

};

//! Standard output log (uses std::cout to submit a report).
class XSC_EXPORT StdLog : public Log
{

    public:

        //! Implements the base class interface.
        void SubmitReport(const Report& report) override;

        //! Prints all submitted reports to the standard output.
        void PrintAll(bool verbose = true);

    private:

        struct IndentReport
        {
            std::string indent;
            Report      report;
        };

        using IndentReportList = std::vector<IndentReport>;

        void PrintReport(const IndentReport& r, bool verbose);
        void PrintAndClearReports(IndentReportList& reports, bool verbose, const std::string& headline = "");

        IndentReportList infos_;
        IndentReportList warnings_;
        IndentReportList errors_;

};


} // /namespace Xsc


#endif



// ================================================================================