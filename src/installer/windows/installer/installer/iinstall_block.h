#pragma once

#include <string>

class IInstallBlock
{
public:
    IInstallBlock(double weight, const std::wstring name, bool critical = true) : weight_(weight), name_(name), critical_(critical) {}
    virtual ~IInstallBlock() { }

    double getWeight() const { return weight_; }

    // 0..100   progress
    // >=100      finished
    // -1       error
    virtual int executeStep() = 0;
    std::wstring getLastError() { return lastError_; }
    std::wstring getName() const { return name_; }
    bool isCritical() const { return critical_; }

protected:
    const double weight_;
    const std::wstring name_;
    const bool critical_;

    std::wstring lastError_;
};
