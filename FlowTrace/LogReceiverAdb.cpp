#include "stdafx.h"

#ifdef _USE_ADB

//#define _READ_LOCAL
//#define _WRITE_LOCAL
//#define _DEBUG_BUF

#include "LogReceiverAdb.h"
#include "Resource.h"
#include "MainFrm.h"

#if defined(_WRITE_LOCAL) || defined(_READ_LOCAL) 
static FILE *testLogFile;
static FILE *testPsFile;
#endif
LogReceiverAdb gLogReceiverAdb;
LogReceiver* gpLogReceiver = &gLogReceiverAdb;
static bool resetAtStart;

void LogReceiverAdb::start(bool reset)
{
	//logcat(0, 0);
	resetAtStart = reset;
	m_working = true;
	logcatLogThread.StartWork();
	logcatLogThread.SetThreadPriority(THREAD_PRIORITY_HIGHEST);
	psThread.StartWork();
	psThread.SetThreadPriority(THREAD_PRIORITY_HIGHEST);//THREAD_PRIORITY_LOWEST
}

void LogReceiverAdb::stop()
{
	lock();
	m_working = false;
	logcatLogThread.StopWork();
	psThread.StopWork();
	unlock();
}

void PsThread::Terminate()
{
	streamCallback.Done(0);
}

void LogcatLogThread::Terminate()
{
	streamCallback.Done(0);
}


struct LOG_PARCER {
	LOG_PARCER(char* e) : expr(e), maxPos((int)strlen(e)) { reset(); }
	bool pending() { 
		return pos && pos < maxPos; 
	}
	bool compeated() { 
		return cb >= maxPos && !pending();
	}
	void reset() { cb = pos = 0; buf[0] = 0; }
	void parce(const char* szLog, int cbLog);

	static const int buf_size = 512;
	char buf[buf_size + 1];
	int pos, cb;
	const char* dataStart;
	const char* dataEnd;
private:
	const int maxPos;
	const char *expr;
};

void LOG_PARCER::parce(const char* szLog, int cbLog)
{
	dataStart = dataEnd = szLog;
	for (; dataEnd < (szLog + cbLog) && !compeated(); dataEnd++) {
		if (pos == 0 && expr[0] != '*') { 
			char * p = strchr((char*)dataEnd, expr[0]); //fast search for metadata start
			if (p) {
				dataEnd = p;
			}
		}
		char c = *dataEnd;
		if (c == expr[pos] || '*' == expr[pos]) {
			if (cb == 0) {
				dataStart = dataEnd;
			}
			if (cb < buf_size) {
				buf[cb++] = c;
				if (expr[pos] != '*') {
					pos++;
				}
				else if (expr[pos + 1] == c) {
					pos++;
					if (!compeated())
						pos++;
				}
			}
			else {
				ATLASSERT(false);
				reset();
				break;
			}
		}
		else {
			if (cb)
				dataEnd--;
			reset();
		}
	}
	buf[cb] = 0;
}

static LOG_PARCER ft("<~*~>"); //for flow trace data
static ROW_LOG_REC adbRec;

static inline int NextMetaDataDigit(char* buf, int buf_size, int &i, bool& ok)
{
	int ret = 0;
	if (ok) {
		while (buf[i] != 0 && !isdigit(buf[i]) && i < buf_size)
			i++;
		int i0 = i;
		ret = ok ? atoi(buf + i) : 0;
		while (isdigit(buf[i]) && i < buf_size)
			i++;
		ok = (i != i0);
	}
	return ret;
}

static bool ParceFtData()
{
	ROW_LOG_REC rec;
	rec.reset();
	//Size Prefix: hC-single-byte character hd-short int
	int c = sscanf_s(ft.buf, "<~ %hd %hd %d %hd %hd %hd %d %u %u %d - ",
		&rec.log_type, &rec.log_flags, &rec.nn, &rec.cb_app_name, &rec.cb_module_name,
		&rec.cb_fn_name, &rec.this_fn, &rec.call_site, &rec.fn_line, &rec.call_line);

	if (c != 10)
		return false;
	if (rec.cb_app_name < 0 || rec.cb_module_name < 0 || rec.cb_fn_name < 0)
		return false;
	if (rec.cb_app_name + rec.cb_module_name + rec.cb_fn_name > ft.cb - 20)
		return false;

	rec.p_fn_name = ft.buf + ft.cb - rec.cb_fn_name - 3;
	rec.p_module_name = rec.p_fn_name - rec.cb_module_name - 1;
	rec.p_app_name = rec.p_module_name - rec.cb_app_name - 1;

	rec.ftChecked = adbRec.ftChecked;
	rec.pid = adbRec.pid;
	rec.tid = adbRec.tid;
	rec.sec = adbRec.sec;
	rec.msec = adbRec.msec;
	adbRec = rec;

	return true;
}

