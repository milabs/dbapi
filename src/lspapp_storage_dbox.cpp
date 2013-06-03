#include "lspapp.hpp"
#include "lspapp_auth.hpp"
#include "lspapp_storage_dbox.hpp"

static void authorize(const string & url)
{
	string cmd("xdg-open " + url + " &>/dev/null");

	if (!system(cmd.c_str()))
		return;

	cout << "Proceed the authorization at the link below:" << endl << url << endl;
}

static size_t curl_devnull(char * ptr, size_t size, size_t nmemb, void * data)
{
	return size * nmemb;
}

static CURLcode curl_post_file_multipart(const char * u, const char * h, const fs::path & file, const string & filename)
{
	CURLcode result = CURLE_FAILED_INIT;

	struct curl_httppost * lastptr = NULL;
	struct curl_httppost * formpost = NULL;

	curl_formadd(&formpost,
		     &lastptr,
		     CURLFORM_COPYNAME, "file",
		     CURLFORM_FILE, file.c_str(),
		     CURLFORM_FILENAME, filename.c_str(),
		     CURLFORM_END);

	CURL * curl = curl_easy_init();
	if (curl) {
		struct curl_slist * slist = NULL;

		curl_easy_setopt(curl, CURLOPT_URL, u);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_devnull);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		if (h) {
			slist = curl_slist_append(slist, h);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
		}
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

		result = curl_easy_perform(curl);
		curl_easy_cleanup(curl); 

		curl_slist_free_all(slist);
	}

	curl_formfree(formpost);

	return result;
}

// ==================== CLspStorageDropBox ==================== //

CLspStorageDropBox::CLspStorageDropBox() :
	mApiRoot("sandbox"),
	mApiHome("default")
{
	// authentication
	mApiName[apiOAuthRequestToken]
		= "https://api.dropbox.com/1/oauth/request_token";
	mApiName[apiOAuthAuthorize]
		= "https://www.dropbox.com/1/oauth/authorize";
	mApiName[apiOAuthAccessToken]
		= "https://api.dropbox.com/1/oauth/access_token";

	// accounts
	mApiName[apiAccountInfo]
		= "https://api.dropbox.com/1/account/info";
	
	// files and metadata
	mApiName[apiFiles]
		= "https://api-content.dropbox.com/1/files";
	mApiName[apiFilesPut]
		= "https://api-content.dropbox.com/1/files_put";
	mApiName[apiMeta]
		= "https://api.dropbox.com/1/metadata";
	mApiName[apiSearch]
		= "https://api.dropbox.com/1/search";

	// file operations
	mApiName[apiFileOpsCopy]
		= "https://api.dropbox.com/1/fileops/copy";
	mApiName[apiFileOpsCreateFolder]
		= "https://api.dropbox.com/1/fileops/create_folder";
	mApiName[apiFileOpsDetele]
		= "https://api.dropbox.com/1/fileops/delete";
	mApiName[apiFileOpsMove]
		= "https://api.dropbox.com/1/fileops/move";
}

CLspStorageDropBox::~CLspStorageDropBox()
{
	NOT_IMPLEMENTED();
}

// ==================== CLspStorageDropBox : public methods ==================== //

bool CLspStorageDropBox::init(pt::ptree * tree)
{
	if (!readConfigTree(*tree))
		return false;

	if (!connect() && !authenticate(60))
		return false;

	delete create_directory("/");

	// update the configuration
	tree->put("account.info.uuid", mApiHome);
	mAppToken.toTree(tree->put_child("account.auth.app", pt::ptree()));
	mAccessToken.toTree(tree->put_child("account.auth.access", pt::ptree()));

	return true;
}

bool CLspStorageDropBox::status()
{
	if (!connect())
		return false;

	pt::ptree * pt = request(mApiName[apiAccountInfo]);
	if (pt) {
		cout << "DropBox account information" << endl;
		pt::json_parser::write_json(cout, *pt);
		delete pt;
	}

	return pt != NULL;
}

