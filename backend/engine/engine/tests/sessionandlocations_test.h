#ifndef SESSIONANDLOCATIONS_H
#define SESSIONANDLOCATIONS_H

#include <QByteArray>

//#define TEST_CREATE_API_FILES
//#define TEST_API_FROM_FILES

// for testing session and locations change
// for create test files with API todo:
// 1) #define TEST_CREATE_API_FILES
// 2) create folder c:\5
// 3) after execute you will see 2 files in folder c:\5 -> session.api and locations.api
// 4) rename files to session_free.api or session_pro.api, to locations_free.api or locations_pro.api

// for testing define TEST_API_FROM_FILES

class SessionAndLocationsTest
{
public:
    static SessionAndLocationsTest &instance()
    {
        static SessionAndLocationsTest s;
        return s;
    }

    QByteArray getSessionData();
    QByteArray getLocationsData();

    bool toggleSessionStatus();


private:
    SessionAndLocationsTest();

private:
    bool isFreeSessionNow_;

};

#endif // SESSIONANDLOCATIONS_H
