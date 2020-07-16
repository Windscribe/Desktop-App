#include <string>
#include <vector>
#include "simple_xor_crypt.h"

std::string SimpleXorCrypt::encrypt(std::string &data, const std::string &key)
{
	int gg = key.size();
	std::string xorstring = data; 
	for (size_t i = 0; i < xorstring.size(); i++) 
	{ 
        xorstring[i] = data[i] ^ key[i % key.size()];
	}
	return xorstring;
}

std::string SimpleXorCrypt::decrypt(std::string &data, const std::string &key)
{
	return encrypt(data, key);
}