pt::ptree * CLspStorageDropBox::list(const string & path, const string & mask)
{
	pt::ptree * pt = NULL;

	// query for the metadata
	pt = request(formatPath(apiMeta, path, "list=true"));

	// return contents only for dirs
	if (pt && pt->get("is_dir", "false") == "true") {
		pt::ptree * list = new pt::ptree();

		// фильтруем файлы
		boost::regex exp(mask);

		// преобразование результата, отделение имени файла от его полного имени
		BOOST_FOREACH(const pt::ptree::value_type & v, pt->get_child("contents", pt::ptree()))
		{
			string name = fs::path(v.second.get("path", "")).filename().string();

			if (!boost::regex_match(name, exp))
				continue;

			list->push_back(make_pair(name, v.second));
		}
		delete pt, pt = list;
	} else
		delete pt, pt = NULL;

	return convert(pt);
}

pt::ptree * CLspStorageDropBox::stat(const string & path)
{
	pt::ptree * pt = NULL;

	// query for the metadata
	pt = request(formatPath(apiMeta, path));

	return convert(pt);
}

pt::ptree * CLspStorageDropBox::remove(const string & path)
{
	pt::ptree * pt = NULL;

	// query for the delete
	pt = request(formatPath2(apiFileOpsDetele, path));
	if (pt && (pt->get("is_deleted", "false") == "true")) {
		// nop
	} else
		delete pt, pt = NULL;

	return convert(pt);
}

pt::ptree * CLspStorageDropBox::rename(const string & old_path, const string & new_path)
{
	pt::ptree * pt = NULL;

	// query for remove
	pt = request(formatPathFromTo(apiFileOpsMove, old_path, new_path));

	return convert(pt);
}

pt::ptree * CLspStorageDropBox::create_directory(const string & path)
{
	pt::ptree * pt = NULL;

	// query for the create_folder
	pt = request(formatPath2(apiFileOpsCreateFolder, path));
	if (pt && (pt->get("is_dir", "false") == "true")) {
		// nop
	} else
		delete pt, pt = NULL;

	return convert(pt);
}

pt::ptree * CLspStorageDropBox::remove_directory(const string & path)
{
	pt::ptree * pt = NULL;

	// check for the object
	pt = stat(path);
	if (pt && (pt->get("is_dir", "false") == "true")) {
		delete pt, pt = NULL;

		// query for the delete
		pt = request(formatPath2(apiFileOpsDetele, path));
		if (pt && (pt->get("is_deleted", "false") == "true")) {
			// nop
		} else
			delete pt, pt = NULL;
	} else
		delete pt, pt = NULL;

	return convert(pt);
}

pt::ptree * CLspStorageDropBox::put_file(const fs::path & file, const string & path)
{
	string url;

	if (!fs::exists(file))
		return NULL;

	string file_name = fs::path(path).filename().string();
	string file_path = fs::path(path).parent_path().string();

	url = formatPath(apiFiles,
			 file_path, "file=" + file_name + "&filename=" + file_name + "&overwrite=true");

	pair<string, string> ss = mAppToken.oauthRequest2(url, &mAccessToken);
	if (CURLE_OK != curl_post_file_multipart(ss.first.c_str(), ss.second.c_str(), file, file_name))
		return NULL;

	return stat(path);
}

pt::ptree * CLspStorageDropBox::get_file(const string & path, const fs::path & file)
{
	NOT_IMPLEMENTED();
	return NULL;
}

// ==================== CLspStorageDropBox : private methods ==================== //

bool CLspStorageDropBox::connect()
{
	pt::ptree * pt;
	bool result = false;

	pt = request(mApiName[apiAccountInfo]);
	if (pt) {
		if (pt->find("uid") != pt->not_found())
			result = true;
		delete pt;
	}

	return result;
}

bool CLspStorageDropBox::authenticate(int timeout)
{
	string url;
	OAuthToken token;

	url = mApiName[apiOAuthRequestToken];
	if (!mAppToken.oauthRequestToken(url, &token))
		return false;

	url = mApiName[apiOAuthAuthorize];
	authorize(url + "?oauth_token=" + token.key());

	time_t stoptime = time(NULL) + timeout;
	do {
		url = mApiName[apiOAuthAccessToken];
		if (mAppToken.oauthRequestToken(url, &token)) {
			mAccessToken = token;
			return true;
		}
	} while (sleep(1), stoptime > time(NULL));

	return false;
}

