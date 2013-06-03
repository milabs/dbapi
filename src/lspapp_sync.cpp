#include "lspapp.hpp"
#include "lspapp_sync.hpp"

// ==================== CLspSyncManager : public methods ==================== //

CLspSyncManager::CLspSyncManager()
{
}

CLspSyncManager::~CLspSyncManager()
{
	NOT_IMPLEMENTED();
}

void CLspSyncManager::setSyncSrc(CLspStorage * src)
{
	mSyncSrc = src;
}

void CLspSyncManager::setSyncDst(CLspStorage * dst)
{
	mSyncDst = dst;
}

void CLspSyncManager::addSyncItem(const CLspSyncItem & item)
{
	mSyncItems.push_back(item);
}

void CLspSyncManager::start(int timeout)
{
	mThread = boost::thread(&CLspSyncManager::syncThreadRoutine, this, timeout);
}

void CLspSyncManager::finish()
{
	mThread.interrupt();
	mThread.join();
}

// ==================== CLspSyncManager : private methods ==================== //

void CLspSyncManager::syncOne(const CLspSyncItem & item)
{
	pt::ptree * list = mSyncSrc->list(item.si_name, item.si_mask);
	if (!list)
		return;

	// старые в начале списка
	list->sort(CLspStorage::compareByModifiedAscending);

	uint32_t totalSize = 0;
	BOOST_FOREACH(pt::ptree::value_type & v, *list)
	{
		totalSize += v.second.get("bytes", 0);
	}

	if (!totalSize)
		goto return_delete;

#ifdef DEBUG
	cerr << "Intended to push " << totalSize << " bytes" << endl;
#endif
	// подготовить место в удалённом хранилище для загрузки
	mSyncDst->cleanupWithSizeLimit(item.si_name,
				       item.si_mask, item.si_dstSizeLimit + totalSize);

	BOOST_FOREACH(pt::ptree::value_type & v, *list)
	{
		string filename = item.si_name + "/" + v.first;
		fs::path tempfile = fs::temp_directory_path() / fs::unique_path();

		// TODO: storage-to-storage copy
		//
		//       Сейчас копирование между хранилищами происходит
		//       с использованием временных файлов, что не всегда
		//       удобно. Но пока это не критично.
		//

#ifdef DEBUG
		cerr << "sync " << filename << endl;
		cerr << "temp " << tempfile << endl;
#endif		

		pt::ptree * get = mSyncSrc->get_file(filename, tempfile);
		if (get) {
			pt::ptree * put = mSyncDst->put_file(tempfile, filename);
			if (put) {
				if (put->get("bytes", 0) == get->get("bytes", 0))
					delete mSyncSrc->remove(filename);
				else
					delete mSyncDst->remove(filename);
				delete put;
			}
			delete get;
		}

		if (fs::exists(tempfile))
			fs::remove(tempfile);
	}

return_delete:
	delete list;
}

void CLspSyncManager::syncItems()
{
	BOOST_FOREACH(const CLspSyncItem & v, mSyncItems)
	{
		// зачистка по времени
		mSyncSrc->cleanupWithTimeLimit(v.si_name, v.si_mask, v.si_srcTimeLimit);
		mSyncDst->cleanupWithTimeLimit(v.si_name, v.si_mask, v.si_dstTimeLimit);

		// синхронизация
		syncOne(v);

		// зачистка по размеру
		mSyncDst->cleanupWithSizeLimit(v.si_name, v.si_mask, v.si_dstSizeLimit);
		mSyncSrc->cleanupWithSizeLimit(v.si_name, v.si_mask, v.si_srcSizeLimit);
	}
}

void CLspSyncManager::syncThreadRoutine(int timeout)
{
	using namespace boost;

	try
	{
		while (true) {
			syncItems();
			this_thread::sleep(posix_time::seconds(timeout));
		}
	}
	catch (thread_interrupted & e)
	{
#ifdef DEBUG
		cerr << BOOST_CURRENT_FUNCTION << " -- interrupted" << endl;
#endif
	}
}
