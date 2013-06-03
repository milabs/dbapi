#include "lspapp.hpp"
#include "lspapp_config.hpp"

using namespace std;

// ==================== CLspConfig ==================== //

CLspConfig::CLspConfig() :
	mConfigFile("config/config.json")
{
}

CLspConfig::CLspConfig(const CLspConfig & r) :
	mConfigFile(r.mConfigFile), mConfigTree(r.mConfigTree)
{
}

CLspConfig::CLspConfig(const fs::path & configFile)
{
	mConfigFile = configFile.has_filename() ?
		configFile : configFile / "config.json";
}

CLspConfig::~CLspConfig()
{
	NOT_IMPLEMENTED();
}

bool CLspConfig::init(const pt::ptree & args)
{
	fs::path configFile;

	configFile = args.get("config", "");
	if (!configFile.empty()) {
		mConfigFile = configFile.has_filename() ?
			configFile : configFile / "config.json";
	}
	mConfigTree.add_child("args", args);
#ifdef DEBUG
		cerr << "========== args ==========" << endl;
		pt::json_parser::write_json(cerr, mConfigTree.get_child("args"));
#endif
	return true;
}

bool CLspConfig::load(const fs::path & configFile)
{
	fs::path config(configFile);

	if (config.empty())
		config = mConfigFile;

	fs::path filename = config.filename();
	fs::path filepath = config.parent_path();

	try
	{
		fs::path path;

		//
		// load config
		//

		mConfigTree.add_child("config", pt::ptree());
		path = filepath / filename;
		pt::json_parser::read_json(path.string(),
					   mConfigTree.get_child("config"));
#ifdef DEBUG
		cerr << "========== config ==========" << endl;
		pt::json_parser::write_json(cerr, mConfigTree.get_child("config"));
#endif

		//
		// load account
		//

		mConfigTree.add_child("account", pt::ptree());
		path = filepath / "account" / mConfigTree.get("config.account", "default");
		if (path.filename() != "default")
			path.replace_extension(".json");
		pt::json_parser::read_json(path.string(),
					   mConfigTree.get_child("account"));
#ifdef DEBUG
		cerr << "========== account ==========" << endl;
		pt::json_parser::write_json(cerr, mConfigTree.get_child("account"));
#endif

		//
		// load modules
		//

		mConfigTree.add_child("modules", pt::ptree());
		path = filepath / "modules";
		BOOST_FOREACH(const pt::ptree::value_type & v,
			      mConfigTree.get_child("config.modules.active", pt::ptree()))
		{
			fs::path module(path);
			module /= "mod_" + v.second.data();
			module.replace_extension(".json");

			pt::ptree & child = mConfigTree.get_child("modules");
			child.push_back(make_pair(v.second.data(), pt::ptree()));
			pt::json_parser::read_json(module.string(),
						   child.get_child(v.second.data()));
		}
#ifdef DEBUG
		cerr << "========== modules ==========" << endl;
		pt::json_parser::write_json(cerr, mConfigTree.get_child("modules"));
#endif

	}
	catch (pt::ptree_error & e)
	{
		cerr << e.what() << endl;
		return false;
	}

	return true;
}

bool CLspConfig::save(const fs::path & configFile) const
{
	fs::path config(configFile);

	if (config.empty())
		config = mConfigFile;

	fs::path filename = config.filename();
	fs::path filepath = config.parent_path();

	try
	{
		fs::path path;

		//
		// save account
		//

		path = filepath / "account" / mConfigTree.get("config.account", "default");
		if (path.filename() != "default")
			path.replace_extension(".json");
		pt::json_parser::write_json(path.string(),
					    mConfigTree.get_child("account", pt::ptree()));

	}
	catch (pt::ptree_error & e)
	{
		cerr << e.what() << endl;
		return false;
	}

	return true;
}

pt::ptree & CLspConfig::getChildAt(const string & path)
{
	return mConfigTree.get_child(path, mConfigTreeStub);
}