bool CLspStorageDropBox::readConfigTree(const pt::ptree & tree)
{
	string value;

	// значения в конфиг-файле...

	mApiHome = tree.get("account.info.uuid", "default");
	mAppToken.fromTree(tree.get_child("account.auth.app", pt::ptree()));
	mAccessToken.fromTree(tree.get_child("account.auth.access", pt::ptree()));

	// данные командной строки могут быть важнее...

	mApiHome = tree.get("args.uuid", mApiHome);
	if (tree.get("args.key", "") != "" && tree.get("args.secret", "") != "")
		mAppToken.fromTree(tree.get_child("args"));

	// некоторые проверки можно делать уже сейчас...

	if (!mAppToken.validate())
		throw string("Invalid application token");

	return true;
}

string CLspStorageDropBox::formatPath(dboxAPI_t api, const string & path, const string & args)
{
	string result;

	result = (path[0] == '/') ?
		mApiHome + path : mApiHome + "/" + path;

	//
	// формируем путь по шаблону: API/<root>/<path>?args...
	//

	result = mApiName[api] + "/" + mApiRoot + "/" + result;
	if (!args.empty())
		result += "?" + args;
#ifdef DEBUG
	cerr << result << endl;
#endif
	return result;
}

string CLspStorageDropBox::formatPath2(dboxAPI_t api, const string & path, const string & args)
{
	string result;

	result = (path[0] == '/') ?
		mApiHome + path : mApiHome + "/" + path;

	//
	// формируем путь по шаблону: API?root=<root>&path=<path>&args...
	//

	result = mApiName[api] + "?root=" + mApiRoot + "&path=" + result;
	if (!args.empty())
		result += "&" + args;
#ifdef DEBUG
	cerr << result << endl;
#endif
	return result;
}

string CLspStorageDropBox::formatPathFromTo(dboxAPI_t api, const string & path_fr, const string & path_to, const string & args)
{
	string result, fp, tp;

	fp = (path_fr[0] == '/') ?
		mApiHome + path_fr : mApiHome + "/" + path_fr;
	tp = (path_to[0] == '/') ?
		mApiHome + path_to : mApiHome + "/" + path_to;

	//
	// формируем путь по шаблону: API?root=<root>&from_path=<from_path>&to_path=<to_path>&args...
	//

	result = mApiName[api] + "?root=" + mApiRoot + "&from_path=" + fp + "&to_path=" + tp;
	if (!args.empty())
		result += "&" + args;
#ifdef DEBUG
	cerr << result << endl;
#endif
	return result;
}

pt::ptree * CLspStorageDropBox::convert(pt::ptree * pt)
{
	if (!pt)
		return pt;

	BOOST_FOREACH(pt::ptree::value_type &v, *pt) {
		string str;

		// convert modified to UNIX time (GMT -> localtime)
		str = v.second.get("modified", "");
		if (!str.empty()) {
			struct tm tm_gmt = {0}, tm_local = {0};
			if (strptime(str.c_str(), "%a, %d %b %Y %H:%M:%S %z", &tm_gmt)) {
				time_t posix_time = timegm(&tm_gmt);
				localtime_r(&posix_time, &tm_local);
				v.second.put("modified", mktime(&tm_local));
			}
		}
	}

	return pt;
}

pt::ptree * CLspStorageDropBox::request(const string & url)
{
	char * recv = mAppToken.oauthRequest(url, &mAccessToken);
	if (!recv)
		return NULL;

	pt::ptree * pt = new pt::ptree;
	try {
		stringstream data(recv);
		pt::json_parser::read_json(data, *pt);
		if (pt->find("error") == pt->not_found()) {
			free(recv);
			return pt;
		}
	} catch (pt::json_parser::json_parser_error& e) {
		cerr << e.what() << endl;
	}
	delete pt;

	free(recv);
	return NULL;
}
