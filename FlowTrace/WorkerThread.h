#pragma once
class WorkerThread
{
public:
    WorkerThread(void);

    DWORD getTID(){ return m_dwTID; }
    bool IsWorking(){ return m_bWorking; }
    void StartWork(LPVOID pWorkParam = 0);
    void StopWork(const int rettryCount = 500);
    virtual void Work(LPVOID pWorkParam) = 0;
    virtual void Terminate() = 0;
	DWORD SleepThread(DWORD  dwMilliseconds);
	void ResumeThread();
	void SetThreadPriority(int nPriority);
private:
    LPVOID m_pWorkParam;
    bool m_bWorking;
    HANDLE m_hTreadEvent;
    HANDLE m_hThread;
    DWORD m_dwTID;
	void Cleanup();
    static DWORD WINAPI ThreadFunk(LPVOID lpParameter);
};

