target_sources(engine PRIVATE
        connecteduserscounter.cpp
        connecteduserscounter.h
        httpproxyserver/httpproxyconnection.cpp
        httpproxyserver/httpproxyconnection.h
        httpproxyserver/httpproxyconnectionmanager.cpp
        httpproxyserver/httpproxyconnectionmanager.h
        httpproxyserver/httpproxyheader.h
        httpproxyserver/httpproxyreply.cpp
        httpproxyserver/httpproxyreply.h
        httpproxyserver/httpproxyrequest.cpp
        httpproxyserver/httpproxyrequest.h
        httpproxyserver/httpproxyrequestparser.cpp
        httpproxyserver/httpproxyrequestparser.h
        httpproxyserver/httpproxyserver.cpp
        httpproxyserver/httpproxyserver.h
        httpproxyserver/httpproxywebanswer.cpp
        httpproxyserver/httpproxywebanswer.h
        httpproxyserver/httpproxywebanswerparser.cpp
        httpproxyserver/httpproxywebanswerparser.h
        socketutils/socketwriteall.cpp
        socketutils/socketwriteall.h
        socksproxyserver/socksproxycommandparser.cpp
        socksproxyserver/socksproxycommandparser.h
        socksproxyserver/socksproxyconnection.cpp
        socksproxyserver/socksproxyconnection.h
        socksproxyserver/socksproxyconnectionmanager.cpp
        socksproxyserver/socksproxyconnectionmanager.h
        socksproxyserver/socksproxyidentreqparser.cpp
        socksproxyserver/socksproxyidentreqparser.h
        socksproxyserver/socksproxyreadexactly.cpp
        socksproxyserver/socksproxyreadexactly.h
        socksproxyserver/socksproxyserver.cpp
        socksproxyserver/socksproxyserver.h
        socksproxyserver/socksstructs.h
        vpnsharecontroller.cpp
        vpnsharecontroller.h
)

if (WIN32)
    target_sources(engine PRIVATE
        wifisharing/icsmanager.cpp
        wifisharing/icsmanager.h
        wifisharing/wifisharing.cpp
        wifisharing/wifisharing.h
        wifisharing/wlanmanager.cpp
        wifisharing/wlanmanager.h
    )
endif (WIN32)
