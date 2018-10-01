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
        std::string error_message;
        if (msg != NULL)
        {
            // count the number of arguments
            char curr_char;
            size_t index = 0;
            size_t prev_substring_index = 0;
            std::vector<char> arg_types;
            std::vector<std::pair<int, int>> sub_msgs;
            do
            {
                curr_char = msg[index++];
                if (curr_char == '%')
                {
                    arg_types.push_back(msg[index++]);
                    sub_msgs.push_back(std::make_pair(prev_substring_index, index));
                    prev_substring_index = index + 1;   // +1 to get rid of the empty space in between words
                }
            } while (curr_char != NULL);

            // if there are no arguments, just copy the string 
            if (arg_types.size() == 0)
            {
                error_message = msg;
            }

            else
            {
                // get the values of all the extra arguments and place them inside the strings
                va_list args;
                va_start(args, msg);
                std::vector<std::string> messages;
                for (size_t i(0); i < arg_types.size(); ++i)
                {
                    char message[4096] = "";
                    char temp[4096] = "";
                    memcpy(temp, msg + sub_msgs[i].first, sub_msgs[i].second - sub_msgs[i].first + 1);
                    // put the extra parameters inside of msg
                    if (arg_types[i] == 's')
                    {
                        sprintf(message, temp, va_arg(args, char*));
                    }
                    else if (arg_types[i] == 'd')
                    {
                        sprintf(message, temp, va_arg(args, int));
                    }
                    else if (arg_types[i] == 'f')
                    {
                        sprintf(message, temp, va_arg(args, double));
                    }
                    else if (arg_types[i] == 'u')
                    {
                        sprintf(message, temp, va_arg(args, size_t));
                    }
                    messages.push_back(message);
                }
                va_end(args);

                for (size_t i(0); i < messages.size(); ++i)
                {
                    error_message += messages[i];
                }
            }            
        }	

        std::stringstream ss;
        ss                                      << std::endl;
        ss << "!Assert:   " << condition        << std::endl;
        ss << "File:      " << file             << std::endl;
        ss << "Message:   " << error_message    << std::endl;
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

