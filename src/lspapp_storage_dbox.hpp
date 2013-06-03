#ifndef __LSPAPP_STORAGE_DBOX_HPP__
#define __LSPAPP_STORAGE_DBOX_HPP__

#include "lspapp.hpp"
#include "lspapp_auth.hpp"
#include "lspapp_storage.hpp"

// ==================== CLspStorageDropBox ==================== //

class CLspStorageDropBox : public CLspStorage
{
	enum dboxAPI_t {
		// authentication
		apiOAuthRequestToken = 0,
		apiOAuthAuthorize,
		apiOAuthAccessToken,
		// accounts
		apiAccountInfo,
		// files and metadata
		apiFiles,
		apiFilesPut,
		apiMeta,
		apiSearch,
		// file operations
		apiFileOpsCopy,
		apiFileOpsCreateFolder,
		apiFileOpsDetele,
		apiFileOpsMove,
		// this is the last one
		apiITEMS
	};

public:
	CLspStorageDropBox();
	virtual ~CLspStorageDropBox();

	bool init(pt::ptree * tree);
	bool status();

	// pure storage methods

	pt::ptree * stat(const string & path);
	pt::ptree * list(const string & path, const string & mask);

	pt::ptree * remove(const string & path);
	pt::ptree * rename(const string & old_path, const string & new_path);

	pt::ptree * create_directory(const string & path);
	pt::ptree * remove_directory(const string & path);

	// mixed storage and filesystem methods

	pt::ptree * get_file(const string & path, const fs::path & file);
	pt::ptree * put_file(const fs::path & file, const string & path);

private:
	bool connect();
	bool authenticate(int timeout);

	// чтение параметров
	bool readConfigTree(const pt::ptree & tree);

	// оформление путей
	string formatPath(dboxAPI_t api, const string & path, const string & args = "");
	string formatPath2(dboxAPI_t api, const string & path, const string & args = "");
	string formatPathFromTo(dboxAPI_t api, const string & path_fr, const string & path_to, const string & args = "");

	// запрос к сервису
	pt::ptree * convert(pt::ptree * pt);
	pt::ptree * request(const string & url);

	// токены доступа к сервису
	OAuthToken mAppToken;
	OAuthToken mAccessToken;

	// DropBox REST API
	string mApiRoot;
	string mApiHome;
	string mApiName[apiITEMS];
};

#endif /* __LSPAPP_STORAGE_DBOX_HPP__ */
