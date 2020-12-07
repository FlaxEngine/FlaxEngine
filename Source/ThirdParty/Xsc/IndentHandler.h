/*
 * IndentHandler.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_INDENT_HANDLER_H
#define XSC_INDENT_HANDLER_H


#include "Export.h"
#include <string>
#include <stack>


namespace Xsc
{


/* ===== Public classes ===== */

//! Indentation handler base class.
class XSC_EXPORT IndentHandler
{
    
    public:
        
        IndentHandler(const std::string& initialIndent = std::string(2, ' '));

        //! Sets the next indentation string. By default two spaces.
        void SetIndent(const std::string& indent);

        //! Increments the indentation.
        void IncIndent();

        //! Decrements the indentation.
        void DecIndent();

        //! Returns the current full indentation string.
        inline const std::string& FullIndent() const
        {
            return indentFull_;
        }

    private:

        std::string                         indent_;
        std::string                         indentFull_;
        std::stack<std::string::size_type>  indentStack_;

};

//! Helper class for temporary indentation.
class ScopedIndent
{

    public:

        inline ScopedIndent(IndentHandler& handler) :
            handler_ { handler }
        {
            handler_.IncIndent();
        }

        inline ~ScopedIndent()
        {
            handler_.DecIndent();
        }

    private:

        IndentHandler& handler_;

};


} // /namespace Xsc


#endif



// ================================================================================