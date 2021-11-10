#ifndef IINSTALL_BLOCK_H
#define IINSTALL_BLOCK_H

#include <string>

class IInstallBlock
{
protected:
	double weight_;
	std::wstring name_;
	std::wstring lastError_;

public:
	IInstallBlock(double weight, const std::wstring name) : weight_(weight), name_(name) {}
	virtual ~IInstallBlock(){ }

	double getWeight() const { return weight_;  }

	// 0..100   progress
	// >=100      finished
	// -1       error
	virtual int executeStep() = 0;
	std::wstring getLastError() { return lastError_; }
    std::wstring getName() const { return name_; }
};

#endif // IINSTALL_BLOCK_H
