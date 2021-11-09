#include "stdafx.h"
#include "LogReceiverAdb.h"
#include "Settings.h"
#include "Helpers.h"

void LogcatLogSupplier::Terminate()
{
	streamCallback.Done(0);
}

bool LogcatStreamCallback::HundleStream(char* szLog, int cbLog, bool isError)
{
	if (!isError)
		gLogReceiverAdb.HandleLogData(szLog, cbLog);
	return gLogReceiverAdb.working();
}

//tcp.port=5037
/*
adb logcat -g
/dev/log/main: ring buffer is 2048Kb (2047Kb consumed), max entry is 5120b, max payload is 4076b
/dev/log/system: ring buffer is 256Kb (92Kb consumed), max entry is 5120b, max payload is 4076b
*/
//adb logcat -G 512K or adb logcat -G 4M
//static const char* cmdLogcatClear = "logcat -c";
//static const char* cmdLogcatClear[]{ "logcat", "-b", "all", "-c" }; //adb logcat -b all -c
//static const char* cmdStartServer[]{ "start-server" };
//-v long: Display all metadata fields and separate messages with blank lines.
//adb logcat -v long FLOW_TRACE:* *:S
//adb logcat -v long FLOW_TRACE:S
//adb logcat -v long -f d:\temp\log.txt
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:*", "*:S" }; //show only FLOW_TRACE tag
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:S" }; //hide FLOW_TRACE tag
//static const char* cmdLogcatLog = "logcat -v long"; // adb logcat -v long
void LogcatLogSupplier::Work(LPVOID pWorkParam)
{
	resetAtStart = *((bool*)pWorkParam);

	intDevice();

	const CHAR* szLogcatArg = gSettings.GetLogcatArg();

	connectDevice();

	if (resetAtStart)
	{
		char* p = cmd;
		size_t c = _countof(cmd);
		if (szDeviceID[0])
		{
			Helpers::strCpy(p, szDeviceID, c);
			Helpers::strCpy(p, " ", c);
		}
		Helpers::strCpy(p, "logcat -c", c);
		adb_commandline(cmd, &dummyStreamCallback);
	}

	while (IsWorking()) {

		connectDevice();

		if (szLogcatArg[0])
		{
			char* p = cmd;
			size_t c = _countof(cmd);
			if (szDeviceID[0])
			{
				Helpers::strCpy(p, szDeviceID, c);
				Helpers::strCpy(p, " ", c);
			}
			Helpers::strCpy(p, "logcat ", c);
			Helpers::strCpy(p, szLogcatArg, c);
			adb_commandline(cmd, &dummyStreamCallback);
		}

		char* p = cmd;
		size_t c = _countof(cmd);
		if (szDeviceID[0])
		{
			Helpers::strCpy(p, szDeviceID, c);
			Helpers::strCpy(p, " ", c);
		}
		Helpers::strCpy(p, "logcat -v long", c);

		if (gSettings.GetApplyLogcutFilter())
		{
			StringList& stringList = gSettings.getAdbFilterList();
			for (int i = 0; i < stringList.getItemCount(); i++) {
				Helpers::strCpy(p, " ", c); 
				Helpers::strCpy(p, stringList.getItem(i), c);
			}
		}

		adb_commandline(cmd, &streamCallback);
		SleepThread(1000);
	}
}
