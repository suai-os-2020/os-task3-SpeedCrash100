#include "lab3.h"
#include <windows.h>

#include <list>
#include <map>
#include <set>
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
        auto status = WaitForSingleObject(m_semaphores[id], 0);
        while (status == WAIT_TIMEOUT) {
            status = WaitForSingleObject(m_semaphores[id], 0);
        }
    }

    void out(size_t id)
    {
        bool atEnd = (id == (m_semaphores.size() - 1));
        if (atEnd) {
            ReleaseSemaphore(m_semaphores[0], 1, NULL);
        } else {
            ReleaseSemaphore(m_semaphores[id + 1], 1, NULL);
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

//Globals
static const int TIME_MODIF = 4;
std::map<char, HANDLE> threads_map; //Map of threads filtered by name
std::set<char> processing; //Threads thats runs in current cycle
HANDLE stdout_mutex;
HANDLE app_mutex;
int currentStage = 0;
SemaphoreQueue semaphore_queue = SemaphoreQueue(3);

//Threads functions
DWORD WINAPI thread_a(LPVOID);
DWORD WINAPI thread_b(LPVOID);
DWORD WINAPI thread_c(LPVOID);
DWORD WINAPI thread_d(LPVOID);
DWORD WINAPI thread_e(LPVOID);
DWORD WINAPI thread_f(LPVOID);
DWORD WINAPI thread_g(LPVOID);
DWORD WINAPI thread_k(LPVOID);
DWORD WINAPI thread_h(LPVOID);
DWORD WINAPI thread_i(LPVOID);

//States
void state_0(); //Run A
void state_1(); //Wait A; Run E, B, C, F
void state_2(); //Wait C; Run D, G
void state_3(); //Wait B, D; Run H
void state_4(); //Wait G, F; Run I
void state_5(); //Wait E, H; Run K;
void state_6(); //Wait K, I;

//Utils
//Create thread, handles errors
HANDLE create_thread(DWORD(WINAPI* thrd_fun)(LPVOID));
//Wait all threads for created, and finished
void wait_threads(const std::list<char>& thread_names);
//Locks stdout mutex before printing
void print_char_threadsafe(char c);
//Wait for stage
void wait_for_stage(int stage);
//Processing add threadsafe
void processing_add(char ch);
//Processing del threadsafe
void processing_del(char ch);

int lab3_init()
{
    stdout_mutex = CreateMutex(NULL, false, NULL);
    app_mutex = CreateMutex(NULL, false, NULL);
    if (!app_mutex || !stdout_mutex) {
        std::cerr << "Cannot create mutex, error code: " << GetLastError() << std::endl;
        return -1;
    }

    state_0();
    state_1();
    state_2();
    state_3();
    state_4();
    state_5();
    state_6();

    CloseHandle(stdout_mutex);
    CloseHandle(app_mutex);

    for (auto& h_pair : threads_map) {
        CloseHandle(h_pair.second);
    }
    threads_map.clear();

    return 0;
}

HANDLE create_thread(DWORD(WINAPI* thrd_fun)(LPVOID))
{
    HANDLE out = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thrd_fun, NULL, 0, NULL);
    if (out)
        return out;
    std::cerr << "Cannot create thread, error code: " << GetLastError() << std::endl;
    exit(-1);
}

void wait_threads(const std::list<char>& thread_names)
{
    for (auto ch : thread_names) {
        //Wait for creation
        while (threads_map.find(ch) == threads_map.end()) {
            Sleep(0);
        };
        //Wait for execute
        while (WaitForSingleObject(threads_map[ch], 0) == WAIT_TIMEOUT) {
            Sleep(0);
        }
    }
}

void print_char_threadsafe(char c)
{
    while (WaitForSingleObject(stdout_mutex, 0) == WAIT_TIMEOUT)
        Sleep(0);
    std::cout << c << std::flush;
    ReleaseMutex(stdout_mutex);
}

void wait_for_stage(int stage)
{
    while (stage > currentStage) {
        Sleep(0);
    }
}

void processing_add(char ch)
{
    while (WaitForSingleObject(app_mutex, 0) == WAIT_TIMEOUT)
        Sleep(0);
    processing.insert(ch);
    ReleaseMutex(app_mutex);
}

void processing_del(char ch)
{
    while (WaitForSingleObject(app_mutex, 0) == WAIT_TIMEOUT)
        Sleep(0);
    processing.erase(ch);
    ReleaseMutex(app_mutex);
}

DWORD WINAPI thread_a(LPVOID)
{
    processing_add('a');
    for (int i = 0; i < 1 * TIME_MODIF; i++) {
        print_char_threadsafe('a');
        computation();
    }
    processing_del('a');
    return 0;
}
DWORD WINAPI thread_b(LPVOID)
{
    for (int stage = 1; stage < 3; stage++) {
        wait_for_stage(stage);

        processing_add('b');

        for (int i = 0; i < 1 * TIME_MODIF; i++) {
            print_char_threadsafe('b');
            computation();
        }

        processing_del('b');
    }
    return 0;
}
DWORD WINAPI thread_c(LPVOID)
{
    processing_add('c');
    for (int i = 0; i < 1 * TIME_MODIF; i++) {
        print_char_threadsafe('c');
        computation();
    }
    processing_del('c');
    return 0;
}
DWORD WINAPI thread_d(LPVOID)
{
    processing_add('d');
    for (int i = 0; i < 1 * TIME_MODIF; i++) {
        print_char_threadsafe('d');
        computation();
    }
    processing_del('d');
    return 0;
}
DWORD WINAPI thread_e(LPVOID)
{
    for (int stage = 1; stage < 5; stage++) {
        wait_for_stage(stage);

        processing_add('e');

        for (int i = 0; i < 1 * TIME_MODIF; i++) {
            if (stage == 4)
                semaphore_queue.in(0);

            print_char_threadsafe('e');
            computation();

            if (stage == 4)
                semaphore_queue.out(0);
        }

        processing_del('e');
    }

    return 0;
}
DWORD WINAPI thread_f(LPVOID)
{
    for (int stage = 1; stage < 4; stage++) {
        wait_for_stage(stage);

        processing_add('f');

        for (int i = 0; i < 1 * TIME_MODIF; i++) {
            print_char_threadsafe('f');
            computation();
        }

        processing_del('f');
    }
    return 0;
}
DWORD WINAPI thread_g(LPVOID)
{
    for (int stage = 2; stage < 4; stage++) {
        wait_for_stage(stage);

        processing_add('g');

        for (int i = 0; i < 1 * TIME_MODIF; i++) {
            print_char_threadsafe('g');
            computation();
        }

        processing_del('g');
    }
    return 0;
}
DWORD WINAPI thread_k(LPVOID)
{
    processing_add('k');
    for (int i = 0; i < 1 * TIME_MODIF; i++) {
        print_char_threadsafe('k');
        computation();
    }
    processing_del('k');
    return 0;
}
DWORD WINAPI thread_h(LPVOID)
{
    for (int stage = 3; stage < 5; stage++) {
        wait_for_stage(stage);

        processing_add('h');

        for (int i = 0; i < 1 * TIME_MODIF; i++) {
            if (stage == 4)
                semaphore_queue.in(1);
            print_char_threadsafe('h');
            computation();
            if (stage == 4)
                semaphore_queue.out(1);
        }

        processing_del('h');
    }
    return 0;
}
DWORD WINAPI thread_i(LPVOID)
{
    for (int stage = 4; stage < 6; stage++) {
        wait_for_stage(stage);

        processing_add('i');

        for (int i = 0; i < 1 * TIME_MODIF; i++) {
            if (stage == 4)
                semaphore_queue.in(2);
            print_char_threadsafe('i');
            computation();
            if (stage == 4)
                semaphore_queue.out(2);
        }

        processing_del('i');
    }
    return 0;
}

//States
void state_0() //Run A
{
    currentStage = 0;
    threads_map['a'] = create_thread(thread_a);
}
void state_1() //Wait A; Run E, B, C, F
{
    wait_threads({ 'a' });
    while (!processing.empty()) {
        Sleep(0);
    }
    currentStage = 1;
    threads_map['e'] = create_thread(thread_e);
    threads_map['b'] = create_thread(thread_b);
    threads_map['c'] = create_thread(thread_c);
    threads_map['f'] = create_thread(thread_f);
}
void state_2() //Wait C; Run D, G
{
    wait_threads({ 'c' });
    while (!processing.empty()) {
        Sleep(0);
    }
    currentStage = 2;
    threads_map['d'] = create_thread(thread_d);
    threads_map['g'] = create_thread(thread_g);
}
void state_3() //Wait B, D; Run H
{
    wait_threads({ 'b', 'd' });
    while (!processing.empty()) {
        Sleep(0);
    }
    currentStage = 3;
    threads_map['h'] = create_thread(thread_h);
}
void state_4() //Wait G, F; Run I
{
    wait_threads({ 'g', 'f' });
    while (!processing.empty()) {
        Sleep(0);
    }
    currentStage = 4;
    threads_map['i'] = create_thread(thread_i);
}
void state_5() //Wait E, H; Run K;
{
    wait_threads({ 'e', 'h' });
    while (!processing.empty()) {
        Sleep(0);
    }
    currentStage = 5;
    threads_map['k'] = create_thread(thread_k);
}

void state_6() //Wait K, I
{
    wait_threads({ 'k', 'i' });
    while (!processing.empty()) {
        Sleep(0);
    }
    currentStage = 6;
}
