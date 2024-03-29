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

#define LOGGER_ENTRY_MAX_LEN  (8*1024) // (4*1024) in android sources
static LogParcer mt(LOGGER_ENTRY_MAX_LEN);
static MetaDataInfo gMtInfo;
static int cMtInfo;
static bool logRestarted;
static LOG_REC_ADB_DATA logData;

void LogReceiverAdb::start(bool reset)
{
	if (!gSettings.GetUseAdb())
		return;

	m_working = true;
	mt.reset();
	gMtInfo.reset();
	cMtInfo = 0;
	logRestarted = false;
	gArchive.m_SkipedLogcat = 0;
	gAdbProp.restartAdbIfNeeded = gSettings.GetRestartAdb();
	logData.reset();
	logcatPsCommand.StartWork();
	logcatLogSupplier.StartWork(&reset);
}

void LogReceiverAdb::stop()
{
	m_working = false;
	logcatLogSupplier.StopWork(100);
	logcatPsCommand.StopWork(100);
}
/*
static void AddTrace(size_t cbLog, char* szLog, int color)
{
	if (cbLog)
	{
		gLogReceiver.lock();
		logData.p_trace = szLog;
		logData.cb_trace = (WORD)cbLog;
		logData.len = sizeof(LOG_REC_BASE_DATA) + logData.cbData();
		logData.color = color;
		gArchive.appendAdb(&logData);
		gLogReceiver.unlock();
	}
}
int parceCollor(char** c);
*/

static void TraceLog(const char* szLog, int cbLog)
{
	if (cbLog)
	{
		gLogReceiver.lock();
		logData.p_trace = szLog;
		logData.cb_trace = (WORD)cbLog;
		logData.len = sizeof(LOG_REC_BASE_DATA) + logData.cbData();
		logData.color = 0;
		gArchive.appendAdb(&logData);
		gLogReceiver.unlock();
	}
	/*
	int color = 0;
	if (cbLog)
	{
		logData.log_flags |= LOG_FLAG_COLOR_PARCED;
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
	*/
}

#define MAX_LOG_SIZE 4000
static char ft_log_buf[MAX_LOG_SIZE + 1];
static LOG_REC_NET_DATA* ft = (LOG_REC_NET_DATA*)ft_log_buf;

static void WriteLog(const char* szLog, int cbLog)
{
	//stdlog("%s\n", szLog);
	if (cbLog == 0)
		return;
	if (logRestarted)
		return;

	bool isFlowTraceLog = false;
	long long* pLL1 = (long long*)(logData.tag + 2);
	long long* pLL2 = pLL1 + 1;
	if (logData.cb_fn_name == 18 && *pLL1 == 4706917329019030598 && *pLL2 == 4201654279412335939) // && 0 == strncmp(adbRec.tag + 2, "FLOW_TRACE_INFO:", 16)
	{
		int cbHeader;
		int cf = sscanf_s(szLog, TEXT("%d~%d~%d~%hhd~%hd~%hd~%hd~%d~%hd~%d~%u~%u~%hhd~%hhd~%hhd~%u~%u~%d:"),
			&ft->nn, &ft->pid, &ft->tid, &ft->priority, &ft->cb_app_name, &ft->cb_module_name, &ft->cb_fn_name, &ft->fn_line, &ft->cb_trace, &ft->call_line, &ft->this_fn, &ft->call_site, &ft->log_type, &ft->log_flags, &ft->color, &ft->sec , &ft->msec, &cbHeader);

		if (ft->cb_fn_name > 65000)
			ft->cb_fn_name = 0;

		if (cf != 18)
		{
			ATLASSERT(false);
		}
		else if (cbHeader < 10 || cbHeader > 100 || *(szLog + cbHeader + 2) != ':')
		{
			ATLASSERT(false);
		}
		else if (cbHeader + 3 + ft->cb_app_name + ft->cb_module_name + ft->cb_fn_name > cbLog)
		{
			ATLASSERT(false);
		}
		else
		{
			if (cbHeader + 3 + ft->cb_app_name + ft->cb_module_name + ft->cb_fn_name + ft->cb_trace > cbLog)
				ft->cb_trace = cbLog - (cbHeader + 3 + ft->cb_app_name + ft->cb_module_name + ft->cb_fn_name);
			
			int c = ft->cbData();
			memcpy(ft->data, szLog + cbHeader + 3, c);
			ft->data[c] = 0;
			ft->len = sizeof(LOG_REC_BASE_DATA) + c;
			isFlowTraceLog = true;
		}
	}

	if (isFlowTraceLog)
	{
		ft->sec = logData.sec;
		ft->msec = logData.msec;
		logData.log_flags &= (~LOG_FLAG_ADB); //clear the bit
		gArchive.appendNet(ft);
	}
	else
	{
		TraceLog(szLog, cbLog);
	}
}

