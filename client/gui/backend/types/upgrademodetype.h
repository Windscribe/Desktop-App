#pragma once

struct UpgradeModeType
{
    bool isNeedUpgrade;
    int gbLeft;
    int gbMax;

    UpgradeModeType() : isNeedUpgrade(false), gbLeft(0), gbMax(0) {}
};

inline bool operator==(const UpgradeModeType& lhs, const UpgradeModeType& rhs)
{
    return lhs.isNeedUpgrade == rhs.isNeedUpgrade && lhs.gbLeft == rhs.gbLeft && lhs.gbMax == rhs.gbMax;
}

inline bool operator!=(const UpgradeModeType& lhs, const UpgradeModeType& rhs)
{
    return lhs.isNeedUpgrade != rhs.isNeedUpgrade || lhs.gbLeft == rhs.gbLeft || lhs.gbMax == rhs.gbMax;
}
