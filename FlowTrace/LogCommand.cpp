#include "stdafx.h"

#include "LogReceiverAdb.h"
#include "Settings.h"
#include "Helpers.h"

void LogCommand::intDevice()
{
	const CHAR* szAdbArg = gSettings.GetAdbArg();
	if (strchr(szAdbArg, ':')) {
		strncpy_s(szDeviceIpPort, szAdbArg, sizeof(szDeviceIpPort) - 1);
		szDeviceIpPort[sizeof(szDeviceIpPort) - 1] = 0;
		szDeviceID[0] = 0;
	}
	else {
		szDeviceIpPort[0] = 0;
		strncpy_s(szDeviceID, szAdbArg, sizeof(szDeviceID) - 1);
		szDeviceID[sizeof(szDeviceIpPort) - 1] = 0;
	}
}
void LogCommand::connectDevice()
{
	if (szDeviceIpPort[0])
	{
		char* p = cmd;
		size_t c = _countof(cmd);
		Helpers::strCpy(p, "connect ", c);
		Helpers::strCpy(p, szDeviceIpPort, c);
		adb_commandline(cmd, &dummyStreamCallback);
	}
}