#include "utils.h"
#include "3rdparty/pstream.h"

namespace Utils
{

// based on 3rd party lib (http://pstreams.sourceforge.net/)
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    std::string cmdLine = cmd;

    for (auto it = args.begin(); it != args.end(); ++it)
    {
        cmdLine += " \"";
        cmdLine += *it;
        cmdLine += "\"";
    }

    if (pOutputStr)
    {
        pOutputStr->clear();
    }

    redi::ipstream proc(cmdLine, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
    {
        if (pOutputStr)
        {
            *pOutputStr += line + "\n";
        }
    }
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
    {
        proc.clear();
    }

    if (appendFromStdErr) {
        // read child's stderr
        while (std::getline(proc.err(), line))
        {
            if (pOutputStr)
            {
                *pOutputStr += line + "\n";
            }
        }
    }

    proc.close();
    if (proc.rdbuf()->exited())
    {
         return proc.rdbuf()->status();
    }
    return 0;
}


size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos)
{
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    std::transform(toSearch.begin(), toSearch.end(), toSearch.begin(), ::tolower);
    return data.find(toSearch, pos);
}

}
