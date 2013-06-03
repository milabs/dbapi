#include "lspapp.hpp"
#include "lspapp_module.hpp"

using namespace std;

// ==================== CLspModule : ctors / dtors ==================== //

CLspModule::CLspModule() :
	mModuleName("dummy"),
	mModulePath("dummy")
{
	memset(&mModuleData, 0, sizeof(mModuleData));
}

CLspModule::CLspModule(const string & name, const fs::path & path) :
	mModuleName(name),
	mModulePath(path)
{
	memset(&mModuleData, 0, sizeof(mModuleData));
}

CLspModule::~CLspModule()
{
	NOT_IMPLEMENTED();
}

// ==================== CLspModule : public methods ==================== //

bool CLspModule::init(const pt::ptree & tree)
{
	mModulePath = mModulePath / mModuleName;

	readConfigTree(tree);

	for (lsp_module_t * pm = gStaticModules; pm->name != NULL; pm++) {
		if (boost::iequals(pm->name, mModuleName)) {
			mModuleData = *pm;
			break;
		}
	}

	if (!mModuleData.callback) {
		// TODO: dlopen
	}

	return true;
}

bool CLspModule::start(CLspSyncManager * syncManager)
{
	boost::system::error_code error;

	// всегда пытаться создать рабочий каталог
	fs::create_directories(mModulePath, error);
	if (!fs::exists(mModulePath, error))
		return false;

	syncManager->addSyncItem(mSyncItem);

	// отметиться
	journalMessage("started");

	mThread = boost::thread(&CLspModule::workThreadRoutine, this);

	return true;
}

bool CLspModule::finish()
{
	mThread.interrupt();
	mThread.join();
}

void CLspModule::journalMessage(const string & message)
{
	using namespace boost::posix_time;

	fs::path file = journalRotateCurrent();
	if (message.empty())
		return;

	mMutex.lock();
	ofstream ofs(file.string().c_str(), ios::app);
	if (ofs.is_open()) {
		char token[128];

		snprintf(token, sizeof(token), "[%s] %s: ",
			 to_simple_string(second_clock::local_time()).c_str(), mModuleName.c_str());

		vector<string> list;
		boost::split(list, message, boost::is_any_of("\n"));
		BOOST_FOREACH(const string & s, list) {
			if (s.empty())
				continue;
			ofs << token << s << endl;
		}

		ofs.flush();
		ofs.close();
	} else {
		cerr << "can't open file for logging -- " << file << endl; 
	}
	mMutex.unlock();
}

// ==================== CLspModule : private methods ==================== //

void CLspModule::readConfigTree(const pt::ptree & tree)
{
	mFreq = getTimeOrSize(tree.get("freq", "10s"));
	mRotateTime = getTimeOrSize(tree.get("rotate_time", "30m"));
	mRotateSize = getTimeOrSize(tree.get("rotate_size", "10K"));

	mSyncItem.si_name = mModuleName;
	mSyncItem.si_mask = "(.*\\.log)";
	mSyncItem.si_srcTimeLimit = getTimeOrSize(tree.get("local_time_limit", "1d"));
	mSyncItem.si_srcSizeLimit = getTimeOrSize(tree.get("local_size_limit", "10M"));
	mSyncItem.si_dstTimeLimit = getTimeOrSize(tree.get("remote_time_limit", "5d"));
	mSyncItem.si_dstSizeLimit = getTimeOrSize(tree.get("remote_size_limit", "50M"));

#ifdef DEBUG
	cerr << mModuleName << "@" << mModulePath.string() << endl;
	cerr << "  freq " << mFreq << endl;
	cerr << "  rotate_time " << mRotateTime << endl;
	cerr << "  rotate_size " << mRotateSize << endl;
	cerr << "  local_time_limit " << mSyncItem.si_srcTimeLimit << endl;
	cerr << "  local_size_limit " << mSyncItem.si_srcSizeLimit << endl;
	cerr << "  remote_time_limit " << mSyncItem.si_dstTimeLimit << endl;
	cerr << "  remote_size_limit " << mSyncItem.si_dstSizeLimit << endl;
#endif
}

fs::path CLspModule::journalRotateCurrent()
{
	char filename[256];

	try
	{
		// поиск текущего файла (*.cur) и его ротация пр необходимости
		fs::directory_iterator next(mModulePath), last;
		BOOST_FOREACH(const fs::path & item, make_pair(next, last)) {
			if (item.extension() == ".cur") {
				time_t current = time(NULL), from_time;

				if (sscanf(item.stem().c_str(), "%lu", &from_time) != 1)
					continue;

				if (fs::file_size(item) >= mRotateSize || from_time < (current - mRotateTime)) {
					struct tm tm;

					char to_string[64];
					char from_string[64];

					localtime_r(&from_time, &tm);
					strftime(from_string, 64, "%y%m%d_%H%M%S", &tm);

					localtime_r(&current, &tm);
					strftime(to_string, 64, "%y%m%d_%H%M%S", &tm);

					sprintf(filename, "%s__%s.log", from_string, to_string);
					fs::rename(item, mModulePath / filename);
					break;
				}

				return item;
			}
		}

	}
	catch (fs::filesystem_error & e)
	{
		cerr << e.what() << endl;
	}

	sprintf(filename, "%lu.cur", time(NULL));
	return mModulePath / filename;

}

void CLspModule::workThreadRoutine()
{
	using namespace boost;

	try
	{
		while (true) {
			journalMessage(mModuleData.callback(this));
			this_thread::sleep(posix_time::seconds(mFreq));
		}
	}
	catch (thread_interrupted & e)
	{
#ifdef DEBUG
		cerr << BOOST_CURRENT_FUNCTION << "-- interrupted" << endl;
#endif
	}
}