static void TraceLog(const char* szLog, int cbLog)
{
	//TODO do not trace trailing \r\n in flow node
	if (cbLog) {
		adbRec.p_trace = szLog;
		adbRec.cb_trace = cbLog;
		adbRec.len = sizeof(ROW_LOG_REC) + adbRec.cbData();
		gArchive.append(&adbRec);
	}
}

static void WriteLog(const char* szLog, int cbLog)
{
	if (cbLog == 0)
		return;
	//TraceLog(szLog, cbLog);
	//return;
#if defined(_DEBUG_BUF) && defined(_DEBUG) 
	static char bbb[4000];
	memcpy(bbb, (void*)szLog, cbLog);
	bbb[cbLog] = 0;
	cbLog = cbLog;
#endif
	if ((!adbRec.ftChecked && adbRec.pid && adbRec.tid) || (ft.pending())) {
		if (!adbRec.ftChecked)
			ft.reset();
		adbRec.ftChecked = true;

		ft.parce(szLog, cbLog);
		int start = (int)(ft.dataStart - szLog);
		int end = (int)(ft.dataEnd - szLog);
		ATLASSERT(!ft.compeated() || start == 0);
		if (ft.compeated() && start == 0) {
			if (!ParceFtData()) {
				TraceLog(ft.buf, ft.cb);
			}
			TraceLog(szLog + end, cbLog - end);
		}
		else if (ft.pending() && start == 0) {//we have incompleate data at the end of szLog
			ATLASSERT(end == cbLog);
			return;
		}
		else {
			TraceLog(szLog, cbLog);
			ft.reset();
			adbRec.resetFT();
		}
	}
	else {
		TraceLog(szLog, cbLog);
	}
}
// test on "\r\n[\r\n\x1b[0m\r\n\r\n[ 03-02 20:27:03.803 14375:14375 D/FLOW_TRACE ]\r\n"
// metadata example: [ 02-22 16:39:05.830   775:  865 D/ft ]
static LOG_PARCER mt("\r\n[ *]\r\n"); //for adb metadara

static bool ParceMetaData()
{
	bool ok = true;
	int i = 0;
	ROW_LOG_REC rec;
	rec.reset();
	int unused1 = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int unused2 = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int h = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int m = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int s = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int ms = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int pid = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int tid = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	if (ok && pid && tid) {
		//replace cur log info
		rec.pid = pid;
		rec.tid = tid;
		rec.sec = 3600 * h + 60 * m + s;
		rec.msec = ms;
		adbRec = rec;
	}

	return ok;
}

static inline void ResetLog()
{
	mt.reset();
	ft.reset();
	adbRec.reset();
}

