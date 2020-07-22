#include "simple_crypt.h"


std::string SimpleCrypt::encrypt(std::string data, const std::string &key)
{
	std::string xorstring = data; 
	for (size_t i = 0; i < xorstring.size(); i++) 
	{ 
        xorstring[i] = data[i] ^ key[i % key.size()];
	}
	return xorstring;
}

std::string SimpleCrypt::decrypt(std::string data, const std::string &key)
{
	return encrypt(data, key);
}
