#include "stdafx.h"
#include "Settings.h"
#include "LogReceiverSerial.h"
#include "Helpers.h"

LogReceiverSerial gLogReceiverSerial;

#define HANDLE_FLAG_OVERLAPPED 1
#define HANDLE_FLAG_IGNOREEOF 2
#define HANDLE_FLAG_UNITBUFFER 4

enum {
	SER_PAR_NONE, SER_PAR_ODD, SER_PAR_EVEN, SER_PAR_MARK, SER_PAR_SPACE
};
enum {
	SER_FLOW_NONE, SER_FLOW_XONXOFF, SER_FLOW_RTSCTS, SER_FLOW_DSRDTR
};

void LogReceiverSerial::start()
{
	if (gSettings.GetUseComPort_1())
		add(new SerialThread(gSettings.comPort_1));
	if (gSettings.GetUseComPort_2())
		add(new SerialThread(gSettings.comPort_2));
}

void LogReceiverSerial::stop()
{
	for (int i = 0; i < cThreads; i++)
	{
		pThreads[i]->StopWork();
		delete pThreads[i];
	}
	cThreads = 0;
}

void LogReceiverSerial::add(SerialThread* pThread)
{
	if (gLogReceiver.working() && (cThreads < _countof(pThreads)))
	{
		pThreads[cThreads++] = pThread;
		pThread->dummyPid = gLogReceiver.getDummyPid();
		pThread->StartWork();
	}
	else
	{
		delete pThread;
	}
}

void SerialThread::serial_close()
{
	if (serport != INVALID_HANDLE_VALUE)
	{
		HANDLE h = serport;
		serport = INVALID_HANDLE_VALUE;
		CloseHandle(h);
	}
}

void SerialThread::Terminate()
{
	serial_close();
}

char* SerialThread::serial_configure()
{
	DCB dcb;
	COMMTIMEOUTS timeouts;

	/*
	 * Set up the serial port parameters. If we can't even
	 * GetCommState, we ignore the problem on the grounds that the
	 * user might have pointed us at some other type of two-way
	 * device instead of a serial port.
	 */
	if (GetCommState(serport, &dcb)) {
		/*
		 * Boilerplate.
		 */
		dcb.fBinary = TRUE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fTXContinueOnXoff = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fErrorChar = FALSE;
		dcb.fNull = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		dcb.fAbortOnError = FALSE;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;

		/*
		 * Configurable parameters.
		 */
		dcb.BaudRate = m_comPort.GetSpeed();
		dcb.ByteSize = m_comPort.GetDataBits();

		switch (m_comPort.GetStopBits() * 2) {
		case 2: dcb.StopBits = ONESTOPBIT; break;
		case 3: dcb.StopBits = ONE5STOPBITS; break;
		case 4: dcb.StopBits = TWOSTOPBITS; break;
		default: return "Invalid number of stop bits (need 1, 1.5 or 2)";
		}

		switch (m_comPort.GetParity()) {
		case SER_PAR_NONE: dcb.Parity = NOPARITY; break;
		case SER_PAR_ODD: dcb.Parity = ODDPARITY; break;
		case SER_PAR_EVEN: dcb.Parity = EVENPARITY; break;
		case SER_PAR_MARK: dcb.Parity = MARKPARITY; break;
		case SER_PAR_SPACE: dcb.Parity = SPACEPARITY; break;
		default: return "Invalid parity";
		}

		switch (m_comPort.GetFlowControl()) {
		case SER_FLOW_NONE: break;
		case SER_FLOW_XONXOFF: dcb.fOutX = dcb.fInX = TRUE; break;
		case SER_FLOW_RTSCTS: dcb.fRtsControl = RTS_CONTROL_HANDSHAKE; dcb.fOutxCtsFlow = TRUE; break;
		case SER_FLOW_DSRDTR: dcb.fDtrControl = DTR_CONTROL_HANDSHAKE; dcb.fOutxDsrFlow = TRUE; break;
		default: return "Invalid Flow Control";
		}

		if (!SetCommState(serport, &dcb))
			return "Unable to configure serial port";

		timeouts.ReadIntervalTimeout = 1;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;
		if (!SetCommTimeouts(serport, &timeouts))
			return "Unable to configure serial timeouts";
	}

	return NULL;
}

