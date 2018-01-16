#pragma once
#include "WorkerThread.h"
#include "LogData.h"

class Addr2LineThread :
  public WorkerThread
{
public:
  Addr2LineThread();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);
  void Resolve();

private:
  bool connectToServer(char *szServerName, WORD portNum);
  char *readUrl2(char *szfilepath);
  SOCKET conn;
  HANDLE m_hWorkEvent;
  string server;
};
