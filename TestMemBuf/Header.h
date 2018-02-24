// TestMemBuf.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


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
    DunamicMemBuf(DWORD64 maxBufSize)
    {
        ZeroMemory(this, sizeof(*this));
        m_allocSize = 256 * 1024 * 1024;
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

    template <typename Type> Type New(size_t count, bool zero)
    {
        return Alloc(sizeof(Type)*count, zero);
    }

    void * Alloc(size_t size, bool zero)
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
        if (zero && buf)
            ZeroMemory(buf, size);
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
    PtrArray(MEM_BUF* pMemBuf)
    {
        ZeroMemory(this, sizeof(*this));
        m_pMemBuf = pMemBuf;
        m_IsFull = (nullptr == (m_p = m_pMemBuf->New<void*>(256, true)));
    }

    const Type* AddPtr(const Type* p)
    {
        const Type* pRet = nullptr;
        void **pp1, **pp2, **pp3, **pp4;

        while (!m_IsFull)
        {
            if (m_c1 == 256)
            {
                m_IsFull = true;
                continue;

            }
            pp1 = (void**)m_p; //m_p already allocated in constructor
            if (pp1[m_c1] == nullptr)
            {
                m_IsFull = (nullptr == (pp1[m_c1] = m_pMemBuf->Alloc(256 * sizeof(void*), true)));
                continue;
            }

            if (m_c2 == 256)
            {
                m_c2 = 0; m_c1++;
                continue;
            }
            pp2 = (void**)pp1[m_c1];
            if (pp2[m_c2] == nullptr)
            {
                m_IsFull = (nullptr == (pp2[m_c2] = m_pMemBuf->Alloc(256 * sizeof(void*), true)));
                continue;
            }

            if (m_c3 == 256)
            {
                m_c3 = 0;  m_c2++;
                continue;
            }
            pp3 = (void**)pp2[m_c2];
            if (pp3[m_c3] == nullptr)
            {
                m_IsFull = (nullptr == (pp3[m_c3] = m_pMemBuf->Alloc(256 * sizeof(void*), true)));
                continue;
            }

            if (m_c4 == 256)
            {
                m_c4 = 0;  m_c3++;
                continue;
            }
            pp4 = (void**)pp3[m_c3];
            pp4[m_c4] = (void*)p;
            pRet = p;
            m_c4++;
            m_Count++;
            break;
        }
        return pRet;
    }

    Type* Add(DWORD size, bool zero)
    {
        Type* p = (Type*)m_pMemBuf->Alloc(size, zero);
        if (p == nullptr)
            return nullptr;
        return (Type*)AddPtr(p);
    }

    Type* Get(DWORD i)
    {
        if (i >= m_Count)
            return nullptr;

        void **pp1, **pp2, **pp3, **pp4;
        DWORD c1, c2, c3, c4;

        pp1 = (void**)m_p;
        c1 = (i & 0xFF000000) >> 24;
        pp2 = (void**)pp1[c1];
        c2 = (i & 0x00FF0000) >> 16;
        pp3 = (void**)pp2[c2];
        c3 = (i & 0x0000FF00) >> 8;
        pp4 = (void**)pp3[c3];
        c4 = (i & 0x000000FF);
        Type* p = (Type*)pp4[c4];
        return p;
    }

    unsigned long Count() { return m_Count; }
    MEM_BUF * m_pMemBuf;
    bool m_IsFull;
    DWORD m_Count;
    DWORD m_c1, m_c2, m_c3, m_c4;
    void* m_p;
};

///////////////////////////////////////////////////////////////////
int main()
{
    MEM_BUF buf(16LL * 1024 * 1024 * 1024);
    PtrArray<char> arr(&buf);

    DWORD i, c = -1;
    char* pp;
    char ch = 'a';
    for (i = 0; i < c; i++)
    {
        if (0 == i % (1024 * 1024))
            printf("add %d\n", i);
        pp = arr.Add(2, true);
        if (!pp)
            break;
        pp[0] = ch++;

    }
    printf("tot %d\n", i);
    ch = 'a';
    c = i;
    for (i = 0; i < c; i++)
    {
        if (0 == i % (1024 * 1024))
            printf("get %d\n", i);
        pp = arr.Get(i);
        if (!pp || pp[0] != ch || pp[1] != 0)
        {
            break;
        }
        ch++;
    }
    return 0;
}

//unsigned short short11 = 1024;
//bitset<16> bitset11{ short11 };
//cout << bitset11 << endl;     // 0000010000000000  
//
//unsigned short short12 = short11 >> 1;  // 512  
//bitset<16> bitset12{ short12 };
//cout << bitset12 << endl;     // 0000001000000000