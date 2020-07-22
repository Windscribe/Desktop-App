#pragma once
#include <string>

class CertStorage
{
public:
	CertStorage();

	std::string getPrivateKey() { return privateKey_; }
	std::string getCert() { return crt_; }

private:
	std::string privateKey_;
	std::string crt_;

	void loadCerts();
};