bool LogcatStreamCallback::HundleStream(const char* szLog, int cbLog)
{
	//static DWORD gdwLastPrintTick = 0;
	//int cbLog0 = cbLog;

	//stdlog("->%d\n", cbLog);
	//stdlog("\x1%s", szLog);
#ifdef _WRITE_LOCAL
	if (testLogFile)
		fwrite(szLog, cbLog, 1, testLogFile);
	return true;
#endif
	//return;
	while (cbLog && gLogReceiverAdb.working()) {
		mt.parce(szLog, cbLog);
		int start = (int)(mt.dataStart - szLog);
		int end = (int)(mt.dataEnd - szLog);
		if (mt.compeated()) {
			WriteLog(szLog, start);
			if (!ParceMetaData())
				WriteLog(mt.buf, mt.cb);
			mt.reset();
		}
		else if (mt.pos > 0) {//we have incompleate metadata at the end of szLog
			ATLASSERT(end == cbLog);
			WriteLog(szLog, start);
		}
		else {
			WriteLog(szLog, end);
		}
		szLog += end;
		cbLog -= end;
	}
	//DWORD dwTick = GetTickCount();
	//stdlog("<-%d - %d\n", cbLog0, dwTick-gdwLastPrintTick);
	//gdwLastPrintTick = dwTick;
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
//adb logcat -v long FlowTrace:D *:S
//adb logcat -v long -f d:\temp\log.txt
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:*", "*:S" };
static const char* cmdLogcatLog[]{ "logcat", "-v", "long" };
void LogcatLogThread::Work(LPVOID pWorkParam)
{
#if defined(_WRITE_LOCAL)
	if (!testLogFile) fopen_s(&testLogFile, "d:\\temp\\adbLog.txt", "wb");
#elif defined(_READ_LOCAL)
	if (!testLogFile) fopen_s(&testLogFile, "d:\\temp\\adbLog.txt", "rb");
#endif
#if defined(_READ_LOCAL)
	ResetLog();
	while (true) {
		int cb = (int)fread(streamCallback.GetBuf(), 1, streamCallback.GetBufSize(), testLogFile);
		if (cb <= 0)
			break;
		streamCallback.OnStdout(streamCallback.GetBuf(), cb);
	}	
#else
	if (resetAtStart) {
		resetAtStart = false;
		adb_commandline(_countof(cmdLogcatClear), cmdLogcatClear, &streamCallback);
	}
	while (IsWorking()) {
		ResetLog();
		adb_commandline(_countof(cmdLogcatLog), cmdLogcatLog, &streamCallback);
		SleepThread(1000);
	}
#endif

#if defined(_WRITE_LOCAL) || defined(_READ_LOCAL) 
	if (testLogFile)
		fclose(testLogFile);
	testLogFile = 0;
#endif
}


struct PS_INFO {
	int pid;
	char name[MAX_APP_NAME + 1];
	int cName;
};
static PS_INFO psInfo[512];
static int maxPsInfo = _countof(psInfo) - 1;
static int cPsInfo;
//USER     PID   PPID  VSIZE  RSS   PRIO  NICE  RTPRI SCHED   WCHAN    PC         NAME
static LOG_PARCER ps("\n*\r"); //captur one line
bool PsStreamCallback::HundleStream(const char* szLog, int cbLog)
{
#ifdef _WRITE_LOCAL
	if (testPsFile)
		fwrite(szLog, cbLog, 1, testPsFile);
	return true;
#endif
	//stdlog("->%d\n", cbLog);
	//stdlog("\x1%s", szLog);
	//return true;
	while (cbLog && (cPsInfo < maxPsInfo) && gLogReceiverAdb.working()) {
		ps.parce(szLog, cbLog);
		int start = (int)(ps.dataStart - szLog);
		int end = (int)(ps.dataEnd - szLog);
		if (ps.compeated()) {
			char* sz = ps.buf;
			int c = ps.cb - 1;
			sz[c] = 0;
			//stdlog("\x1%s\n", sz);
			while (sz[0] != ' ') { //reach to first space
				sz++;
				c--;
			}
			while (!isdigit(sz[0])) { //reach to first digit
				sz++;
				c--;
			}
			int pid = atoi(sz);
			//if (pid == 18325)
			//	pid = pid;
			while (c && sz[c] != ' ')
				c--;
			char* name = sz + c;
			c = std::min(MAX_APP_NAME, (int)strlen(name));
			if (c && pid) {
				psInfo[cPsInfo].pid = pid;
				memcpy(psInfo[cPsInfo].name, name, c);
				psInfo[cPsInfo].name[c] = 0;
				psInfo[cPsInfo].cName = c;
				//stdlog("\x1 %d %s\n", pid, psInfo[cPsInfo].name);
				cPsInfo++;
			}
			ps.reset();
		}
		szLog += end;
		cbLog -= end;
	}

	return (cPsInfo < maxPsInfo) && gLogReceiverAdb.working();
}

static const char* cmdAdbShell[]{ "cmd_shell", "ps" };
void PsThread::Work(LPVOID pWorkParam)
{
#if defined(_WRITE_LOCAL)
	if (!testPsFile) fopen_s(&testPsFile, "d:\\temp\\adbPs.txt", "wb");
#elif defined(_READ_LOCAL)
	if (!testPsFile) fopen_s(&testPsFile, "d:\\temp\\adbPs.txt", "rb");
#endif

#if defined(_READ_LOCAL)
	cPsInfo = 0;
	ps.reset();
	while (true) {
		char* buf = streamCallback.GetBuf();
		int cb = (int)fread(buf, 1, streamCallback.GetBufSize(), testPsFile);
		if (cb <= 0)
			break;
		buf[cb] = 0;
		streamCallback.OnStdout(streamCallback.GetBuf(), cb);
	}
#else
	while (true) {
#if !defined(_WRITE_LOCAL)
		SleepThread(5000);
#endif
		if (IsWorking()) {
			cPsInfo = 0;
			ps.reset();
			adb_commandline(_countof(cmdAdbShell), cmdAdbShell, &streamCallback);//do this after LogcatLogThread::Work
			streamCallback.OnStdout("\r", 1);//end with new line
#if defined(_WRITE_LOCAL)
			break;
#endif
			bool updateViews = false;
			
			while (IsWorking() && cPsInfo--) {
				if (gArchive.setAppName(psInfo[cPsInfo].pid, psInfo[cPsInfo].name, psInfo[cPsInfo].cName))
					updateViews = true;
			}
			if (IsWorking() && updateViews) {
				gMainFrame->RedrawViews();
			}
		}
		else {
			break;
		}
	}
#endif

#if defined(_WRITE_LOCAL) || defined(_READ_LOCAL) 
	if (!testPsFile)
		fclose(testPsFile);
	testPsFile = 0;
#endif
}

#endif //_USE_ADB