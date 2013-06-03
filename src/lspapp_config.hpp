#ifndef __LSPAPP_CONFIG_HPP__
#define __LSPAPP_CONFIG_HPP__

#include "lspapp.hpp"

using namespace std;

// ==================== CLspConfig ==================== //

class CLspConfig
{
public:
	CLspConfig();
	CLspConfig(const CLspConfig & r);
	CLspConfig(const fs::path & configFile);
	~CLspConfig();

	bool init(const pt::ptree & args);
	bool load(const fs::path & configFile = "");
	bool save(const fs::path & configFile = "") const;

	pt::ptree & getChildAt(const string & path);
private:
	// файл конфигурации
	fs::path mConfigFile;

	// дерево конфигурации
	pt::ptree mConfigTree;
	pt::ptree mConfigTreeStub;
};

#endif /* __LSPAPP_CONFIG_HPP__ */
