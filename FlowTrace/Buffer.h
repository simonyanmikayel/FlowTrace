#pragma once

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

struct MemBuf
{
    MemBuf(unsigned _int64 maxBufSize = (128LL * 1024 * 1024 * 1024), unsigned long allocSize = 128 * 1024 * 1024) :
        m_allocSize(allocSize)
    {
        InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
        unsigned long chankCount = (unsigned long)(maxBufSize / m_allocSize);
        m_MemBufChank = chankCount ? (MemBufChank*)calloc(chankCount, sizeof(MemBufChank)) : nullptr;
        m_chankCount = m_MemBufChank ? chankCount : 0;
        m_chankEnd = m_MemBufChank ? (m_MemBufChank + m_chankCount) : nullptr;
        m_curChank = m_MemBufChank;
    }

    ~MemBuf()
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

    static void* AllocBuf(unsigned long& size, unsigned long minSize)
    {
        minSize = min(size, minSize);
        unsigned long allocSize = size;
        void * buf = nullptr;
        while (allocSize >= minSize)
        {
            buf = malloc(allocSize);
            if (buf == nullptr)
                allocSize = (allocSize / 5) * 4 + 1;
            else
                break;
        }
        size = buf ? allocSize : 0;
        return buf;
    }
    void * Alloc(unsigned long size)
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
                }
            }
        }
        LeaveCriticalSection(&m_cs);
        return buf;
    }
private:
    void * _Alloc(unsigned long size)
    {
        if (m_curChank == m_chankEnd)
            return nullptr;

        if (m_curChank->buf == nullptr)
        {
            unsigned long allocSize = max(m_allocSize, size);
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
        unsigned long size;
        unsigned long used;
    };

    CRITICAL_SECTION m_cs;
    MemBufChank * m_MemBufChank;
    MemBufChank * m_chankEnd;
    MemBufChank * m_curChank;
    unsigned long m_chankCount;
    unsigned long m_allocSize;
};

struct PtrArray
{
    PtrArray(MemBuf* pMemBuf, unsigned long maxCount) :
        m_pMemBuf(pMemBuf)
        , m_maxCount(maxCount)
        , m_Count(0)
    {
        m_Array = (void**)m_pMemBuf->Alloc(maxCount * sizeof(void**));
    }

    void* Add(void* p)
    {
        if (!m_Array || m_Count == m_maxCount)
            return nullptr;
        m_Array[m_Count] = p;
        m_Count++;
        return p;
    }

    void* Add(unsigned long size, bool zero = false)
    {
        void* p = m_pMemBuf->Alloc(size);
        if (p == nullptr)
            return nullptr;
        if (zero)
            ZeroMemory(p, size);
        return Add(p);
    }

    void* Get(unsigned long i)
    {
        if (!m_Array || i >= m_Count)
            return nullptr;
        return m_Array[i];
    }

    unsigned long Count() { return m_Count; }
    bool Full() { return !m_Array || m_Count == m_maxCount; }

private:
    MemBuf* m_pMemBuf;
    unsigned long m_Count;
    unsigned long m_maxCount;
    void ** m_Array;
};
