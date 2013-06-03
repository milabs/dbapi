#ifndef __LSAPP_MODULE_HPP__
#define __LSAPP_MODULE_HPP__

#include "lspapp.hpp"
#include "lspapp_sync.hpp"

using namespace std;

// ==================== CLspModule ==================== //

class CLspModule
{
public:
	CLspModule();
	CLspModule(const string & name, const fs::path & path);
	~CLspModule();

	bool init(const pt::ptree & tree);

	bool start(CLspSyncManager * syncManager);
	bool finish();

	// добавление сообщения в журнал
	void journalMessage(const string & message);
	// принудительная ротация журнала
	fs::path journalRotateCurrent();

	uint32_t getFreq() const { return mFreq; }

private:
	void readConfigTree(const pt::ptree & tree);
	uint32_t getTimeOrSize(const string & str, const string & def = "0B") const;

	// имя модуля
	string mModuleName;
	fs::path mModulePath;
	// структура-описатель модуля
	lsp_module_t mModuleData;

	// частота
	uint32_t mFreq;
	// параметры ротации
	uint32_t mRotateTime, mRotateSize;
	// элемент синхронизации
	CLspSyncItem mSyncItem;;

	// параметры рабочего потока
	void workThreadRoutine();
	boost::mutex mMutex;
	boost::thread mThread;
};

#endif /* __LSPAPP_MODULE_HPP__ */
