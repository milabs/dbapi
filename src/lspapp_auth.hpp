#ifndef __LSPAPP_AUTH_HPP__
#define __LSPAPP_AUTH_HPP__

#include "lspapp.hpp"

using namespace std;

// ==================== OAuthToken ==================== //

class OAuthToken
{
public:
	OAuthToken() {}
	OAuthToken(const OAuthToken & r) : mKey(r.mKey), mSecret(r.mSecret) {}
	OAuthToken(const string & key, const string & secret) : mKey(key), mSecret(secret) {}

	const string & key(const string & key = "");
	const string & secret(const string & secret = "");

	const char * c_key() const
		{
			return mKey.c_str();
		}
	const char * c_secret() const
		{
			return mSecret.c_str();
		}

	bool initiate(const char *);
	bool validate() const;

	// взаимодействие с сервисом аутентификации
	char * oauthRequest(const string & url, OAuthToken * token);
	pair<string, string> oauthRequest2(const string & url, OAuthToken * token, const string & method = "POST");
	bool oauthRequestToken(const string & url, OAuthToken * token);

	// save to tree / load from tree
	bool toTree(pt::ptree & pt) const;
	bool fromTree(const pt::ptree & pt);

private:
	string mKey, mSecret;
};

#endif /* __LSPAPP_AUTH_HPP__ */
