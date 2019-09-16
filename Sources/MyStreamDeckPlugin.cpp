//==============================================================================
/**
@file       MyStreamDeckPlugin.cpp

@brief      CPU plugin

@copyright  (c) 2018, Corsair Memory, Inc.
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================

#include "MyStreamDeckPlugin.h"
#include <atomic>
#include <map>

#ifdef __APPLE__
	#include "macOS/CpuUsageHelper.h"
#else
	#include "Windows/CpuUsageHelper.h"
#endif

#include "Common/ESDConnectionManager.h"

class CallBackTimer
{
public:
    CallBackTimer() :_execute(false) { }

    ~CallBackTimer()
    {
        if( _execute.load(std::memory_order_acquire) )
        {
            stop();
        };
    }

    void stop()
    {
        _execute.store(false, std::memory_order_release);
        if(_thd.joinable())
            _thd.join();
    }

    void start(int interval, std::function<void(void)> func)
    {
        if(_execute.load(std::memory_order_acquire))
        {
            stop();
        };
        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this, interval, func]()
        {
            while (_execute.load(std::memory_order_acquire))
            {
                func();
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            }
        });
    }

    bool is_running() const noexcept
    {
        return (_execute.load(std::memory_order_acquire) && _thd.joinable());
    }

private:
    std::atomic<bool> _execute;
    std::thread _thd;
};

MyStreamDeckPlugin::MyStreamDeckPlugin()
{
	mCpuUsageHelper = new CpuUsageHelper();
	mTimer = new CallBackTimer();
	mTimer->start(1000, [this]()
	{
		this->UpdateTimer();
	});
}

MyStreamDeckPlugin::~MyStreamDeckPlugin()
{
	if(mTimer != nullptr)
	{
		mTimer->stop();
		
		delete mTimer;
		mTimer = nullptr;
	}
	
	if(mCpuUsageHelper != nullptr)
	{
		delete mCpuUsageHelper;
		mCpuUsageHelper = nullptr;
	}
}

void MyStreamDeckPlugin::UpdateTimer()
{
	//
	// Warning: UpdateTimer() is running in the timer thread
	//
	if(mConnectionManager != nullptr)
	{
		mVisibleContextsMutex.lock();
		int currentValue = mCpuUsageHelper->GetCurrentCPUValue();
		for (const std::string& context : mVisibleContexts)
		{
			mConnectionManager->SetTitle(std::to_string(currentValue) + "%", context, kESDSDKTarget_HardwareAndSoftware);
		}
		mVisibleContextsMutex.unlock();
	}
}

std::vector<std::string> split(std::string string, char delimeter)
{
	std::stringstream ss(string);
	std::string item;
	std::vector<std::string> splittedStrings;
	while (std::getline(ss, item, delimeter))
	{
		splittedStrings.push_back(item);
	}
	return splittedStrings;
}

std::unordered_map<std::string, std::string> MyStreamDeckPlugin::run_client_query(SOCKET s, std::string command)
{
	// Add new line to command
	std::string command_send = command + '\n';

	send(s, command_send.c_str(), strlen(command_send.c_str()), 0);

	//Prevent connection shutdown until commands are executed
	//Sleep(100);

	long bytesRead = 1;
	const int buf_len = 1024;
	char recv_buffer[buf_len];
	std::string line;
	std::unordered_map<std::string, std::string> result;

	line = "";
	while (bytesRead > 0)
	{
		// -1 damit \0 ans Ende kann
		int bytesRead = recv(s, recv_buffer, buf_len - 1, 0);

		if (bytesRead == 0)
		{
			DebugPrint("Server hat die Verbindung getrennt..\n");
		}
		else if (bytesRead == SOCKET_ERROR)
		{
			DebugPrint("Fehler: recv, fehler code: %d\n", WSAGetLastError());
		}
		else if (bytesRead > 0)
		{
			//DebugPrint("\nServer antwortet mit %ld Bytes\n", bytesRead);

			for (int i = 0; i < bytesRead; i++)
			{
				//if(recv_buffer[i] == '\0')DebugPrint("Found 0");
				if (recv_buffer[i] == '\r')
				{
					//DebugPrint("Found: r\n");
				}
				else if (recv_buffer[i] == '\n')
				{
					//DebugPrint("Found: n\n");
					DebugPrint("Last Line: %s\n", line.c_str());

					if (line.substr(0, 5) == "error") {
						if (line.find("msg=ok"))
						{
							DebugPrint("success\n");
						}
						else
						{
							DebugPrint("fail\n");
						}

						return result;
					}
					else if (line.find("=") != std::string::npos)
					{
						std::vector<std::string> tokens = split(line, ' ');

						DebugPrint("Found %d tokens\n", tokens.size());

						for (auto const& line_part : tokens)
						{
							if (line_part.find("=") != std::string::npos)
							{
								std::vector<std::string> result_part = split(line_part, '=');

								if (result_part.size() != 2) {
									DebugPrint("result_part is not 2 it is %d\n", result_part.size());
									exit(1);
								}

								result[result_part[0]] = result_part[1];
							}
						}
					}

					line = "";
				}
				else
				{
					line += recv_buffer[i];
				}
				//if (recv_buffer[i] == '\r\n')DebugPrint("Found rn");
			}

			//recv_buffer[bytesRead] = '\0';
			//DebugPrint("Server antwortet: %s\n", recv_buffer);
			// do something with the bytes.  Note you cannot guarantee that the buffer contains a valid C string.
		}
	}

	//buf[rc] = '\0';
	//DebugPrint("\nServer antwortet: %s\n", buf);
}



void MyStreamDeckPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	DebugPrint("KeyDownForAction\n");
	DebugPrint("inAction: %s\n", inAction.c_str());
	//DebugPrint("inContext: %s\n", inContext.c_str());
	//DebugPrint("inDeviceID: %s\n", inDeviceID.c_str());
	//mConnectionManager->LogMessage("Test1");

	/*
	if(inAction == "de.kevinbirke.teamspeak.mute"){
		char clientupdate[256] = "clientupdate client_input_muted=1\n";
		send(s, clientupdate, strlen(clientupdate), 0);
	}

	if (inAction == "de.kevinbirke.teamspeak.unmute") {
		char clientupdate[256] = "clientupdate client_input_muted=0\n";
		send(s, clientupdate, strlen(clientupdate), 0);
	}
	*/

	WSADATA wsa;
	long rc;
	SOCKET s;
	SOCKADDR_IN addr;

	// Winsock starten
	rc = WSAStartup(MAKEWORD(2, 0), &wsa);
	if (rc != 0)
	{
		DebugPrint("Fehler: startWinsock, fehler code: %d\n", rc);
		return;
	}
	else
	{
		DebugPrint("Winsock gestartet!\n");
	}

	// Socket erstellen
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		DebugPrint("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n", WSAGetLastError());
		return;
	}
	else
	{
		DebugPrint("Socket erstellt!\n");
	}

	// Verbinden
	memset(&addr, 0, sizeof(SOCKADDR_IN)); // zuerst alles auf 0 setzten
	addr.sin_family = AF_INET;
	addr.sin_port = htons(25639);
	inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));

	rc = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));
	if (rc == SOCKET_ERROR)
	{
		DebugPrint("Fehler: connect gescheitert, fehler code: %d\n", WSAGetLastError());
		return;
	}
	else
	{
		DebugPrint("Verbunden mit 127.0.0.1..\n");
	}

	//
	//Aufruf run_command
	//
	std::unordered_map<std::string, std::string> result;
	result = run_client_query(s, "auth apikey=2I7E-OY65-MFW8-7ILV-7PXM-NE81");
	result = run_client_query(s, "whoami");

	DebugPrint("result clid: %s\n", result["clid"].c_str());

	closesocket(s);
	WSACleanup();
}

void MyStreamDeckPlugin::KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
	//DebugPrint("Test2");
	//mConnectionManager->LogMessage("Test2");
}

void MyStreamDeckPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remember the context
	mVisibleContextsMutex.lock();
	mVisibleContexts.insert(inContext);
	mVisibleContextsMutex.unlock();
}

void MyStreamDeckPlugin::WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remove the context
	mVisibleContextsMutex.lock();
	mVisibleContexts.erase(inContext);
	mVisibleContextsMutex.unlock();
}

void MyStreamDeckPlugin::DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo)
{
	// Nothing to do
}

void MyStreamDeckPlugin::DeviceDidDisconnect(const std::string& inDeviceID)
{
	// Nothing to do
}

void MyStreamDeckPlugin::SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
}
