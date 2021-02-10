#include "stdafx.h"
#include "addr2line.h"
#include "AddrInfo.h"
#include "Helpers.h"
#include "Settings.h"

ADDR_INFO InvalidAddrInfo = { INFINITE, 0, "", NULL };
ADDR_INFO* pInvalidAddrInfo = &InvalidAddrInfo;

struct MuduleInfo
{
    char* szModuleName;
    char* szModulePath;
    ADDR_INFO* pAddrInfo;
    MuduleInfo* pPrev;
};

static MuduleInfo* lastMuduleInfo = NULL;
static bool Initialized = false;

void AddrInfo::Reset()
{
    Initialized = false;
    lastMuduleInfo = NULL;
}

void AddModule(CHAR* szModule)
{
    if (szModule[0] == 0)
        return;
    MuduleInfo* pMuduleInfo = (MuduleInfo*)gArchive.Alloc(sizeof(MuduleInfo), true);
    size_t cb = strlen(szModule);
    pMuduleInfo->szModulePath = (char*)gArchive.Alloc((DWORD)cb + 1);
    memcpy(pMuduleInfo->szModulePath, szModule, cb);
    pMuduleInfo->szModulePath[cb] = 0;
    pMuduleInfo->szModuleName = pMuduleInfo->szModulePath;
    char* slash = strrchr(pMuduleInfo->szModuleName, '\\');
    if (!slash)
        slash = strrchr(pMuduleInfo->szModuleName, '/');
    if (slash)
        pMuduleInfo->szModuleName = slash + 1;

    pMuduleInfo->pPrev = lastMuduleInfo;
    lastMuduleInfo = pMuduleInfo;
}

int CreateModuleList()
{
    Initialized = true;

    lastMuduleInfo = (MuduleInfo*)gArchive.Alloc(sizeof(MuduleInfo), true);

    CHAR* szModules = gSettings.GetModules();
    char *newLine = strchr(szModules, '\n');
    while (newLine)
    {
        *newLine = 0;

        AddModule(szModules);

        szModules = newLine + 1;
        newLine = strchr(szModules, '\n');
    }
    return 0;
}

MuduleInfo* GetMuduleInfo(INFO_NODE* infoNode)
{
    MuduleInfo* pMuduleInfo = lastMuduleInfo;
    char* moduleName = infoNode->moduleName();
    while (pMuduleInfo)
    {
		if (pMuduleInfo->szModuleName) {
			int moduleNameLength = std::max(infoNode->moduleNameLength(), (int)strlen(pMuduleInfo->szModuleName));
			if (0 == _strnicmp(moduleName, pMuduleInfo->szModuleName, moduleNameLength)) {
				break;
			}
		}
        pMuduleInfo = pMuduleInfo->pPrev;
    }
    return pMuduleInfo;
}

struct WORK_CTXT {
    MuduleInfo* pMuduleInfo;
    DWORD maxAddr;
    int total;
    ADDR_INFO* last_info;
};
static WORK_CTXT workCtxt;

static int line_info(
    unsigned long address,
    unsigned char op_index,
    char *src,
    char* section_name,
    char* function_name,
    unsigned int line)
{
    int cbSrc = (int)strlen(src);
    ADDR_INFO* p_addr_info = NULL;
    if (workCtxt.last_info && 0 == strcmp(workCtxt.last_info->src, src))
    {
        if (p_addr_info = (ADDR_INFO*)gArchive.Alloc(sizeof(ADDR_INFO)))
            p_addr_info->src = workCtxt.last_info->src;
    }
    else
    {
        if (p_addr_info = (ADDR_INFO*)gArchive.Alloc(sizeof(ADDR_INFO) + cbSrc + 1))
        {
            p_addr_info->src = ((char*)p_addr_info) + sizeof(ADDR_INFO);
            memcpy(p_addr_info->src, src, cbSrc + 1);
        }
    }
    if (p_addr_info == NULL)
        return 0;

    //if (address == 0x13B8C8) { //5b308 13B8C8
    //    stdlog("src: %s:%d %s  %s\n", src, line, section_name ? section_name : "?", function_name ? function_name : "??");
    //    address = address;
    //}

    workCtxt.maxAddr = max(workCtxt.maxAddr, address);
    p_addr_info->addr = address;
    p_addr_info->line = line;
    p_addr_info->pPrev = workCtxt.last_info;
    workCtxt.last_info = p_addr_info;
    workCtxt.pMuduleInfo->pAddrInfo = p_addr_info;
    workCtxt.total++;
    return 1;
}

void FillModuleInfo(MuduleInfo* pMuduleInfo)
{
    ZeroMemory(&workCtxt, sizeof(workCtxt));
    workCtxt.pMuduleInfo = pMuduleInfo;
    enum_file_addresses(pMuduleInfo->szModulePath, ".text", line_info);//".text"
    if (pMuduleInfo->pAddrInfo == NULL)
        pMuduleInfo->pAddrInfo = pInvalidAddrInfo;
}

void AddrInfo::Resolve(INFO_NODE* infoNode)
{
    if (infoNode->isFlow())
    {
        FLOW_NODE* flowNode = (FLOW_NODE*)infoNode;
        flowNode->p_func_info = flowNode->p_call_info = pInvalidAddrInfo;
    }

    if (!Initialized)
    {
        CreateModuleList();
    }

    MuduleInfo* pMuduleInfo = GetMuduleInfo(infoNode);
    if (!pMuduleInfo)
        return;

    if (pMuduleInfo->pAddrInfo == NULL)
        FillModuleInfo(pMuduleInfo);

    if (pMuduleInfo->pAddrInfo == pInvalidAddrInfo)
        return;

    DWORD call_addr = infoNode->call_site;
    DWORD func_addr = 0;
    if (infoNode->isFlow())
    {
        FLOW_NODE* flowNode = (FLOW_NODE*)infoNode;
        func_addr = flowNode->this_fn;
    }

    const int max_delta = 128;
    DWORD call_addr_delta = max_delta;
    DWORD func_addr_delta = max_delta;
    ADDR_INFO *p_addr_info = pMuduleInfo->pAddrInfo;
    while (p_addr_info && (func_addr || call_addr))
    {
        if (call_addr && call_addr >= p_addr_info->addr && (call_addr - p_addr_info->addr) < call_addr_delta)
        {
            call_addr_delta = call_addr - p_addr_info->addr;
            infoNode->p_call_info = p_addr_info;
        }
        else if (call_addr && call_addr < p_addr_info->addr && (p_addr_info->addr - call_addr) < call_addr_delta)
        {
            call_addr_delta = p_addr_info->addr - call_addr;
            infoNode->p_call_info = p_addr_info;
        }

        if (func_addr && func_addr >= p_addr_info->addr && (func_addr - p_addr_info->addr) < func_addr_delta)
        {
            func_addr_delta = func_addr - p_addr_info->addr;
            FLOW_NODE* flowNode = (FLOW_NODE*)infoNode;
            flowNode->p_func_info = p_addr_info;
        }
        else if (func_addr && func_addr < p_addr_info->addr && (p_addr_info->addr - func_addr) < func_addr_delta)
        {
            func_addr_delta = p_addr_info->addr - func_addr;
            FLOW_NODE* flowNode = (FLOW_NODE*)infoNode;
            flowNode->p_func_info = p_addr_info;
        }
        p_addr_info = p_addr_info->pPrev;
    }
}
