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
	//reach to first space
	c = c;
	while (c && sz[0] != ' ')
	{
		sz++;
		c--;
	}
	//reach to first digit
	c = c;
	while (c && !isdigit(sz[0]))
	{
		sz++;
		c--;
	}
	int ret = atoi(sz);
	return ret;
}

static char* getWord(char*& sz, int& c)
{
	char* ret = sz;
	//skip white
	c = c;
	while (c && sz[0] <= ' ')
	{ 
		ret++;
		sz++;
		c--;
	}
	//skip none white
	c = c;
	while (c && sz[0] > ' ')
	{ //reach to first digit
		sz++;
		c--;
	}
	return ret;
}

static PS_INFO psInfoTemp[maxPsInfo];
static int cPsInfoTemp;
static int firstLine = true;
static bool haveTID = false;
static int pidPos;
static int ppidPos;
static int tidPos;
static int namePos;
static int cmdPos; // thread name
//PID NAME
//static LogParcer ps("\n*\r"); //captur one line
static LogParcer ps(1024);
bool PsStreamCallback::HundleStream(char* szLog, int cbLog, bool isError)
{
	if (isError)
		return false;
	//stdlog("\x1->%s\n", szLog);
	//return true;
	bool ret = true;
	while (ret && cbLog && (cPsInfoTemp < maxPsInfo) && gLogReceiver.working()) {
		int skiped = ps.getLine(szLog, cbLog, false);
		if (ps.compleated()) {
			char* word;
			char* sz = ps.buf();
			int c = ps.size();
			int pos = 0;
			if (firstLine) {
				// first line is one of
				// USER     PID   PPID  VSIZE  RSS     WCHAN    PC         NAME
				// USER     PID   TID   PPID   VSZ  RSS WCHAN   ADDR     S CMD
				firstLine = false;
				pidPos = tidPos = namePos = cmdPos = ppidPos = -1;
				do {
					word = getWord(sz, c);
					if (0 == strncmp("PID", word, 3))
						pidPos = pos;
					else if (0 == strncmp("TID", word, 3))
						tidPos = pos;
					else if (0 == strncmp("PPID", word, 4))
						ppidPos = pos;
					else if (0 == strncmp("NAME", word, 4))
						namePos = pos;
					else if (0 == strncmp("CMD", word, 3))
						cmdPos = pos;
					pos++;
				} while (c);
				haveTID = (tidPos >= 0);
				if (namePos >= 0 && cmdPos == -1)
					namePos++; //on valina the header does not contain S (state)

				ret = (pidPos >= 0 && namePos >= 0);
			}
			else {
				psInfoTemp[cPsInfoTemp].reset();
				do {
					word = getWord(sz, c);
					if (pidPos == pos)
						psInfoTemp[cPsInfoTemp].pid = atoi(word);
					else if (tidPos == pos)
						psInfoTemp[cPsInfoTemp].tid = atoi(word);
					else if (ppidPos == pos)
						psInfoTemp[cPsInfoTemp].ppid = atoi(word);
					else if (namePos == pos) {
						psInfoTemp[cPsInfoTemp].cName = std::min((int)MAX_APP_NAME, (int)(sz - word));
						memcpy(psInfoTemp[cPsInfoTemp].name, word, psInfoTemp[cPsInfoTemp].cName);
					}
					else if (cmdPos == pos) {
						psInfoTemp[cPsInfoTemp].cCmd = std::min((int)MAX_APP_NAME, (int)(sz - word));
						memcpy(psInfoTemp[cPsInfoTemp].cmd, word, psInfoTemp[cPsInfoTemp].cCmd);
					}
					pos++;
				} while (c);
				if (!psInfoTemp[cPsInfoTemp].cCmd) {
					psInfoTemp[cPsInfoTemp].cCmd = psInfoTemp[cPsInfoTemp].cName;
					memcpy(psInfoTemp[cPsInfoTemp].cmd, psInfoTemp[cPsInfoTemp].name, psInfoTemp[cPsInfoTemp].cCmd);
				}
				ret = (psInfoTemp[cPsInfoTemp].cName > 0 && psInfoTemp[cPsInfoTemp].pid > 0);
				if (ret)
					cPsInfoTemp++;
			}
			ps.reset();
		}
		szLog += skiped;
		cbLog -= skiped;
	}

	return ret && (cPsInfoTemp < maxPsInfo) && gLogReceiver.working();
}

static const char* cmdPs[] = { "cmd_shell ps -t", "cmd_shell ps -A -o PID,TID,NAME,CMD" };
static bool cmdPsIsOk[] = { true, true };
static int cmdNom = 0;
static char cmd[1204];

void LogcatPsCommand::Work(LPVOID pWorkParam)
{
	//return;
	int cPsInfoPrev = 0;
	while (IsWorking()) {
		cPsInfoTemp = 0;
		firstLine = true;
		ps.reset();
		//stdlog("->adb shell ps\n");
		char* p = cmd;
		size_t c = _countof(cmd);
		size_t cmdCount = _countof(cmdPs);

		if (!cmdPsIsOk[cmdNom])
			cmdNom++;
		if (cmdNom >= cmdCount)
			cmdNom = 0;

		Helpers::strCpy(p, gSettings.GetAdbArg(), c);
		Helpers::strCpy(p, " ", c);
		Helpers::strCpy(p, cmdPs[cmdNom], c);
		adb_commandline(cmd, &streamCallback);//do this after LogcatLogSupplier::Work
		char b[] = { '\r', 0 };
		streamCallback.OnStdout(b, 1);//end with new line
		//return;
		if (gArchive.setPsInfo(psInfoTemp, cPsInfoTemp, haveTID) || cPsInfoPrev != cPsInfoTemp)
			Helpers::RedrawViews();
		cPsInfoPrev = cPsInfoTemp;
		
		cmdPsIsOk[cmdNom] = (cPsInfoPrev > 2);
		for (int i = 0; i < (cmdPsIsOk[cmdNom] ? 10 : 4) && IsWorking(); i++)
			SleepThread(500);
	}
}
