#pragma once

class CheckRunningApp_mac
{
public:
    // check previously running application instance and activate it, if found
    // return true in bShouldCloseCurrentApp, if need finish current instance of app
    static void checkPrevInstance(bool &bShouldCloseCurrentApp);
};
