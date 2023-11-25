#pragma once

#include <string>
#include <vector>

class ArgumentsParser
{
public:
    bool parse();
    std::wstring getExecutablePath() const;
    bool getArg(const std::wstring &argName, std::wstring *outValue = nullptr);

private:
    std::wstring executablePath_;
    std::vector< std::pair<std::wstring, std::wstring> > args_;
};

