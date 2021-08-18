#pragma once

class PrivilegeHelper final
{
public:
    PrivilegeHelper();
    ~PrivilegeHelper();
    bool checkElevation() const;
    bool checkElevationForHandle(HANDLE handle) const;

private:
    bool is_elevated_;
};
