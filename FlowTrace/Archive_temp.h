#pragma once

struct LOG_REC_BASE_DATA
{
	int len;
	BYTE log_type;
	BYTE log_flags;
	BYTE color;
	BYTE priority;
	unsigned int nn;
	WORD cb_app_name;
	WORD cb_module_name;
	WORD cb_fn_name;
	union {
		WORD cb_trace;
		WORD cb_java_call_site; // for java we keep here caller class:method
	};
	int tid;
	int pid;
	DWORD sec;
	DWORD msec;
	DWORD this_fn;
	DWORD call_site;
	int fn_line;
	int call_line;

	int cbData() { return cb_app_name + cb_module_name + cb_fn_name + cb_trace; }
	int size() { return sizeof(LOG_REC_BASE_DATA) + cbData(); }
	bool isValid() {
		int cb_data = cbData();
		int cb_size = size();
		return len >= cb_size && cb_app_name >= 0 && cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(ROW_LOG_REC) && cb_size < MAX_RECORD_LEN;
	}
	bool isFlow() { return log_type == LOG_TYPE_ENTER || log_type == LOG_TYPE_EXIT; }
	bool isTrace() { return log_type == LOG_TYPE_TRACE; }
};
struct LOG_REC_NET_DATA : public LOG_REC_BASE_DATA
{
	char data[1];
};
struct LOG_REC_ADB_DATA : public LOG_REC_BASE_DATA
{
	bool ftChecked; //is adb sent flow trace
	const char* p_app_name;
	const char* p_module_name;
	const char* p_fn_name;
	const char* p_trace;
};
struct LOG_REC
{
	int cbData() { return getLog()->cbData(); }
	int size() { return getLog()->size(); }
	bool isValid() { return getLog()->isValid(); }
	bool isFlow() { return getLog()->isFlow(); }
	bool isTrace() { return getLog()->isTrace(); }
	virtual LOG_REC_BASE_DATA* getLog() = 0;
	virtual const char* appName() = 0;
	virtual const char* moduleName() = 0;
	virtual const char* fnName() = 0;
	virtual const char* trace() = 0;
	virtual int cbModuleName() = 0;
};
struct LOG_REC_NET : public LOG_REC
{
	LOG_REC_BASE_DATA* getLog() { return rec; };
	const char* appName() { return rec->data; }
	const char* moduleName() { return rec->cb_module_name ? (appName() + rec->cb_app_name) : appName(); }
	const char* fnName() { return moduleName() + cbModuleName(); }
	const char* trace() { return fnName() + rec->cb_fn_name; }
	int cbModuleName() { return rec->cb_module_name ? rec->cb_module_name : rec->cb_app_name; }
private:
	LOG_REC_NET_DATA* rec;
};
struct LOG_REC_ADB : public LOG_REC
{
	LOG_REC_BASE_DATA* getLog() { return rec; };
	const char* appName() { return rec->p_app_name; }
	const char* moduleName() { return rec->p_module_name; }
	const char* fnName() { return rec->p_fn_name; }
	const char* trace() { return rec->p_trace; }
	int cbModuleName() { return rec->cb_module_name; }
private:
	LOG_REC_ADB_DATA* rec;
};
