#include "TwitchPlaysAPI.h"
using namespace std;


mutex testMutex;

TwitchPlays::TwitchPlays(const char *filename): callbackRaw(NULL)
{
	commandHandler.AddCommand("msg", 2, &msgCommand);
	commandHandler.AddCommand("join", 1, &joinCommand);
	commandHandler.AddCommand("part", 1, &partCommand);
	commandHandler.AddCommand("ctcp", 2, &ctcpCommand);

	FILE *fi = fopen(filename, "r");
	if (fi == NULL)
	{
		fprintf(stderr, "* Error: Cannot open config file: %s\n", filename);
		return ;
	}
	char s[200], par[200], val[200];
	int len;
	while (fgets(s, 200, fi))
	{
		len = strlen(s);
		if (len == 0) continue;
		if (s[len - 1] == '\n') s[len - 1] = '\0';
		sscanf(s, "%s = %s", par, val);
		if (strcmp(par, "host") == 0)
		{
            
			host = new char[len + 1];
			strcpy(host, val);
		}
		else if (strcmp(par, "port") == 0)
			port = atoi(val);
		else if (strcmp(par, "nick") == 0)
			nick = val;
		else if (strcmp(par, "user") == 0)
			user = val;
		else if (strcmp(par, "password") == 0)
			password = val;
		else if (strcmp(par, "channel") == 0)
		{
			int i;
			len = strlen(val);
			for (i = 0; i < len; i++)
				val[i] = isalpha(val[i]) ? tolower(val[i]) : val[i];
			channel = val;
		}
	}
	printf("Finished reading config file: %s\n", filename);
}

bool TwitchPlays::start()
{
	client.Debug(false);
	curTwitch = this;
    
    thread t1(inputThread, &client), t2(TwitchListener, this);

    t1.join();
    cout << "t1" << endl;
    t2.join();
    cout << "t2" << endl;

    cout << "after my thread" << running << endl;

	while (!running) ; // still loading
	return true;
}

bool TwitchPlays::stop()
{
	running = false;
	return true;
}

void TwitchPlays::hookRaw(void (*cbRaw)(string, string))
{
	callbackRaw = client.callbackRaw = cbRaw;
}

void TwitchPlays::sendMessage(string msg)
{
	client.SendIRC("PRIVMSG #" + channel + " :" + msg);
}

void * TwitchListener(void *arg)
{
	TwitchPlays *twi = (TwitchPlays *)arg;
	IRCClient &irc = twi->client;
	if (irc.InitSocket())
	{
//		bool t = true;
        testMutex.lock();
		 std::cout << "Socket initialized. Connecting..." << std::endl;
        testMutex.unlock();
		if (irc.Connect(twi->host, twi->port))
		{
            testMutex.lock();
			 std::cout << "Connected. Loggin in..." << std::endl;
            testMutex.unlock();
			if (irc.Login(twi->nick, twi->user, twi->password))
			{
                testMutex.lock();
				 std::cout << "Logged in." << std::endl;
                testMutex.unlock();
				twi->running = true;
				signal(SIGINT, signalHandler);
                testMutex.lock();
				irc.SendIRC("JOIN #" + twi->channel);
                testMutex.unlock();
				while (irc.Connected() && twi->running)
				{
					cout << "getting data" << endl;
					irc.ReceiveData();
				}

			}

			if (irc.Connected())
				irc.Disconnect();

			;// std::cout << "Disconnected." << std::endl;
		}
	}
	return NULL;
}

void msgCommand(std::string arguments, IRCClient* client)
{
	std::string to = arguments.substr(0, arguments.find(" "));
	std::string text = arguments.substr(arguments.find(" ") + 1);

	std::cout << "To " + to + ": " + text << std::endl;
	client->SendIRC("PRIVMSG " + to + " :" + text);
};

void joinCommand(std::string channel, IRCClient* client)
{
	if (channel[0] != '#')
		channel = "#" + channel;

	client->SendIRC("JOIN " + channel);
}

void partCommand(std::string channel, IRCClient* client)
{
	if (channel[0] != '#')
		channel = "#" + channel;

	client->SendIRC("PART " + channel);
}

void ctcpCommand(std::string arguments, IRCClient* client)
{

	//std::string to = arguments.substr(0, arguments.find(" "));
	//std::string text = arguments.substr(arguments.find(" ") + 1);

	//std::transform(text.begin(), text.end(), text.begin(), towupper);

	client->SendIRC("CTCP");
}

void signalHandler(int signal)
{
	curTwitch -> running = false;
}

void * inputThread(void *client)
{
	std::string command;
    testMutex.lock();
	std::cout << "in this part" << std::endl;
    testMutex.unlock();
	while (true)
	{
		getline(std::cin, command);
		if (command == "")
			continue;

		if (command[0] == '/')
		{
			std::cout << "" << std::endl;
			commandHandler.ParseCommand(command, (IRCClient*)client);
		}
		else
			((IRCClient*)client)->SendIRC(command);

		if (command == "quit")
        {
            testMutex.lock();
            cout << "quiting" << endl;
            testMutex.unlock();
            break;
        }
	}
    cout << "at return"<< endl;
	//pthread_exit(NULL);
	return NULL;
}

bool ConsoleCommandHandler::AddCommand(std::string name, int argCount, void(*handler)(std::string /*params*/, IRCClient* /*client*/))
{
	CommandEntry entry;
	entry.handler = handler;
	std::transform(name.begin(), name.end(), name.begin(), towlower);
	_commands.insert(std::pair<std::string, CommandEntry>(name, entry));
	return true;
}

void ConsoleCommandHandler::ParseCommand(std::string command, IRCClient* client)
{
	if (_commands.empty())
	{
		std::cout << "No commands available." << std::endl;
		return;
	}

	if (command[0] == '/')
		command = command.substr(1); // Remove the slash

	std::string name = command.substr(0, command.find(" "));
	std::string args = command.substr(command.find(" ") + 1);
	int argCount = std::count(args.begin(), args.end(), ' ');

	std::transform(name.begin(), name.end(), name.begin(), towlower);

	std::map<std::string, CommandEntry>::const_iterator itr = _commands.find(name);
	if (itr == _commands.end())
	{
		std::cout << "Command not found." << std::endl;
		return;
	}

	if (++argCount < itr->second.argCount)
	{
		std::cout << "Insuficient arguments." << std::endl;
		return;
	}

	(*(itr->second.handler))(args, client);
}
