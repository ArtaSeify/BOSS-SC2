/* -*- c-basic-offset: 4 -*- */

#include "FileTools.h"
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h> 

#ifdef WIN32   // Windows system specific
    #include <windows.h>
    #include <direct.h>
#else          // Unix based system specific
    #include <sys/time.h>
#endif

using namespace BOSS;
//namespace fs = boost::filesystem;

bool FileTools::dirExists(const std::string & dir)
{
#ifdef WIN32
    DWORD ftyp = GetFileAttributesA(dir.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
    {
        return false;  //something is wrong with your path!
    }

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;   // this is a directory!
    }

    return false;    // this is not a directory!
#endif
}

void FileTools::MakeDirectory(const std::string & dir)
{
    int nError = 0;

    std::string dir_name = "";
    for (auto chr : dir)
    {
        dir_name += chr;
        if (chr == '/' && !dirExists(dir_name))
        {
            //fs::create_directory(dir_name);
#ifdef WIN32
            nError = _mkdir(dir_name.c_str()); // can be used on Windows
#else 
            mode_t nMode = 0733; // UNIX style permissions
            nError = mkdir(dir.c_str(), nMode); // can be used on non-Windows
#endif
        }
    }

    if (!dirExists(dir))
    {
        //fs::create_directory(dir_name);
#ifdef WIN32
        nError = _mkdir(dir_name.c_str()); // can be used on Windows
#else 
        mode_t nMode = 0733; // UNIX style permissions
        nError = mkdir(dir.c_str(), nMode); // can be used on non-Windows
#endif
    }

    if (nError != 0) {
        BOSS_ASSERT(false, "Could not make directory %s", dir.c_str());
    }
}
