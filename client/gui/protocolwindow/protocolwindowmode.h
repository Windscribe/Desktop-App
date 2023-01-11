#pragma once

enum ProtocolWindowMode {
    kUninitialized = 0,
    kChangeProtocol = 1,
    kFailedProtocol = 2,
    kSavePreferredProtocol = 3,
    kSendDebugLog = 4,
    kDebugLogSent = 5,
};