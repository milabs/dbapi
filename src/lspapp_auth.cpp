#include "lspapp.hpp"
#include "lspapp_auth.hpp"

// ==================== OAuthToken ==================== //

const string & OAuthToken::key(const string & key)
{
	if (!key.empty())
		mKey = key;
	return mKey;
}

const string & OAuthToken::secret(const string & secret)
{
	if (!secret.empty())
		mSecret = secret;
	return mSecret;
}

bool OAuthToken::validate() const
{
	return (mKey.length() == 15 && mSecret.length() == 15);
}

bool OAuthToken::initiate(const char * p)
{
	if (!p)
		return false;

	string str(p);

	int ps1 = str.find("oauth_token=");
	if (ps1 == string::npos)
		return false;

	int ps2 = str.find("oauth_token_secret=");
	if (ps2 == string::npos)
		return false;

	mKey = str.substr(ps1 + 12, 15);
	mSecret = str.substr(ps2 + 19, 15);

	return true;
}

char * OAuthToken::oauthRequest(const string & url, OAuthToken * token)
{
	const char * c_key = NULL, * c_secret = NULL;
	const char * t_key = NULL, * t_secret = NULL;

	c_key = mKey.c_str();
	c_secret = mSecret.c_str();
	
	if (token && token->validate()) {
		t_key = token->c_key();
		t_secret = token->c_secret();
	}

	char * send = NULL, * recv = NULL, * args = NULL;

	send = oauth_sign_url2(url.c_str(), &args, OA_HMAC, NULL, c_key, c_secret, t_key, t_secret);
	if (send)
		recv = oauth_http_post2(send, args, NULL);
	free(send), free(args);

	return recv;
}

pair<string, string> OAuthToken::oauthRequest2(const string & url, OAuthToken * token, const string & method)
{
	const char * c_key = NULL, * c_secret = NULL;
	const char * t_key = NULL, * t_secret = NULL;

	c_key = mKey.c_str();
	c_secret = mSecret.c_str();
	
	t_key = token->c_key();
	t_secret = token->c_secret();

	char ** argv = NULL;
	int argc = oauth_split_url_parameters(url.c_str(), &argv);

	oauth_sign_array2_process(&argc, &argv, NULL, OA_HMAC,
				  method.c_str(), c_key, c_secret, t_key, t_secret);

	char * req_url = oauth_serialize_url_sep(argc, 0, argv, (char *)"&", 1);
	char * req_hdr = oauth_serialize_url_sep(argc, 1, argv, (char *)", ", 6);

	oauth_free_array(&argc, &argv);

	string result_url(req_url);
	string result_hdr(req_hdr);

	result_hdr = "Authorization: OAuth " + result_hdr;

	free(req_url), free(req_hdr);

	return make_pair(result_url, result_hdr);
}

bool OAuthToken::oauthRequestToken(const string & url, OAuthToken * token)
{
	char * recv;

	recv = oauthRequest(url, token);
	if (recv) {
		OAuthToken other;

		other.initiate(recv);
		free(recv);

		if (other.validate()) {
			*token = other;
			return true;
		}
	}

	return false;
}

bool OAuthToken::toTree(pt::ptree & pt) const
{
	pt.put("key", mKey);
	pt.put("secret", mSecret);

	return true;
}

bool OAuthToken::fromTree(const pt::ptree & pt)
{
	mKey = pt.get("key", "");
	mSecret = pt.get("secret", "");

	return validate();
}
