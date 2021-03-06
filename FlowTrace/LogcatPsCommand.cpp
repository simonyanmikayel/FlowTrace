#include "stdafx.h"
#include "LogReceiverAdb.h"
#include "Settings.h"
#include "Helpers.h"

void LogcatPsCommand::Terminate()
{
	streamCallback.Done(0);
}

static int getDigit(char* &sz, int& c)
{
	c = c;
	while (c && sz[0] != ' ')
	{ //reach to first space
		sz++;
		c--;
	}
	c = c;
	while (c && !isdigit(sz[0]))
	{ //reach to first digit
		sz++;
		c--;
	}
	int ret = atoi(sz);
	return ret;
}

static PS_INFO psInfoTemp[maxPsInfo];
static int cPsInfoTemp;
//PID NAME
//static LOG_PARCER ps("\n*\r"); //captur one line
static LOG_PARCER ps(1024);
bool PsStreamCallback::HundleStream(char* szLog, int cbLog)
{
	//stdlog("->%d\n", cbLog);
	//stdlog("\x1->%s\n", szLog);
	//return true;
	szLog[cbLog] = 0;
	while (cbLog && (cPsInfoTemp < maxPsInfo) && gLogReceiver.working()) {
		int skiped = ps.getLine(szLog, cbLog, false);
		if (ps.compleated()) {
			char* sz = ps.buf();
			int c = ps.size();
			//stdlog("\x1size: %d, %s\n", ps.size(), sz);
			int pid = getDigit(sz, c);
			int ppid = getDigit(sz, c);
			while (c && sz[c - 1] != ' ')
				c--;
			char* name = sz + c;
			c = std::min(MAX_APP_NAME, (int)strlen(name));
			while (c && name[0] == ' ')
			{
				name++;
				c--;
			}
			if (c && pid)
			{
				psInfoTemp[cPsInfoTemp].pid = pid;
				psInfoTemp[cPsInfoTemp].ppid = ppid;
				memcpy(psInfoTemp[cPsInfoTemp].name, name, c);
				psInfoTemp[cPsInfoTemp].name[c] = 0;
				psInfoTemp[cPsInfoTemp].cName = c;
				//stdlog("\x1 [PID: %d %s]\n", pid, psInfoTemp[cPsInfoTemp].name);
				cPsInfoTemp++;
			}
			ps.reset();
		}
		szLog += skiped;
		cbLog -= skiped;
	}

	return (cPsInfoTemp < maxPsInfo) && gLogReceiver.working();
}

static const char* cmdAdbShell = "cmd_shell ps -t";
static char cmd[1204];

void LogcatPsCommand::Work(LPVOID pWorkParam)
{
	//return;
	int cPsInfoPrev = 0;
	while (IsWorking()) {
		cPsInfoTemp = 0;
		ps.reset();
		//stdlog("->adb shell ps\n");
		char* p = cmd;
		size_t c = _countof(cmd);

		Helpers::strCpy(p, gSettings.GetAdbArg(), c);
		Helpers::strCpy(p, " ", c);
		Helpers::strCpy(p, cmdAdbShell, c);
		adb_commandline(cmd, &streamCallback);//do this after LogcatLogSupplier::Work
		char b[] = { '\r', 0 };
		streamCallback.OnStdout(b, 1);//end with new line
		//return;
		if (gArchive.setPsInfo(psInfoTemp, cPsInfoTemp) || cPsInfoPrev != cPsInfoTemp)
			Helpers::RedrawViews();
		cPsInfoPrev = cPsInfoTemp;
		for (int i = 0; i < 5 && IsWorking(); i++)
			SleepThread(1000);
	}
}
