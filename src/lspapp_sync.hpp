#ifndef __LSPAPP_SYNC_HPP__
#define __LSPAPP_SYNC_HPP__

#include "lspapp.hpp"
#include "lspapp_storage.hpp"

using namespace std;

// ========== [ CLspSyncItem : элемент синхронизации ] ========== //

struct CLspSyncItem
{
	CLspSyncItem() {}
	CLspSyncItem(const string & name, const string & mask = "(.*)") :
		si_name(name), si_mask(mask)
		{
			si_srcSizeLimit = 0, si_srcTimeLimit = 0;
			si_dstSizeLimit = 0, si_dstTimeLimit = 0;
		}

	// имя элемента (каталог)
	string si_name;
	// маска файлов (в каталоге)
	string si_mask;

	// лимиты в истчнике
	uint32_t si_srcSizeLimit;
	uint32_t si_srcTimeLimit;

	// лимиты в получателе
	uint32_t si_dstSizeLimit;
	uint32_t si_dstTimeLimit;
};

// ========== [ CLspSyncManager : менеждер синхронизации ] ========== //

class CLspSyncManager
{
public:
	CLspSyncManager();
	~CLspSyncManager();

	void setSyncSrc(CLspStorage * src);
	void setSyncDst(CLspStorage * dst);

	void addSyncItem(const CLspSyncItem & item);

	void start(int timeout = 60);
	void finish();

private:
	void syncOne(const CLspSyncItem & item);
	void syncItems();

	// Источник и приёиник инфрмации
	CLspStorage * mSyncSrc, * mSyncDst;

	// Список элементов синхронизации (каталогов)
	vector<CLspSyncItem> mSyncItems;

	// Рабочий поток
	void syncThreadRoutine(int timeout);
	boost::thread mThread;
};

#endif /* __LSPAPP_SYNC_HPP__ */
