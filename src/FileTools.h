/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "BOSSAssert.h"
#include <string>
#include <sys/types.h> 
#include <sys/stat.h> 


namespace BOSS
{
    namespace FileTools
    {
        bool dirExists(const std::string & dir);
        void MakeDirectory(const std::string & dir);
    }    
}