char* SerialThread::serial_open()
{

	const char* portName = m_comPort.GetName();
	if (portName[0] == 0)
		return "port does not specified";

	CString serfilename;
	serfilename.Format("%s%s", strchr(m_comPort.GetName(), '\\') ? "" : "\\\\.\\", m_comPort.GetName());
	serport = CreateFile(serfilename.GetBuffer(), GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (serport == INVALID_HANDLE_VALUE)
	{
		return "Can not open the port";
	}

	char* err = serial_configure();
	if (err)
	{
		return err;
	}

	return NULL;
}

SerialThread::SerialThread(ComPort& comPort) : m_comPort(comPort), logParcer(4096)
{
	serport = INVALID_HANDLE_VALUE;
	flags = HANDLE_FLAG_OVERLAPPED | HANDLE_FLAG_IGNOREEOF;// | HANDLE_FLAG_UNITBUFFER;
	logData.reset(m_comPort.GetName());
}

void SerialThread::TraceLog(const char* szLog, int cbLog)
{
	if (cbLog)
	{
		gLogReceiver.lock();
		logData.p_trace = szLog;
		logData.cb_trace = (WORD)cbLog;
		logData.len = sizeof(LOG_REC_BASE_DATA) + logData.cbData();
		logData.color = 0;
		gArchive.appendSerial(&logData);
		gLogReceiver.unlock();
	}
}

void SerialThread::Work(LPVOID pWorkParam)
{
	HANDLE oev = NULL;
	OVERLAPPED ovl, * povl;
	povl = &ovl;

	int readret, readlen;

	if (flags & HANDLE_FLAG_OVERLAPPED) {
		povl = &ovl;
		oev = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	else {
		povl = NULL;
	}

	if (flags & HANDLE_FLAG_UNITBUFFER)
		readlen = 1;
	else
		readlen = sizeof(buffer) - 1;

	while (IsWorking())
	{
		if (serport == INVALID_HANDLE_VALUE)
		{
			char* err = serial_open();
			if (serport == INVALID_HANDLE_VALUE)
			{
				SleepThread(1000);
				continue;
			}
		}
		if (povl) {
			memset(povl, 0, sizeof(OVERLAPPED));
			povl->hEvent = oev;
		}
		readret = ReadFile(serport, buffer, readlen, &len, povl);
		if (!readret)
			readerr = GetLastError();
		else
			readerr = 0;
		if (povl && !readret && readerr == ERROR_IO_PENDING) {
			WaitForSingleObject(povl->hEvent, INFINITE);
			readret = GetOverlappedResult(serport, povl, &len, FALSE);
			if (!readret)
				readerr = GetLastError();
			else
				readerr = 0;
		}

		if (!readret) {
			/*
			 * Windows apparently sends ERROR_BROKEN_PIPE when a
			 * pipe we're reading from is closed normally from the
			 * writing end. This is ludicrous; if that situation
			 * isn't a natural EOF, _nothing_ is. So if we get that
			 * particular error, we pretend it's EOF.
			 */
			if (readerr == ERROR_BROKEN_PIPE)
				readerr = 0;
			len = 0;
		}

		if (readret && len == 0 &&
			(flags & HANDLE_FLAG_IGNOREEOF))
			continue;

		// If we just set len to 0, that means the read operation has returned end-of-file.
		if (len == 0)
		{
			serial_close();
			continue;
		}

		buffer[len] = 0;
		char* szLog = buffer;
		int cbLog = len;

		logData.pid = dummyPid;
		logData.tid = dummyPid;
		DWORD sec, msec;
		Helpers::GetTime(sec, msec);
		logData.sec = sec;
		logData.msec = msec;
		//stdlog("sec: %d msec %d size: %d ends: %d %s \n", sec, msec, cbLog, szLog[cbLog - 1], szLog);
		TraceLog(szLog, cbLog);

		//while (cbLog && IsWorking()) {
		//	int skiped = logParcer.getLine(szLog, cbLog, false);
		//	if (logParcer.compleated()) {
		//		char* sz = logParcer.buf();
		//		int c = logParcer.size();
		//		stdlog("\x1size: %d, %s\n", c, sz);
		//		TraceLog(sz, c);
		//		logParcer.reset();
		//	}
		//	szLog += skiped;
		//	cbLog -= skiped;
		//}

	}

	if (povl)
		CloseHandle(oev);

	Terminate();
}