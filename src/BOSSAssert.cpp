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

    void ReportFailure(const GameState * state, const char * condition, const char * file, int line, const char * msg, ...)
    {
        std::cerr << "Assertion thrown!\n";
        // get the extra parameters
        char messageBuffer[4096] = "";
        
        if (msg != NULL)
        {
            // count the number of arguments
            /*char temp;
            size_t index = 0;
            std::vector<char> arg_types;
            do
            {
                temp = msg[index++];
                if (temp == '%')
                {
                    arg_types.push_back(msg[index++]);
                }
            } while (temp != NULL);

            // if there are no arguments, just copy the string 
            if (arg_types.size() == 0)
            {
                strcpy(messageBuffer, msg);
            }

            // get the values of all the extra arguments
            std::vector<error> error_args;
            va_list args;
            va_start(args, msg);
            for (size_t i(0); i < arg_types.size(); ++i)
            {
                // put the extra parameters inside of msg
                if (arg_types[i] == 's')
                {
                    error_args.push_back(error(va_arg(args, char*)));
                }
                else if (arg_types[i] == 'd')
                {
                    error_args.push_back(error(va_arg(args, int)));
                }
                else if (arg_types[i] == 'f')
                {
                    error_args.push_back(error(va_arg(args, double)));
                }
                else if (arg_types[i] == 'u')
                {
                    error_args.push_back(error(va_arg(args, size_t)));
                }
            }
            va_end(args);
            
            for (size_t i(0); i < 1; ++i)
            {
                // put the extra parameters inside of msg
                if (arg_types[i] == 's')
                {
                    sprintf(messageBuffer, msg, arg1, arg2);
                }
                else if (arg_types[i] == 'd')
                {
                    sprintf(messageBuffer, msg, va_arg(args, int));
                }
                else if (arg_types[i] == 'f')
                {
                    sprintf(messageBuffer, msg, va_arg(args, double));
                }
                else if (arg_types[i] == 'u')
                {
                    sprintf(messageBuffer, msg, va_arg(args, size_t));
                }
            }*/
        }	

        std::stringstream ss;
        ss                                      << std::endl;
        ss << "!Assert:   " << condition        << std::endl;
        ss << "File:      " << file             << std::endl;
        ss << "Message:   " << messageBuffer    << std::endl;
        ss << "Line:      " << line             << std::endl;
        
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

