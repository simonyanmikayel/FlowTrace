#pragma once


#ifdef _STATIC_MEM_BUF
#define MEM_BUF StaticMemBuf
#else
#define MEM_BUF DunamicMemBuf
#endif


inline void* AllocBuf(size_t size, size_t minSize)
{
    minSize = min(size, minSize);
    size_t allocSize = size;
    void * buf = nullptr;
    while (allocSize >= minSize)
    {
#if defined(_FILE_MAP_MEM_BUF) && defined(_STATIC_MEM_BUF)
        LARGE_INTEGER li;
        li.QuadPart = allocSize;
        buf = nullptr;
        HANDLE hMap;
        if (NULL == (hMap = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, li.HighPart, li.LowPart, NULL))
            || NULL == (buf = (char*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0)))
        {
            if (hMap)
                CloseHandle(hMap);
        }
#else
        buf = malloc(allocSize);
#endif
        if (buf == nullptr)
            allocSize = (allocSize / 5) * 4 + 1;
        else
            break;
    }
    size = buf ? allocSize : 0;
    return buf;
}




class StaticMemBuf
{
public:
    StaticMemBuf(size_t maxBufSize)
    {
        ZeroMemory(this, sizeof(*this));
        if (!m_Initialized)
        {
            m_Initialized = true;
            InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
            m_bufSize = maxBufSize;
            m_buf = (char*)AllocBuf(m_bufSize, 512 * 1024 * 1024);
        }
    }

    void * Alloc(size_t size)
    {
        char* buf = nullptr;
        EnterCriticalSection(&m_cs);
        if (m_buf && m_curPos + size < m_bufSize)
        {
            buf = m_buf + m_curPos;
            m_curPos += size;
        }
        LeaveCriticalSection(&m_cs);
        return buf;
    }

    size_t UsedMemory() { return m_curPos; }

private:
    size_t m_curPos;
    static bool m_Initialized;
    static CRITICAL_SECTION m_cs;
    static size_t m_bufSize;
    static char* m_buf;
};

class DunamicMemBuf
{
public:
    DunamicMemBuf(size_t maxBufSize)
    {
        ZeroMemory(this, sizeof(*this));
        m_allocSize = 128 * 1024 * 1024;
        InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
        size_t chankCount = max(1, (unsigned long)(maxBufSize / m_allocSize));
        m_MemBufChank = chankCount ? (MemBufChank*)calloc(chankCount, sizeof(MemBufChank)) : nullptr;
        m_chankCount = m_MemBufChank ? chankCount : 0;
        m_chankEnd = m_MemBufChank ? (m_MemBufChank + m_chankCount) : nullptr;
        m_curChank = m_MemBufChank;
    }

    ~DunamicMemBuf()
    {
        MemBufChank * chank = m_MemBufChank;
        DWORD chanckCount = 0;
        while (chank <= m_curChank && chank < m_chankEnd)
        {
            free(chank->buf);
            chank++;
            chanckCount++;
        }
        DeleteCriticalSection(&m_cs);
    }

    void * Alloc(size_t size)
    {
        void * buf = nullptr;
        EnterCriticalSection(&m_cs);
        if (m_curChank < m_chankEnd)
        {
            buf = _Alloc(size);
            if (buf == nullptr)
            {
                m_curChank++;
                if (m_curChank < m_chankEnd)
                {
                    buf = _Alloc(size);
                    if (buf)
                        m_usedMem += size;
                }
            }
        }
        LeaveCriticalSection(&m_cs);
        return buf;
    }

    size_t UsedMemory() { return m_usedMem; }

private:
    void * _Alloc(size_t size)
    {
        if (m_curChank == m_chankEnd)
            return nullptr;

        if (m_curChank->buf == nullptr)
        {
            size_t allocSize = max(m_allocSize, size);
            if (m_curChank->buf = AllocBuf(allocSize, size))
            {
                m_curChank->size = allocSize;
            }
            else
            {
                m_chankEnd = m_curChank;
                return nullptr;
            }
        }

        if (size > (m_curChank->size - m_curChank->used))
        {
            return nullptr;
        }

        void * buf = (char*)m_curChank->buf + m_curChank->used;
        m_curChank->used += size;
        return buf;
    }

    struct MemBufChank
    {
        void * buf;
        size_t size;
        size_t used;
    };

    CRITICAL_SECTION m_cs;
    MemBufChank * m_MemBufChank;
    MemBufChank * m_chankEnd;
    MemBufChank * m_curChank;
    size_t m_chankCount;
    size_t m_allocSize;
    size_t m_usedMem;
};

template <typename Type> class PtrArray
{
public:
    PtrArray(MEM_BUF* pMemBuf) :
        m_pMemBuf(pMemBuf)
        , m_Count(0)
    {
        m_Array = (void**)m_pMemBuf->Alloc(10000 * sizeof(void**));
    }

    Type* Add(Type* p)
    {
        void** ptr = GetPtr(m_Count);
        if (!ptr)
            return nullptr;
        *ptr = p;
        m_Count++;
        return p;
    }

    Type* Add(unsigned long size, bool zero = false)
    {
        Type* p = (Type*)m_pMemBuf->Alloc(size);
        if (p == nullptr)
            return nullptr;
        if (zero)
            ZeroMemory(p, size);
        return Add(p);
    }

    Type* Get(unsigned long i)
    {
        if (i >= m_Count)
            return nullptr;
        return (Type*)(*GetPtr(i));
    }

    unsigned long Count() { return m_Count; }

private:
    void** GetPtr(DWORD i)
    {
        return m_Array + i;
    }

    MEM_BUF * m_pMemBuf;
    DWORD m_Count;
    void** m_Array;
};
