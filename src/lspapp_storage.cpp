#include "lspapp.hpp"
#include "lspapp_storage.hpp"

using namespace std;

void CLspStorage::cleanupWithTimeLimit(const string & path, const string & mask, uint32_t timeLimit)
{
	if (!timeLimit)
		return;

	pt::ptree * list = this->list(path, mask);
	if (!list)
		return;

	// старые в начале списка
	list->sort(compareByModifiedAscending);

	for (pt::ptree::iterator it = list->begin(); it != list->end(); ++it)
	{
		time_t timeValue = it->second.get("modified", 0);

		if (timeValue && (timeValue < time(NULL) - timeLimit)) {
#ifdef DEBUG
			cerr << "cleanup(time) " << it->second.get("path", "") << endl;
#endif
			delete this->remove(path + "/" + it->first);
		}
	}

	delete list;
}

void CLspStorage::cleanupWithSizeLimit(const string & path, const string & mask, uint32_t sizeLimit)
{
	if (!sizeLimit)
		return;

	pt::ptree * list = this->list(path, mask);
	if (!list)
		return;

	// старые в начале списка
	list->sort(compareByModifiedAscending);

	uint32_t total_size = 0;
	for (pt::ptree::iterator it = list->begin(); it != list->end(); ++it)
	{
		total_size += it->second.get("bytes", 0);
	}

	for (pt::ptree::iterator it = list->begin(); it != list->end(); ++it)
	{
		if (total_size < sizeLimit)
			break;
#ifdef DEBUG
		cerr << "cleanup(size) " << it->second.get("path", "") << endl;
#endif
		total_size -= it->second.get("bytes", 0);
		delete this->remove(path + "/" + it->first);
	}

	delete list;
}
