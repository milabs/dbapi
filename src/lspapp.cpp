#include "lspapp.hpp"
#include "lspapp_config.hpp"
#include "lspapp_module.hpp"
#include "lspapp_storage_dbox.hpp"
#include "lspapp_storage_local.hpp"
#include "lspapp_sync.hpp"

void usage(char * argv0, int code)
{
	fs::path program(argv0);

	const char * name = program.filename().c_str();

	printf("Usage: %s <action> [options...]\n", name);

	printf("\n");
	printf("       %s [--help]\n", name);
	printf("       %s run [--daemon]\n", name);
	printf("       %s config [--key=<key> --secret=<secret>] [--uuid=<uuid>]\n", name);
	printf("       %s status\n", name);
	printf("\n");
	printf("       * Use --config=<file> to specify the config file to use\n");
	printf("             --account=<account> to specify an account to use instead of default one\n");
	printf("\n");

	exit(code);
}

string parse_command_line(int argc, char * argv[], pt::ptree * tree)
{
	if (argc < 2)
		usage(argv[0], -1);

	tree->add("action", argv[1]);
#ifdef DEBUG
	cerr << "cmdline: action = " << argv[1] << endl;
#endif

	//
	// Преобразование командной строки в структуру ptree вида:
	// {
	//     "action" : argv[1],
	//     "params" : {
	//         "key" : "value",
	//         "key" : "value",
	//          ...     .....
	//     }
	// }
	//

	pt::ptree & params = tree->add_child("params", pt::ptree());
	for (int i = 2; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == '-') {
			vector<string> list;

			// skip '--'
			argv[i] += 2;

			boost::split(list, argv[i], boost::is_any_of("="));
			if (list.size() == 1) {
				params.push_back(make_pair(list[0], ""));
#ifdef DEBUG
				cerr << "cmdline: option = " << list[0] << endl;
#endif
				continue;
			}
			if (list.size() == 2) {
				params.push_back(make_pair(list[0], list[1]));
#ifdef DEBUG
				cerr << "cmdline: params = " << list[0] << " : " << list[1] << endl;
#endif
				continue;
			}
		}
		throw string("Invalid argument: ") + argv[i];
	}

	return tree->get("action", "help");
}

//
// Signal handler routines
//

void sig_handler(int signal)
{
}

void sig_init()
{
	sigset_t mask;
	struct sigaction action;

	/* block all the signals */
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	memset(&action, 0, sizeof(action));
	action.sa_handler = sig_handler;

	/* set signal handlers */
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);

	/* unblock some of the signals */
	sigdelset(&mask, SIGINT);
	sigdelset(&mask, SIGQUIT);
	sigdelset(&mask, SIGTERM);
	sigprocmask(SIG_SETMASK, &mask, 0);
}

void sig_cleanup()
{
	/* noop?.. */
}

void daemonize()
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
		exit(1);
	if (pid > 0)
		exit(0);

	setsid();
	if (chdir("/"))
		exit(1);
	umask(0);

	pid = fork();
	if (pid < 0)
		exit(1);
	if (pid > 0)
		exit(0);

	close(0);
	close(1);
	close(2);
}

// ==================== actions ==================== //

int action_run(CLspConfig & config)
{
	if (config.getChildAt("args").get("daemon", "false") != "false")
		daemonize();

	// объекты хранилища
	CLspStorageLocal localStorage;
	if (!localStorage.init(&config.getChildAt("")))
		throw string("localStorage::init faled");

	CLspStorageDropBox dropboxStorage;
	if (!dropboxStorage.init(&config.getChildAt("")))
		throw string("dropboxStorage::init failed");

	// менеджер синхронизации
	CLspSyncManager syncManager;

	syncManager.setSyncSrc(&localStorage);
	syncManager.setSyncDst(&dropboxStorage);

	// инициализация модулей
	vector<CLspModule *> modules;

	string modulePath = config.getChildAt("config").get("logpath", "default");
	BOOST_FOREACH(const pt::ptree::value_type & v, config.getChildAt("modules"))
	{
		CLspModule * m = new CLspModule(v.first, modulePath);

		if (!m->init(v.second))
			throw string("module::init failed");

		if (m->start(&syncManager))
			modules.push_back(m);
		else
			delete m;
	}

	syncManager.start();
	pause();
	syncManager.finish();

	while (!modules.empty()) {
		CLspModule * m = modules.back();
		m->finish();
		delete m;
		modules.pop_back();
	}

	return 0;
}

int action_config(CLspConfig & config)
{
	CLspStorageDropBox dropboxStorage;
	if (!dropboxStorage.init(&config.getChildAt("")))
		throw string("dropboxStorage::init failed");

	if (!config.save())
		throw string("Can't save the configuration");
	
	return 0;
}

int action_status(CLspConfig & config)
{
	CLspStorageDropBox dropboxStorage;
	if (!dropboxStorage.init(&config.getChildAt("")))
		throw string("dropboxStorage::init failed");

	if (!dropboxStorage.status())
		throw string("dropboxStorage::status failed");

	return 0;
}

// ==================== main ==================== //

int MAIN(int argc, char ** argv)
{
	pt::ptree options;

	// разбор командной строки
	string action =	parse_command_line(argc, argv, &options);
	const pt::ptree & args = options.get_child("params");

	// разбор кинфигурации
	CLspConfig config;
	if (!(config.init(args) && config.load()))
		throw string("Can't load the configuration");

	if (action == "run")
		return action_run(config);

	if (action == "config")
		return action_config(config);

	if (action == "status")
		return action_status(config);

	usage(argv[0], 0);
}

int main(int argc, char * argv[])
{
	int result = -1;

	try
	{
//		sig_init();
		result = MAIN(argc, argv);
//		sig_cleanup();
	}
	catch (string & s)
	{
		cerr << "ERROR: " << s << endl;
	}
	catch (exception & e)
	{
		cerr << "EXCEPTION: " << e.what() << endl;
	}
	catch (...)
	{
		cerr << "EXCEPTION: " << "unknown" << endl;
	}

	return result;
}
