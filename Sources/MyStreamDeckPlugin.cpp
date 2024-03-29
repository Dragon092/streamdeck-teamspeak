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

#include "Common/ESDConnectionManager.h"
#include "Common/ESDUtilities.h"

#include "Vendor/cppcodec/cppcodec/base64_rfc4648.hpp"

MyStreamDeckPlugin::MyStreamDeckPlugin()
{
}

MyStreamDeckPlugin::~MyStreamDeckPlugin()
{
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



void MyStreamDeckPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json& inPayload, const std::string& inDeviceID)
{
	DebugPrint("KeyDownForAction\n");
	DebugPrint("inAction: %s\n", inAction.c_str());
	DebugPrint("inContext: %s\n", inContext.c_str());
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

	json settings = inPayload["settings"];

	WSADATA wsa;
	long rc;
	SOCKET s;
	SOCKADDR_IN addr;

	std::string new_icon = "";

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

	std::string apikey = settings.value("apikey", "");

	if (apikey == "") {
		DebugPrint("No API Key set");
		return;
	}

	//
	//Aufruf run_command
	//
	// Authenticate ClientQuery
	run_client_query(s, "auth apikey=" + apikey);

	// Get client id
	//TODO: Does not work if disconnected
	std::unordered_map<std::string, std::string> whoami_result = run_client_query(s, "whoami");
	//DebugPrint("result clid: %s\n", whoami_result["clid"].c_str());

	if (inAction == "de.kevinbirke.teamspeak.microphone") {
		std::string microphone_mode = settings.value("microphone_mode", "toggle");

		if (microphone_mode == "toggle") {
			std::unordered_map<std::string, std::string> clientvariable_result = run_client_query(s, "clientvariable clid=" + whoami_result["clid"] + " client_input_muted");
			//DebugPrint("result client_input_muted: %s\n", clientvariable_result["client_input_muted"].c_str());

			if (clientvariable_result["client_input_muted"] == "0")
			{
				run_client_query(s, "clientupdate client_input_muted=1");
				new_icon = "microphone_muted.png";
			}
			else if (clientvariable_result["client_input_muted"] == "1")
			{
				run_client_query(s, "clientupdate client_input_muted=0");
				new_icon = "microphone.png";
			}
		}
		else if (microphone_mode == "mute") {
			run_client_query(s, "clientupdate client_input_muted=1");
		}
		else if (microphone_mode == "unmute") {
			run_client_query(s, "clientupdate client_input_muted=0");
		}
	} else if (inAction == "de.kevinbirke.teamspeak.sound") {
		std::string sound_mode = settings.value("sound_mode", "toggle");

		if (sound_mode == "toggle") {
			std::unordered_map<std::string, std::string> clientvariable_result = run_client_query(s, "clientvariable clid=" + whoami_result["clid"] + " client_output_muted");
			//DebugPrint("result client_input_muted: %s\n", clientvariable_result["client_input_muted"].c_str());

			if (clientvariable_result["client_output_muted"] == "0")
			{
				run_client_query(s, "clientupdate client_output_muted=1");
				new_icon = "sound_muted.png";
			}
			else if (clientvariable_result["client_output_muted"] == "1")
			{
				run_client_query(s, "clientupdate client_output_muted=0");
				new_icon = "sound.png";
			}
		}
		else if (sound_mode == "mute") {
			run_client_query(s, "clientupdate client_output_muted=1");
		}
		else if (sound_mode == "unmute") {
			run_client_query(s, "clientupdate client_output_muted=0");
		}
	} else if (inAction == "de.kevinbirke.teamspeak.afk") {
		std::string afk_mode = settings.value("afk_mode", "toggle");

		if (afk_mode == "toggle") {
			std::unordered_map<std::string, std::string> clientvariable_result = run_client_query(s, "clientvariable clid=" + whoami_result["clid"] + " client_away");
			//DebugPrint("result client_input_muted: %s\n", clientvariable_result["client_input_muted"].c_str());

			if (clientvariable_result["client_away"] == "0")
			{
				run_client_query(s, "clientupdate client_away=1");
				new_icon = "afk_away.png";
			}
			else if (clientvariable_result["client_away"] == "1")
			{
				run_client_query(s, "clientupdate client_away=0");
				new_icon = "afk.png";
			}
		}
		else if (afk_mode == "away") {
			run_client_query(s, "clientupdate client_away=1");
		}
		else if (afk_mode == "back") {
			run_client_query(s, "clientupdate client_away=0");
		}
	}

	// Update the icon
	if(new_icon != ""){
		std::string pluginPath = ESDUtilities::GetPluginPath();
		std::string encodedFile;
		GetEncodedIconStringFromFile(ESDUtilities::AddPathComponent(pluginPath, "icons/" + new_icon), encodedFile);
		SetImage(encodedFile, inContext);
	}

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

void MyStreamDeckPlugin::DidReceiveSettings(const std::string& inAction, const std::string& inContext, const json& inPayload, const std::string& inDeviceID)
{
	DebugPrint("DidReceiveSettings\n");
	//DebugPrint("inPayload: %s\n", inPayload.dump().c_str());

	//DebugPrint("Done\n");
}

void MyStreamDeckPlugin::SetTitle(const std::string& inTitle, const std::string& inContext)
{
	if (mConnectionManager != nullptr)
		mConnectionManager->SetTitle(inTitle, inContext, kESDSDKTarget_HardwareAndSoftware);
}

void MyStreamDeckPlugin::SetImage(const std::string& inImage, const std::string& inContext)
{
	if (mConnectionManager != nullptr)
		mConnectionManager->SetImage(inImage, inContext, kESDSDKTarget_HardwareAndSoftware);
}

// Helper method, reads file into base64 encoded string
bool MyStreamDeckPlugin::GetEncodedIconStringFromFile(const std::string& inName, std::string& outFileString)
{
	bool success = false;
	std::ifstream pngFile(inName, std::ios::binary | std::ios::ate);
	if (pngFile.is_open())
	{
		std::ifstream::pos_type pos = pngFile.tellg();

		std::vector<char> result(pos);

		pngFile.seekg(0, std::ios::beg);
		pngFile.read(&result[0], pos);

		std::string base64encodedImage = cppcodec::base64_rfc4648::encode(&result[0], result.size());
		outFileString = base64encodedImage;
		success = true;
	}
	else
	{
		success = false;
		DebugPrint("Could not load icon: %s", inName.c_str());
	}
	return success;
}
