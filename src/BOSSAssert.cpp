/* -*- c-basic-offset: 4 -*- */

#include "BOSSAssert.h"
#include "BOSSException.h"
#include <iomanip>
#include <ctime>

using namespace BOSS;

namespace BOSS
{
namespace Assert
{
    const std::string CurrentDateTime() 
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::stringstream ss;
        ss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
        return ss.str();
    }

  void ReportFailure(const GameState * /* state NOT USED ? */, const char * condition, const char * file, int line, const char * msg, ...)
    {
        std::cerr << "Assertion thrown!\n";
        // put extra arguments into a buffer big enough to hold any error message (hopefully)
        char error_message[1024*128];
        va_list argptr;
        va_start(argptr, msg);
        vsprintf(error_message, msg, argptr);
        va_end(argptr);

        // print error message
        std::stringstream ss;
        ss                                      << std::endl;
        ss << "!Assert:   " << condition        << std::endl;
        ss << "File:      " << file             << std::endl;
        ss << "Message:   " << error_message    << std::endl;
        ss << "Line:      " << line             << std::endl;
        
        // throw exception
        #if !defined(EMSCRIPTEN)
            std::cerr << ss.str();  
            throw BOSSException(ss.str());
        #else
            printf("BOSS Exception Thrown:\n %s\n", ss.str().c_str());
            throw BOSSException(ss.str());
        #endif
    }

    
}
}

