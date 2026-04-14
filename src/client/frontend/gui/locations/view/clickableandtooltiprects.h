#pragma once

namespace gui_locations {

enum class TooltipRect {
    kNone = -1,
    kPingTime,
    kItemCaption,
    kItemNickname,
    kCountryCaption,
    kCustomConfigErrorMessage,
};

enum class ClickableRect {
    kNone = -1,
    kFavorite,
    kErrorIcon
};

} // namespace gui
