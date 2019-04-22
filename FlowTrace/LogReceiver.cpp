#include "stdafx.h"
#include "LogReceiverAdb.h"
#include "LogReceiverNet.h"

LogReceiver gLogReceiver;
extern LogReceiverAdb gLogReceiverAdb;
extern LogReceiverNet gLogReceiverNet;

void LogReceiver::start(bool reset)
{
	m_working = true;
	gLogReceiverAdb.start(reset);
	gLogReceiverNet.start(reset);
}

void LogReceiver::stop()
{
	//lock();
	m_working = false;
	gLogReceiverAdb.stop();
	gLogReceiverNet.stop();
	//unlock();
}
