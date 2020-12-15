#include "stdafx.h"
#include "LogReceiverAdb.h"

#ifdef USE_RING_BUF

void LogcatLogConsumer::Terminate()
{
}

void LogcatLogConsumer::Work(LPVOID pWorkParam)
{
	//stdlog("LogcatLogConsumer ->\n");
	while (IsWorking()) {
		size_t c = gLogcatLogBuffer.GetLogData(local_buffer, sizeof(local_buffer) - 1);
		if (c && gLogReceiverAdb.working())
		{
			local_buffer[c] = 0;
			//stdlog("GetLogData->%d %s\n", c, local_buffer);
			gLogReceiverAdb.HandleLogData(local_buffer, c);
		}
	}
}

#endif //USE_RING_BUF