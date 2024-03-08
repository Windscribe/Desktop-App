**wsnet** is a library that is designed for Windscribe clients for all platforms Windows/Mac/Linux/Android/iOS providing access to server APIs in a unified way. Also contains some other networking features such as an asynchronous DNS resolver with the ability to use custom DNS servers and a http network manager with some much needed Windscribe client specific features.

# Motivation

The server API can be accessed by several dozen anti-censorship bypass methods. Implementation and debugging all of these methods for each platform is rather laborious and difficult to test. So the library hides all these details and provides clients with a single and simple interface. Further modification of the server API access methods will only require changes to the library code.  Also the library uses libraries like curl and openssl modified by ECH patches + our own to bypass censorship. Some such methods may be difficult or unrealizable to implement separately for Android/iOS platforms.

# Features
* Support for any architecture(x64, arm64 and so on) and platforms (Windows/Mac/Linux/Android/iOS).
* Ability to connect the library to a project via vcpkg tool (this is more relevant for desktop platforms).
* Asynchronous DNS resolver with support for custom DNS servers.
* Asynchronous HTTP manager providing the ability to whitelist IP-addresses in the firewall via callback functions.
* Asynchronous access to the Windscribe Server API with the ability to switch to staging servers.
* Implemented several dozen methods to bypass censorship used when accessing the server API.
* Functionality for ICMP and HTTP pinging.
* The library maintains logs that can be passed to the client.
* Asynchronous and thread-safe.
* Ready to use in Android and iOS immediately without writing additional code. Connection to Android and iOS projects through aar file and framework accordingly.
* Has automatic and semi-automatic tests. Tested with fairly good code coverage.

# How to build

The C++ compiler (XCode/Visual C++/clang), vcpkg tool and cmake must be installed before build.

## Desktop
To use the library in a desktop project, it is recommended to use vcpkg. Example of vcpkg settings to use the library.

vcpkg.json
```
{
    "dependencies": ["wsnet"]
}
```


vcpkg-configuration.json
```
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg-configuration.schema.json",
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/microsoft/vcpkg",
    "baseline": "61f610845fb206298a69f708104a51d651872877"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "git@gitlab.int.windscribe.com:ws/client/client-libs/ws-vcpkg-registry.git",
      "baseline": "0bff463c4aa39cdd251337a36e43f8eb9c05cb57",
      "packages": [ "openssl", "curl", "wsnet"]
    }
  ]
}
```

The vcpkg port and some of its dependencies(curl, openssl) are in the repository: `ws/client/client-libs/ws-vcpkg-registry.git`

## Android
* Android Studio must be installed.
* Run the build script: `build_android.sh`.
* Get a file `wsnet.aar` ready to use.

## iOS
* Run the build script: `build_ios.sh`.
* Get a framework `wsnet.framework` ready to use.
# How to use

The library must be initialized before it can be used, once during the lifetime of the application. Once initialized, all library functionality is available through a global instance of the WSNet class.

It is also recommended to install the logger callback function if necessary.

After finishing using the library, it is recommended to call the cleanup function, although not necessarily. The library will be cleaned in any case when the program finished. After calling cleanup, all pending callback functions will be canceled.

### Example (initialization):
```

#inlude <wsnet/WSNet.h>

using namespace wsnet;

void logger(const std::string &str)
{
    printf("%s\n", str.c_str());
}

int main()
{
    WSNet::setLogger(logger);
    WSNet::initialize("macos", "2.7.10", false, "");
    auto wsnet = WSNet::instance();

    // Working with the library
    ....

    WSNet::cleanup();
    return 0;
}

```
``
### Example (Dns resolver):
```

    wsnet->dnsResolver()->setDnsServers({ "1.1.1.1", "1.0.0.1"});
    // Blocking DNS resolution, waiting until complete
    auto res = wsnet->dnsResolver()->lookupBlocked("google.com");
    // work with res
    ...

    // Asynchronous DNS resolution
    auto callback = [](std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
    {

    };
    auto request = wsnet->dnsResolver()->lookup("google.com", 0, callback);

    // To cancel a request early, you can call: request->cancel()
```

