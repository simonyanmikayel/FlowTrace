#include "stdafx.h"
#include "LogReceiverAdb.h"
#include "Settings.h"

void LogcatLogSupplier::Terminate()
{
	streamCallback.Done(0);
}

void LogcatStreamCallback::GetStreamBuf(char** p, size_t* c)
{
#ifdef USE_RING_BUF
	gLogcatLogBuffer.GetLogBuffer(p, c);
#else
	* p = local_buffer;
	*c = sizeof(local_buffer) - 1;
#endif //USE_RING_BUF
}

bool LogcatStreamCallback::HundleStream(char* szLog, int cbLog)
{
#ifdef USE_RING_BUF
	gLogcatLogBuffer.AddLogData(szLog, cbLog);
#else
	gLogReceiverAdb.HandleLogData(szLog, cbLog);
#endif //USE_RING_BUF
	return gLogReceiverAdb.working();
}

//tcp.port=5037
/*
adb logcat -g
/dev/log/main: ring buffer is 2048Kb (2047Kb consumed), max entry is 5120b, max payload is 4076b
/dev/log/system: ring buffer is 256Kb (92Kb consumed), max entry is 5120b, max payload is 4076b
*/
//adb logcat -G 512K or adb logcat -G 4M
static const char* cmdLogcatClear[]{ "logcat", "-c" };
//static const char* cmdStartServer[]{ "start-server" };
//-v long: Display all metadata fields and separate messages with blank lines.
//adb logcat -v long FLOW_TRACE:* *:S
//adb logcat -v long FLOW_TRACE:S
//adb logcat -v long -f d:\temp\log.txt
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:*", "*:S" }; //show only FLOW_TRACE tag
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:S" }; //hide FLOW_TRACE tag
static const char* cmdLogcatLog[]{ "logcat", "-v", "long" }; // adb logcat -v long
void LogcatLogSupplier::Work(LPVOID pWorkParam)
{
	resetAtStart = *((bool*)pWorkParam);

	if (resetAtStart) {
		resetAtStart = false;
		adb_commandline(_countof(cmdLogcatClear), cmdLogcatClear, &streamCallback);
	}
	while (IsWorking()) {
		if (gSettings.GetApplyLogcutFilter())
		{
			StringList& stringList = gSettings.getAdbFilterList();
			const char* argv[128]{ 0 };
			int maxCmd = _countof(argv);
			int argc = 0;
			for (int i = 0; argc < maxCmd && i < _countof(cmdLogcatLog); i++, argc++) {
				argv[argc] = cmdLogcatLog[i];
			}
			for (int i = 0; argc < maxCmd && i < stringList.getItemCount(); i++, argc++) {
				argv[argc] = stringList.getItem(i);
			}
			adb_commandline(argc, argv, &streamCallback);
		}
		else
		{
			adb_commandline(_countof(cmdLogcatLog), cmdLogcatLog, &streamCallback);
		}
		SleepThread(1000);
	}
}
