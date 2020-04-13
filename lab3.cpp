#include "lab3.h"
#include <windows.h>

#include <vector>

//
// lab3 code should be located here!
//

unsigned int lab3_thread_graph_id()
{
    return 16;
}

const char* lab3_unsynchronized_threads()
{
    return "bcfe";
}

const char* lab3_sequential_threads()
{
    return "ehi";
}

int lab3_init()
{
    return 0;
}

//Class SemaphoreQueue ported for windows
class SemaphoreQueue {
private:
    std::vector<HANDLE> m_semaphores;

public:
    SemaphoreQueue(size_t size)
    {
        m_semaphores.resize(size);
        m_semaphores[0] = createSem(1);
        for (auto it = m_semaphores.begin() + 1; it != m_semaphores.end(); it++) {
            *it = createSem(0);
        }
    }

    ~SemaphoreQueue()
    {
        for (auto& s : m_semaphores) {
            CloseHandle(s);
        }
        m_semaphores.clear();
    }

    void in(size_t id)
    {
        while (WaitForSingleObject(m_semaphores[id], 0) != WAIT_OBJECT_0)
            ;
    }

    void out(size_t id)
    {
        bool atEnd = (id == (m_semaphores.size() - 1));
        if (atEnd) {
            ReleaseSemaphore(&m_semaphores[0], 1, NULL);
        } else {
            ReleaseSemaphore(&m_semaphores[id + 1], 1, NULL);
        }
    }

private:
    HANDLE createSem(int init_val)
    {
        HANDLE out = CreateSemaphore(nullptr, init_val, 1, nullptr); // Max counter is 1. Queue doesn't need more.
        if (out)
            return out;

        std::cerr << "Cannot create semaphore, code:" << GetLastError() << std::endl;
        exit(-1);
    }
};
