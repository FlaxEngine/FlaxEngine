/*
 * ConsoleManip.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_CONSOLE_MANIP_H
#define XSC_CONSOLE_MANIP_H


#include "Export.h"
#include <ostream>
#include <iostream>


namespace Xsc
{

//! Namespace for console manipulation
namespace ConsoleManip
{


/* ===== Public flags ===== */

//! Output stream color flags enumeration.
struct ColorFlags
{
    enum
    {
        Red     = (1 << 0),                 //!< Red color flag.
        Green   = (1 << 1),                 //!< Green color flag.
        Blue    = (1 << 2),                 //!< Blue color flag.

        Intens  = (1 << 3),                 //!< Intensity color flag.

        Black   = 0,                        //!< Black color flag.
        Gray    = (Red | Green | Blue),     //!< Gray color flag (Red | Green | Blue).
        White   = (Gray | Intens),          //!< White color flag (Gray | Intens).

        Yellow  = (Red | Green | Intens),   //!< Yellow color flag (Red | Green | Intens).
        Pink    = (Red | Blue | Intens),    //!< Pink color flag (Red | Blue | Intens).
        Cyan    = (Green | Blue | Intens),  //!< Cyan color flag (Green | Blue | Intens).
    };
};


/* ===== Public functions ===== */

//! Enables or disables console manipulation. By default enabled.
void XSC_EXPORT Enable(bool enable);

//! Returns true if console manipulation is enabled.
bool XSC_EXPORT IsEnabled();

/**
\brief Pushes the specified front color flags onto the stack.
\param[in] front Specifies the flags for the front color.
This can be a bitwise OR combination of the flags declared in 'ColorFlags'.
\param[in,out] stream Specifies the output stream whose front color is to be changed.
This output stream is only required for Linux and MacOS, since the colors are specified by the streams itself.
\see ColorFlags
*/
void XSC_EXPORT PushColor(long front, std::ostream& stream = std::cout);

/**
\brief Pushes the specified front and back color flags onto the stack.
\param[in] front Specifies the flags for the front color.
This can be a bitwise OR combination of the flags declared in 'ColorFlags'.
\param[in] back Specifies the flags for the background color.
This can be a bitwise OR combination of the flags declared in 'ColorFlags'.
\param[in,out] stream Specifies the output stream whose front color is to be changed.
This output stream is only required for Linux and MacOS, since the colors are specified by the streams itself.
\see ColorFlags
*/
void XSC_EXPORT PushColor(long front, long back, std::ostream& stream = std::cout);

//! Pops the previous front and back color flags from the stack.
void XSC_EXPORT PopColor(std::ostream& stream = std::cout);


/* ===== Public classes ===== */

//! Helper class for scoped color stack operations.
class ScopedColor
{
    
    public:
    
        /**
        \brief Constructor with output stream and front color flags.
        \param[in,out] stream Specifies the output stream for which the scope is to be changed. This is only used for Unix systems.
        \param[in] front Specifies the front color flags. This can be a bitwise OR combination of the entries of the ColorFlags enumeration.
        \see ColorFlags
        \see PushColor(long, std::ostream&)
        */
        inline ScopedColor(long front, std::ostream& stream = std::cout) :
            stream_ { stream }
        {
            PushColor(front, stream_);
        }

        /**
        \brief Constructor with output stream, and front- and back color flags.
        \param[in,out] stream Specifies the output stream for which the scope is to be changed. This is only used for Unix systems.
        \param[in] front Specifies the front color flags. This can be a bitwise OR combination of the entries of the ColorFlags enumeration.
        \param[in] back Specifies the back color flags. This can be a bitwise OR combination of the entries of the ColorFlags enumeration.
        \see ColorFlags
        \see PushColor(std::ostream&, long, long)
        */
        inline ScopedColor(long front, long back, std::ostream& stream = std::cout) :
            stream_ { stream }
        {
            PushColor(front, back, stream_);
        }

        /**
        \brief Destructor which will reset the previous color from the output stream.
        \see PopColor
        */
        inline ~ScopedColor()
        {
            PopColor(stream_);
        }
        
    private:
        
        std::ostream& stream_;
        
};


} // /namespace ConsoleManip

} // /namespace Xsc


#endif



// ================================================================================