static bool ParceMetaData()
{
	if (mt.size() < 3 || mt.buf()[0] != '[' || mt.buf()[mt.size() - 2] != ']')
		return false;

	bool ok = true;
	MetaDataInfo mtInfo;
	mtInfo.size = mt.size();
	int i = 0;
	int unused1 = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	int unused2 = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	int h = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	int m = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	int s = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	int ms = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	mtInfo.pid = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	mtInfo.tid = Helpers::NextDigit(mt.buf(), mt.size(), i, ok);
	if (ok && mtInfo.pid && mtInfo.tid) {
		logData.reset();
		logData.pid = mtInfo.pid;
		logData.tid = mtInfo.tid;
		logData.sec = Helpers::GetSec(h, m, s); // 3600 * h + 60 * m + s;
		logData.msec = ms;
		mtInfo.totMsec = (long long)(logData.sec) * 1000L + logData.msec;
		int cb_fn_name = mt.size() - i - 1;
		if (i > 0 && cb_fn_name > 0 && cb_fn_name < MAX_JAVA_TAG_NAME_LEN - 2)
		{
			//if (0 != strstr(mt.buf + i + 1, "V/Lap")) 
			//{
			//	i = i;
			//}
			memcpy(logData.tag, mt.buf() + i + 1, cb_fn_name);
			logData.tag[cb_fn_name] = 0;
			while (cb_fn_name > 2 && (logData.tag[cb_fn_name - 1] <= ' ' || logData.tag[cb_fn_name - 1] == ']'))
				cb_fn_name--;
			logData.tag[cb_fn_name] = ':';
			logData.tag[++cb_fn_name] = 0;
			logData.cb_fn_name = cb_fn_name;
			logData.p_fn_name = logData.tag;
			if (logData.tag[0] == 'E')
				logData.priority = FLOW_LOG_ERROR;
			else if (logData.tag[0] == 'I')
				logData.priority = FLOW_LOG_INFO;
			else if (logData.tag[0] == 'D')
				logData.priority = FLOW_LOG_DEBUG;
			else if (logData.tag[0] == 'W')
				logData.priority = FLOW_LOG_WARN;
		}
	}

	if (logRestarted) {
		if (gMtInfo.totMsec < mtInfo.totMsec) {
			logRestarted = false;
			//stdlog("1 logRestarted = false: %s\n", mt.buf());
		}
		else if (gMtInfo.totMsec == mtInfo.totMsec) {
			if (cMtInfo) {
				cMtInfo--;
			}
			else {
				//stdlog("2 logRestarted = false: %s\n", mt.buf());
				logRestarted = false;
			}
		}
		if (!logRestarted) {
			gMtInfo.reset();
			gArchive.m_SkipedLogcat = 0;
		}
		else {
			gArchive.m_SkipedLogcat++;
			if (0 == gArchive.m_SkipedLogcat % 500)
				Helpers::UpdateStatusBar();
		}
	}
	if (!logRestarted) {
		if (gMtInfo.totMsec < mtInfo.totMsec) {
			gMtInfo = mtInfo;
			cMtInfo = 1;
		}
		else if (gMtInfo.totMsec == mtInfo.totMsec) {
			if (gMtInfo == mtInfo) {
				cMtInfo++;
			}
			else {
				gMtInfo = mtInfo;
				cMtInfo = 1;
			}
		}
		else {
			//stdlog("logRestarted = true: %s\n", mt.buf());
			logRestarted = true;
		}
	}

	//logRestarted = !(8358 == mtInfo.pid && 8378 == mtInfo.tid);
	return ok;
}

void LogReceiverAdb::HandleLogData(const char* szLog, size_t cbLog)
{
	//static DWORD gdwLastPrintTick = 0;
	//int cbLog0 = cbLog;

	//stdlog("->%d %s\n", cbLog, szLog);
	//stdlog("\x1%s", szLog);
#ifdef _WRITE_LOCAL
	if (testLogFile)
		fwrite(szLog, cbLog, 1, testLogFile);
	return true;
#endif
	//return;
	while (cbLog && gLogReceiver.working()) 
	{
		int skiped = mt.getLine(szLog, cbLog, true);
		//stdlog("HandleLogData->%d %d %d %s\n", mt.compleated(), skiped, mt.size(), mt.buf());
		if (mt.compleated())
		{
			if (!ParceMetaData())
			{
				WriteLog(mt.buf(), mt.size());
			}
			mt.reset();
		}

		szLog += skiped;
		cbLog -= skiped;
	}
}
