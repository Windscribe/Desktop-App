
#ifndef processes_helper_hpp
#define processes_helper_hpp

#include <vector>
#include <string>

// util class for find processes by name
class ProcessesHelper
{
public:
    ProcessesHelper();
    
    std::vector<pid_t> getPidsByProcessname(const char *name);
    bool isProcessFinished(pid_t pid) const;

private:
    std::vector< std::pair< std::string, pid_t > > processes_;
};

#endif 
