#include "stdafx.h"

//#define _READ_LOCAL
//#define _WRITE_LOCAL
//#define _DEBUG_BUF

#include "LogReceiverAdb.h"
#include "Resource.h"
#include "MainFrm.h"
#include "Settings.h"

#if defined(_WRITE_LOCAL) || defined(_READ_LOCAL) 
static FILE *testLogFile;
static FILE *testPsFile;
#endif

LogReceiverAdb gLogReceiverAdb;
static bool resetAtStart;

void LogReceiverAdb::start(bool reset)
{
	//logcat(0, 0);
	resetAtStart = reset;
	psThread.StartWork();
	//psThread.SetThreadPriority(THREAD_PRIORITY_HIGHEST);//THREAD_PRIORITY_LOWEST
	logcatLogThread.StartWork();
	//logcatLogThread.SetThreadPriority(THREAD_PRIORITY_HIGHEST);
}

void LogReceiverAdb::stop()
{
	logcatLogThread.StopWork();
	psThread.StopWork();
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
	void reset() { cb = pos = 0; buf[0] = 0; buf[buf_size];  }
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

static LOG_REC_ADB_DATA adbRec;

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

//#define _USE_FT
#ifdef _USE_FT
static LOG_PARCER ft("<~*~>"); //for flow trace data
static bool ParceFtData()
{
	LOG_REC_ADB_DATA rec;
	rec.reset();
	//Size Prefix: hC-single-byte character hd-short int
	int c = sscanf_s(ft.buf, "<~ %hhd %hhd %d %hd %hd %hd %d %u %u %d - ",
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
#endif //_USE_FT

static void AddTrace(size_t cbLog, char* szLog, int color)
{
	if (cbLog)
	{
		gLogReceiver.lock();
		adbRec.p_trace = szLog;
		adbRec.cb_trace = (WORD)cbLog;
		adbRec.len = sizeof(LOG_REC_ADB_DATA) + adbRec.cbData();
		adbRec.color = color;
		gArchive.append(&adbRec);
		gLogReceiver.unlock();
	}
}

int parceCollor(char** c);
static void TraceLog(const char* szLog, int cbLog)
{
	int color = 0;
	if (cbLog)
	{
		adbRec.log_flags |= LOG_FLAG_COLOR_PARCED;
		char *start = (char*)szLog;
		char *end = (char*)szLog;
		char *endLog = (char*)szLog + cbLog;
		while (end < endLog) {
			while (*(end) >= ' ') {
				end++;
				if (end - start >= MAX_LOG_LEN) {
					AddTrace(end - start, start, color);
					start = end;
				}
			}
			if (*end == '\n' || *end == '\r') {
				AddTrace(end - start + 1, start, color);
				while (*end == '\n' || *end == '\r')
					end++;
				start = end;
			}
			else if (*end == '\033' && *(end + 1) == '[') {
				int c1 = 0, c2 = 0, c3 = 0;
				char* colorPos = end;
				end += 2;
				c1 = parceCollor(&end);
				if (*end == ';') {
					end++;
					c2 = parceCollor(&end);
				}
				if (*end == ';') {
					end++;
					c3 = parceCollor(&end);
				}
				if (*end == 'm')
				{
					end++;
					if (!color) color = c1;
					if (!color) color = c2;
					if (!color) color = c3;
				}
				if (colorPos > start) {
					AddTrace(colorPos - start, start, color);
				}
				start = end;
			}
			else {
				if (*end == '\t') {
					*end = ' ';
				}
				else {
					*end = '*';
				}
				end++;
				if (end - start >= MAX_LOG_LEN) {
					AddTrace(end - start, start, color);
					start = end;
				}
			}
		}
		if (end > start)
		{
			AddTrace(end - start, start, color);
		}
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

#ifdef _USE_FT
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
#else 
	TraceLog(szLog, cbLog);
#endif //_USE_FT
}
// test on "\r\n[\r\n\x1b[0m\r\n\r\n[ 03-02 20:27:03.803 14375:14375 D/FLOW_TRACE ]\r\n"
// metadata example: [ 02-22 16:39:05.830   775:  865 D/TAG ]
static LOG_PARCER mt("\r\n[ *]\r\n"); //for adb metadara

static bool ParceMetaData()
{
	bool ok = true;
	int i = 0;
	int unused1 = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int unused2 = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int h = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int m = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int s = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int ms = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int pid = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	int tid = NextMetaDataDigit(mt.buf, mt.buf_size, i, ok);
	if (ok && pid && tid) {
		adbRec.reset();
		adbRec.pid = pid;
		adbRec.tid = tid;
		adbRec.sec = Helpers::GetSec(h , m, s);// 3600 * h + 60 * m + s;
		adbRec.msec = ms;
		int cb_fn_name = mt.cb - i - 5;
		if (i > 0 && cb_fn_name > 0 && cb_fn_name < MAX_JAVA_TAG_NAME_LEN - 2)
		{
			//if (0 != strstr(mt.buf + i + 1, "V/Lap"))
			//{
			//	i = i;
			//}
			memcpy(adbRec.tag, mt.buf + i + 1, cb_fn_name);
			while (cb_fn_name > 2 && adbRec.tag[cb_fn_name - 1] == ' ')
				cb_fn_name--;
			adbRec.tag[cb_fn_name] = ':';
			adbRec.tag[++cb_fn_name] = 0;
			adbRec.cb_fn_name = cb_fn_name;
			adbRec.p_fn_name = adbRec.tag;
			if (adbRec.tag[0] == 'E')
				adbRec.priority = FLOW_LOG_ERROR;
			else if (adbRec.tag[0] == 'I')
				adbRec.priority = FLOW_LOG_INFO;
			else if (adbRec.tag[0] == 'D')
				adbRec.priority = FLOW_LOG_DEBUG;
			else if (adbRec.tag[0] == 'W')
				adbRec.priority = FLOW_LOG_WARN;
		}
	}

	return ok;
}

static inline void ResetLog()
{
	mt.reset();
#ifdef _USE_FT
	ft.reset();
#endif //_USE_FT
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
	while (cbLog && gLogReceiver.working()) {
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
	return gLogReceiver.working();
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
static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:S" }; //hide FLOW_TRACE tag
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long" };
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
#endif

#if defined(_WRITE_LOCAL) || defined(_READ_LOCAL) 
	if (testLogFile)
		fclose(testLogFile);
	testLogFile = 0;
#endif
}


static PS_INFO psInfoTemp[maxPsInfo];
static int cPsInfoTemp;
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
	while (cbLog && (cPsInfoTemp < maxPsInfo) && gLogReceiver.working()) {
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
			while (c && name[0] == ' ')
			{
				name++;
				c--;
			}
			if (c && pid) {
				psInfoTemp[cPsInfoTemp].pid = pid;
				memcpy(psInfoTemp[cPsInfoTemp].name, name, c);
				psInfoTemp[cPsInfoTemp].name[c] = 0;
				psInfoTemp[cPsInfoTemp].cName = c;
				//stdlog("\x1 %d %s\n", pid, psInfoTemp[cPsInfoTemp].name);
				cPsInfoTemp++;
			}
			ps.reset();
		}
		szLog += end;
		cbLog -= end;
	}

	return (cPsInfoTemp < maxPsInfo) && gLogReceiver.working();
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
	cPsInfoTemp = 0;
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
	while (IsWorking()) {
		cPsInfoTemp = 0;
		ps.reset(); 
		adb_commandline(_countof(cmdAdbShell), cmdAdbShell, &streamCallback);//do this after LogcatLogThread::Work
		streamCallback.OnStdout("\r", 1);//end with new line
#if defined(_WRITE_LOCAL)
		break;
#endif
		if (gArchive.setPsInfo(psInfoTemp, cPsInfoTemp))
			gMainFrame->RedrawViews();
#if !defined(_WRITE_LOCAL)
		for (int i = 0; i < 5 && IsWorking(); i++)
			SleepThread(1000);
#endif
	}
#endif

#if defined(_WRITE_LOCAL) || defined(_READ_LOCAL) 
	if (!testPsFile)
		fclose(testPsFile);
	testPsFile = 0;
#endif
}
