#ifndef __LSPAPP_STORAGE_HPP__
#define __LSPAPP_STORAGE_HPP__

#include "lspapp.hpp"

using namespace std;

// ==================== ILspStorage : interface ==================== //

class ILspStorage
{
public:
	virtual ~ILspStorage() {}

	//
	// инициализация
	//

	virtual bool init(pt::ptree * tree) = 0;
	virtual bool status() = 0;

	//
	// методы работы с объектами хранилища
	//
		
	virtual pt::ptree * stat(const string & path) = 0;
	virtual pt::ptree * list(const string & path, const string & mask = "(.*)") = 0;

	virtual pt::ptree * remove(const string & path) = 0;
	virtual pt::ptree * rename(const string & old_path, const string & new_path) = 0;

	virtual pt::ptree * create_directory(const string & path) = 0;
	virtual pt::ptree * remove_directory(const string & path) = 0;

	//
	// методы работы с объектами хранилища и файловой системы
	//

	virtual pt::ptree * get_file(const string & path, const fs::path & file) = 0;
	virtual pt::ptree * put_file(const fs::path & file, const string & path) = 0;
};

// ==================== CLspStorage : generic class ==================== //

class CLspStorage : virtual public ILspStorage
{
public:
	void cleanupWithTimeLimit(const string & path, const string & mask, uint32_t timeLimit);
	void cleanupWithSizeLimit(const string & path, const string & nask, uint32_t sizeLimit);

	static bool compareByModifiedDescending(const pt::ptree::value_type & v1,
						const pt::ptree::value_type & v2)
		{
			time_t timeValue1, timeValue2;

			timeValue1 = v1.second.get("modified", 0);
			timeValue2 = v2.second.get("modified", 0);

			// большее время в начале
			return timeValue1 > timeValue2;
		}
	static bool compareByModifiedAscending(const pt::ptree::value_type & v1,
					       const pt::ptree::value_type & v2)
		{
			time_t timeValue1, timeValue2;

			timeValue1 = v1.second.get("modified", 0);
			timeValue2 = v2.second.get("modified", 0);

			// меньшее время в начале
			return timeValue1 < timeValue2;
		}
};

#endif /* __LSPAPP_STORAGE_HPP__ */