### Example (Server API):
```
    wsnet->serverApi()->myIP([](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        printf("%s\n", jsonData.c_str());
    });

```


All asynchronous functions (implying the return of the result after some time) take a callback function as a parameter and return an object(CancelableCallback), through which the client can cancel the request early, if required. After canceling the request, the callback function is guaranteed not to be called. It is important that the function exists in memory at the time the callback is called. If the callback function is being removed from memory, you should call cancel for the request before doing so. If you don't need to cancel the request, you can ignore the return value(CancelableCallback).

All library functions/classes are fairly obvious and documented in the public library header files (todo: link here). So look there for full documentation.

Example code for Android(Kotlin):
```

    val logFunc = WSNet.Function0 { strLog : String ->
        runOnUiThread {
            // out log
        }
    }
    WSNet.setLogger(logFunc)
    WSNet.initialize("android", "1.1", false, "")

    WSNet.instance().serverAPI().myIP({ serverApiRetCode: Int, jsonData: String ->
        Log.v("Windscribe", jsonData)
    })

```
Note: It is necessary to connect `wsnet.aar` file to the project before use the library.


Example for iOS(Swift):
```

    let loggerFunc = { [](logStr: String) -> Void in
        print(logStr)
    }
    WSNet.setLogger(loggerFunc)
    WSNet.initialize("ios", appVersion: "1.1", isUseStagingDomains: false, serverApiSettings: "")
    let wsnet = WSNet.instance()
    wsnet.serverAPI().setIgnoreSslErrors(false)
    wsnet.serverAPI().myIP(
        { [](requestId: Int32, data: String) -> Void in
            print(data)
        })

```
Note: It is necessary to add `wsnet.framework` folder to the project before use the library. It is also necessary to create a bridging header and import the necessary library header files there.
# ServerAPI Notes

For the most part, ServerAPI functionality is what will mostly be used from this library. Here are some notes on how to work with it properly.

For serverAPI to work efficiently, it needs to save some settings to persistent storage. In particular this is the latest working method of circumventing censorship.

Thus the client before finalization should call `string settings = serverAPI()->currentSettings()`  and save the `settings` string to persistent storage.

When initializing the client, you should restore the settings from the storage and transfer them with the last parameter when initializing the library.
```
string serverApiSettings = readStringFromPersistentStorage();
WSNet::initialize("macos", "2.7.10", false, serverApiSettings);
```

Also the client must call functions `serverAPI()->setConnectivityState(bool isOnline)` when the connectivity state changes and `severAPI()->setIsConnectedToVpnState(bool isConnected)` when the VPN connectivity state changes.

The client can also control options such as whether to ignore SSL errors and set api resolution settings. The setTryingBackupEndpointCallback function can be useful when you want to show progress when searching for working failovers. In particular, this is implemented in the desktop client at the login stage.

Keep in mind that the ServerAPI server functions can be called in any order any number of times from any thread. The callback function will be guaranteed to be called (unless of course it has been canceled) but the time after which it will be called can be any. It can also be quite long while the failovers are being gone through.
The following results may be returned in the callback function:
* *kSuccess = 0*. Successful execution and the json data can be used. The json is guaranteed to be correct(it will definitely be json and will contain fields such as data and error(if this field is present).
* *kNetworkError = 1*, *kNoNetworkConnection = 2*. Various network errors, it is recommended to repeat the request from the client side
* *kFailoverFailed = 3*. This means that all failovers have been tried and none of them work. In this case, it is recommended to prompt the user to ignore SSL-errors and try the queries again. Otherwise, there's nothing more we can do. There will be no connection to the server API.

# Further improvements
* For Server API functions to make results return as wrapper classes instead of raw json data?