#include "stdafx.h"
#include "Settings.h"
#include "LogReceiverAdb.h"
#include "LogReceiverNet.h"

LogReceiver gLogReceiver;
extern LogReceiverAdb gLogReceiverAdb;
extern LogReceiverNet gLogReceiverNet;

void LogReceiver::start(bool reset)
{
	m_working = true;
	gLogReceiverNet.start(reset);
	if (gSettings.GetUseAdb())
		gLogReceiverAdb.start(reset);
}

void LogReceiver::stop()
{
	//lock();
	m_working = false;
	gLogReceiverNet.stop();
	gLogReceiverAdb.stop();
	//unlock();
}

