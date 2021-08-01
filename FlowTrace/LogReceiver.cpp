#include "stdafx.h"
#include "Settings.h"
#include "LogReceiverAdb.h"
#include "LogReceiverNet.h"
#include "LogReceiverSerial.h"
#include "LogRecorder.h"

LogReceiver gLogReceiver;
extern LogReceiverAdb gLogReceiverAdb;
extern LogReceiverNet gLogReceiverNet;
extern LogReceiverSerial gLogReceiverSerial;

void LogReceiver::start(bool reset)
{
	m_working = true;
	gLogReceiverNet.start();
	gLogReceiverAdb.start(reset);
	gLogReceiverSerial.start();
}

void LogReceiver::stop()
{
	//lock();
	m_working = false;
	gLogReceiverNet.stop();
	gLogReceiverAdb.stop();
	gLogReceiverSerial.stop();
	//unlock();
}
