#ifndef __LSPAPP_STORAGE_LOCAL_HPP__
#define __LSPAPP_STORAGE_LOCAL_HPP__

#include "lspapp.hpp"
#include "lspapp_storage.hpp"

using namespace std;

// ==================== CLspStorageLocal ==================== //

class CLspStorageLocal : public CLspStorage
{

public:
	CLspStorageLocal();
	virtual ~CLspStorageLocal();

	bool init(pt::ptree * tree);
	bool status();

	pt::ptree * stat(const string & path);
	pt::ptree * list(const string & path, const string & mask);

	pt::ptree * remove(const string & path);
	pt::ptree * rename(const string & old_path, const string & new_path);

	pt::ptree * create_directory(const string & path);
	pt::ptree * remove_directory(const string & path);

	pt::ptree * get_file(const string & path, const fs::path & file);
	pt::ptree * put_file(const fs::path & file, const string & path);

private:
	fs::path formatPath(const string & path);

	bool real_stat(const fs::path & file, pt::ptree * pt);

	fs::path mRootPath;
};

#endif /* __LSPAPP_STORAGE_LOCAL_HPP__ */
