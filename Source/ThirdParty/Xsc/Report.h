/*
 * Report.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_REPORT_H
#define XSC_REPORT_H


#include "Export.h"
#include <stdexcept>
#include <string>
#include <vector>


namespace Xsc
{


//! Report types enumeration.
enum class ReportTypes
{
    Info,       //!< Standard information.
    Warning,    //!< Warning message.
    Error       //!< Error message.
};

//! Report exception class which contains a completely constructed message with optional line marker, hints, and context description.
class XSC_EXPORT Report : public std::exception
{
    
    public:
        
        Report(const Report&) = default;
        Report& operator = (const Report&) = default;

        Report(const ReportTypes type, const std::string& message, const std::string& context = "");
        Report(const ReportTypes type, const std::string& message, const std::string& line, const std::string& marker, const std::string& context = "");

        //! Overrides the 'std::exception::what' function.
        const char* what() const throw() override;

        //! Moves the specified hints into this report.
        void TakeHints(std::vector<std::string>&& hints);

        //! Returns the type of this report.
        inline ReportTypes Type() const
        {
            return type_;
        }

        //! Returns the context description string (e.g. a function name where the report occured). This may also be empty.
        inline const std::string& Context() const
        {
            return context_;
        }

        //! Returns the message string.
        inline const std::string& Message() const
        {
            return message_;
        }

        //! Returns the line string where the report occured. This line never has new-line characters at its end.
        inline const std::string& Line() const
        {
            return line_;
        }

        //! Returns the line marker string to highlight the area where the report occured.
        inline const std::string& Marker() const
        {
            return marker_;
        }

        //! Returns the list of optional hints of the report.
        inline const std::vector<std::string>& GetHints() const
        {
            return hints_;
        }

        /**
        \brief Returns true if this report has a line with line marker.
        \see Line
        \see Marker
        */
        inline bool HasLine() const
        {
            return (!line_.empty());
        }

    private:

        ReportTypes                 type_       = ReportTypes::Info;
        std::string                 context_;
        std::string                 message_;
        std::string                 line_;
        std::string                 marker_;
        std::vector<std::string>    hints_;

};


} // /namespace Xsc


#endif



// ================================================================================