uint32_t CLspModule::getTimeOrSize(const string & str, const string & def) const
{
	uint8_t type;
	uint32_t value;

	// pure and simple
	if (sscanf(str.c_str(), "%d%c", &value, &type) != 2) {
		cerr << "incorrect value: " << str << endl;
		return getTimeOrSize(def);
	}

	switch (type) {
		// mega/kilo/byte
	case 'M':
		value *= 1024;
	case 'K':
		value *= 1024;
	case 'B':
		break;
		// week/days/hour/mins/secs
	case 'w':
		value *= 7;
	case 'd':
		value *= 24;
	case 'h':
		value *= 60;
	case 'm':
		value *= 60;
	case 's':
		break;
		// invalid
	default:
		value *= 0;
		break;
	}

	return value;
}

// ==================== static modules ==================== //

typedef struct {
	// this stands for User, Nice, System and Idle
	unsigned long long U, N, S, I;
} cpu_stat_t;

static string modCPU_callback(void * data)
{
	static vector<cpu_stat_t> prev;
	bool prev_empty = prev.empty();

	char cpuname[256];
	cpu_stat_t curr;

	string line;
	stringstream ss;

	ifstream file("/proc/stat");

	ss.precision(2);
	ss.setf(ios::fixed, ios::floatfield);
	for (int i = 0; getline(file, line); i++) {
		memset(&curr, 0, sizeof(curr));

		sscanf(line.c_str(),
		       "%s %llu %llu %llu %llu", cpuname,
		       &curr.U, &curr.N, &curr.S, &curr.I);

		if (strncmp(cpuname, "cpu", 3))
			break;
		if (strcmp(cpuname, "cpu") == 0)
			ss << "usage:";
		else
			ss << " " << cpuname << ":";

		if (prev_empty) {
			cpu_stat_t null = {0};
			prev.push_back(null);
		}

		unsigned long long d[4];
		d[0] = curr.U - prev[i].U;
		d[1] = curr.N - prev[i].N;
		d[2] = curr.S - prev[i].S;
		d[3] = curr.I - prev[i].I;
		unsigned long long sum = d[0] + d[1] + d[2] + d[3];

		prev[i] = curr;
		double usage = sum ? 100.0 * (double) (d[0] + d[1] + d[2]) / (double) sum : 0;

		ss << usage << "%";
	}
	file.close();

	return ss.str();
}

static string modMEM_callback(void * data)
{
	char memname[256];
	unsigned long value;

	string line;
	stringstream ss;

	ifstream file("/proc/meminfo");

	while (getline(file, line)) {
		value = 0;

		sscanf(line.c_str(),
		       "%s %lu", memname, &value);

		if (!(strcmp(memname, "MemTotal:") == 0 || strcmp(memname, "MemFree:") == 0))
			break;

		if (strcmp(memname, "MemFree:") == 0)
			ss << "Free:" << value << " ";
		if (strcmp(memname, "MemTotal:") == 0)
			ss << "Total:" << value << " ";
	}
	file.close();

	return ss.str();
}

typedef struct {
	unsigned long rx, tx;
} net_stat_t;

static string modNET_callback(void * data)
{
	static const char * rx_names[] =
		{
			"bytes",
			"packets",
			"errs",
			"drop",
			"fifo",
			"frame",
			"compressed",
			"multicast",
		};

	static const char * tx_names[] =
		{
			"bytes",
			"packets",
			"errs",
			"drop",
			"fifo",
			"colls",
			"carrier",
			"compressed",
		};
	static vector<net_stat_t> prev;
	bool prev_empty = prev.empty();

	char devname[256];
	unsigned long rx[8], tx[8];

	string line;
	stringstream ss;

	ifstream file("/proc/net/dev");

	getline(file, line);
	getline(file, line);

	ss.precision(2);
	ss.setf(ios::fixed, ios::floatfield);
	for (int i = 0; getline(file, line); i++) {
		memset(rx, 0, sizeof(rx));
		memset(tx, 0, sizeof(tx));

		sscanf(line.c_str(),
		       "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", devname,
		       &rx[0], &rx[1], &rx[2], &rx[3], &rx[4], &rx[5], &rx[6], &rx[7],
		       &tx[0], &tx[1], &tx[2], &tx[3], &tx[4], &tx[5], &tx[6], &tx[7]);

		if (prev_empty) {
			net_stat_t null = { rx[0], tx[0] };
			prev.push_back(null);
		}

		ss << devname << " ";

		double rx_rate = (rx[0] - prev[i].rx) / ((double) ((CLspModule *)data)->getFreq() * 1024);
		prev[i].rx = rx[0];
		ss << "(RX) rateKBs:" << rx_rate << " ";
		for (int j = 0; j < 8; j++) {
			if (j >= 2 && !rx[j])
				continue;
			ss << rx_names[j] << ":" << rx[j] << " ";
		}

		double tx_rate = (tx[0] - prev[i].tx) / ((double) ((CLspModule *)data)->getFreq() * 1024);
		prev[i].tx = tx[0];
		ss << "(TX) rateKBs:" << tx_rate << " ";
		for (int j = 0; j < 8; j++) {
			if (j >= 2 && !tx[j])
				continue;
			ss << tx_names[j] << ":" << tx[j] << " ";
		}

		ss << endl;
	}
	file.close();

	return ss.str();
}

//
// таблица статических модулей
//

lsp_module_t gStaticModules[] = {
	{ "CPU", modCPU_callback, (void *)-1 },
	{ "MEM", modMEM_callback, (void *)-1 },
	{ "NET", modNET_callback, (void *)-1 },
	{ 0 }
};
