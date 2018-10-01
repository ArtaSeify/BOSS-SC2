#pragma once

#include "Common.h"
#include <cstdarg>
#include "BOSSException.h"

#include <ctime>

namespace BOSS
{

class GameState;
namespace Assert
{
    struct error
    {
        int integer;
        size_t unsigned_integer;
        double floating;
        char* string;

        bool integer_set = false;
        bool unsigned_integer_set = false;
        bool floating_set = false;
        bool string_set = false;
        
        error(int & new_integer) { integer = new_integer; }
        error(size_t & new_unsigned) { unsigned_integer = new_unsigned; }
        error(double & new_floating) { floating = new_floating; }
        error(char* & new_string) { string = new_string; }
        int & return_int() { return integer; }
        size_t & return_uint() { return unsigned_integer; }
        double & return_double() { return floating; }
        char* & return_char() { return string; }
    };

    const std::string CurrentDateTime();
    void ReportFailure(const GameState * state, const char * condition, const char * file, int line, const char * msg, ...);
}
}

#define BOSS_ASSERT_ENABLE

#ifdef BOSS_ASSERT_ENABLE

    #define BOSS_ASSERT(cond, msg, ...) \
        do \
        { \
            if (!(cond)) \
            { \
                BOSS::Assert::ReportFailure(nullptr, #cond, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
            } \
        } while(0)

    #define BOSS_ASSERT_STATE(cond, state, filename, msg, ...) \
        do \
        { \
            if (!(cond)) \
            { \
                BOSS::Assert::ReportFailure(&state, #cond, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
            } \
        } while(0)

#else
    #define SPARCRAFT_ASSERT(cond, msg, ...) 
    #define SPARCRAFT_ASSERT_STATE(cond, state, filename, msg, ...) 
#endif
