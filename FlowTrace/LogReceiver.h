#pragma once
#include "WorkerThread.h"

class LogReceiver :
  public WorkerThread
{
public:
  LogReceiver();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);

private:
  SOCKET s;
};

