#pragma once

#include <QString>
#include <QWidget>

const int TOOLTIP_TIMEOUT = 750;
const int TOOLTIP_SHOW_DELAY = 750;
const int TOOLTIP_OFFSET_ROUNDED_CORNER = 6;
const int TOOLTIP_ROUNDED_RADIUS = 3;
const int TOOLTIP_TRIANGLE_HEIGHT = 6;
const int TOOLTIP_TRIANGLE_WIDTH = 12;

enum TooltipId {
    TOOLTIP_ID_NONE = 0,
    TOOLTIP_ID_SERVER_RATINGS,
    TOOLTIP_ID_CONNECT_TO_RATE,
    TOOLTIP_ID_FIREWALL_BLOCKED,
    TOOLTIP_ID_FIREWALL_INFO,
    TOOLTIP_ID_NETWORK_INFO,
    TOOLTIP_ID_FULL_SERVER_NAME,
    TOOLTIP_ID_CONNECTION_INFO,
    TOOLTIP_ID_SPLIT_ROUTING_MODE_INFO,
    TOOLTIP_ID_SPLIT_ROUTING_UNSUPPORTED,
    TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE,
    TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE_ERROR,
    TOOLTIP_ID_VIEW_LOG,
    TOOLTIP_ID_SEND_LOG,
    TOOLTIP_ID_LOG_SENT,
    TOOLTIP_ID_CLEAR_SERVER_RATINGS,
    TOOLTIP_ID_SERVER_RATINGS_CLEARED,
    TOOLTIP_ID_PROXY_IP_COPIED,
    TOOLTIP_ID_PREFERENCES_TAB_INFO,
    TOOLTIP_ID_LOGIN_ADDITIONAL_BUTTON_INFO,
    TOOLTIP_ID_LOCATIONS_TAB_INFO,
    TOOLTIP_ID_LOCATIONS_ITEM_CAPTION,
    TOOLTIP_ID_LOCATIONS_P2P,
    TOOLTIP_ID_LOCATIONS_PING_TIME,
    TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE,
    TOOLTIP_ID_CONFIG_FOOTER,
    TOOLTIP_ID_INVALID_LAN_ADDRESS,
    TOOLTIP_ID_CONNECTION_MODE_LOGIN,
    TOOLTIP_ID_STATIC_IP_DEVICE_NAME,
    TOOLTIP_ID_ELIDED_TEXT,
    TOOLTIP_ID_IP_UTILS_MENU,
};

enum TooltipType {
    TOOLTIP_TYPE_BASIC,
    TOOLTIP_TYPE_DESCRIPTIVE,
    TOOLTIP_TYPE_INTERACTIVE
};

enum TooltipTailType {
    TOOLTIP_TAIL_NONE,
    TOOLTIP_TAIL_LEFT,
    TOOLTIP_TAIL_BOTTOM
};

struct TooltipInfo {
    TooltipId id;
    int x;
    int y;
    TooltipTailType tailtype;
    double tailPosPercent;
    QString title;
    QString desc;
    int width;
    TooltipType type;
    int delay;
    bool animate;
    int animationSpeed;
    QWidget *parent;

    TooltipInfo(TooltipType type, TooltipId id)
        : id(id), x(0), y(0), tailtype(TOOLTIP_TAIL_NONE), tailPosPercent(0), width(0), type(type),
          delay(-1), animate(false), animationSpeed(0), parent(nullptr) {}
};

inline bool operator==(const TooltipInfo& lhs, const TooltipInfo& rhs)
{
    return lhs.id    == rhs.id
        && lhs.title == rhs.title
        && lhs.desc  == rhs.desc
        && lhs.type  == rhs.type;
}
