#include "lspapp.hpp"
#include "lspapp_storage_local.hpp"

// ==================== CLspStorageLocal : public methods ==================== //

CLspStorageLocal::CLspStorageLocal() : mRootPath("default")
{
}

CLspStorageLocal::~CLspStorageLocal()
{
	NOT_IMPLEMENTED();
}

bool CLspStorageLocal::init(pt::ptree * tree)
{
	boost::system::error_code error;

	mRootPath = tree->get("config.logpath", "default");

	fs::create_directories(mRootPath, error);
	if (!fs::exists(mRootPath, error))
		return false;

	return true;
}

bool CLspStorageLocal::status()
{
	return true;
}

pt::ptree * CLspStorageLocal::stat(const string & path)
{
	pt::ptree * pt = new pt::ptree;

	if ( !real_stat(formatPath(path), pt) )
		delete pt, pt = NULL;

	return pt;
}

pt::ptree * CLspStorageLocal::list(const string & path, const string & mask)
{
	fs::path file = formatPath(path);

	if ( !exists(file) )
		return NULL;

	pt::ptree * pt = new pt::ptree;
	if (pt) {
		// фильтруем файлы
		boost::regex exp(mask);

		fs::directory_iterator last;
		for (fs::directory_iterator it(file); it != last; ++it) {
			pt::ptree item;

			if (!regex_match(it->path().filename().string(), exp))
			    continue;

			if ( real_stat(it->path(), &item) )
				pt->push_back(make_pair(it->path().filename().string(), item));
		}
	}

	return pt;
}

pt::ptree * CLspStorageLocal::remove(const string & path)
{
	boost::system::error_code error;

	pt::ptree * pt = stat(path);
	if (pt) {
		fs::remove_all(pt->get("path", ""), error);
		if ( !error )
			pt->put("is_deleted", "true");
	}

	return pt;
}

pt::ptree * CLspStorageLocal::rename(const string & old_path, const string & new_path)
{
	boost::system::error_code error;

	fs::rename(formatPath(old_path), formatPath(new_path), error);
	if ( error )
		return NULL;

	return stat(new_path);
}

pt::ptree * CLspStorageLocal::create_directory(const string & path)
{
	boost::system::error_code error;

	if ( !fs::create_directory(formatPath(path), error) )
		return NULL;

	return stat(path);
}

pt::ptree * CLspStorageLocal::remove_directory(const string & path)
{
	pt::ptree pt;

	if ( real_stat(formatPath(path), &pt) ) {
		if (pt.get("is_dir", "false") == "true")
			return remove(path);
	}

	return NULL;
}

pt::ptree * CLspStorageLocal::get_file(const string & path, const fs::path & file)
{
	boost::system::error_code error;

	copy_file(formatPath(path), file, fs::copy_option::overwrite_if_exists, error);
	if ( error )
		return NULL;

	return stat(path);
}

pt::ptree * CLspStorageLocal::put_file(const fs::path & file, const string & path)
{
	boost::system::error_code error;

	copy_file(file, formatPath(path), fs::copy_option::overwrite_if_exists, error);
	if ( error )
		return NULL;

	return stat(path);
}

// ==================== CLspStorageLocal : private methods ==================== //

fs::path CLspStorageLocal::formatPath(const string & path)
{
	return mRootPath / path;
}

bool CLspStorageLocal::real_stat(const fs::path & file, pt::ptree * pt)
{
	if ( !pt || !fs::exists(file) )
		return false;

	try {
		pt->put("path", file.generic_string());
		pt->put("bytes", is_directory(file) ? 0 : file_size(file));
		pt->put("is_dir", is_directory(file) ? "true" : "false");
		pt->put("modified", last_write_time(file));
	} catch (fs::filesystem_error& e) {
		cerr << BOOST_CURRENT_FUNCTION << "-- " << e.what() << endl;
		return false;
	}

	return true;
}
