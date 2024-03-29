//==============================================================================
/**
@file       MyStreamDeckPlugin.h

@brief      CPU plugin

@copyright  (c) 2018, Corsair Memory, Inc.
			This source code is licensed under the MIT-style license found in the LICENSE file.

**/
//==============================================================================

#include "Common/ESDBasePlugin.h"
#include <mutex>

class MyStreamDeckPlugin : public ESDBasePlugin
{
public:
	
	MyStreamDeckPlugin();
	virtual ~MyStreamDeckPlugin();
	
	void KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	
	void WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	
	void DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo) override;
	void DeviceDidDisconnect(const std::string& inDeviceID) override;
	
	void SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;

	void DidReceiveSettings(const std::string& inAction, const std::string& inContext, const json& inPayload, const std::string& inDeviceID) override;

	void SetTitle(const std::string& inTitle, const std::string& inContext);
	void SetImage(const std::string& inImage, const std::string& inContext);

private:
	std::mutex mVisibleContextsMutex;
	std::set<std::string> mVisibleContexts;

	std::unordered_map<std::string, std::string> run_client_query(SOCKET s, std::string command);

	static bool GetEncodedIconStringFromFile(const std::string& inName, std::string& outFileString);
};
