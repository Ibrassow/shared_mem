#include "spdlog/spdlog.h"
#include <sys/stat.h>        /* For mode constants */
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <thread>


struct Mem
{
    uint8_t change_flag;
    uint8_t data[4];
};

const char* SHARED_MEM_NAME = "/little_mem";
const size_t SHARED_MEM_SIZE = sizeof(Mem);
const char* SEMAPHORE_NAME = "/little_sem";


int TIMEOUT_NS = 0.5 * 1000000000; // 0.5 seconds


void SetupLogger()
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%^%l%$][thread:%t][%s:%#] %v");
}



int main()
{

    SetupLogger();


    // Open the shared memory
    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        spdlog::error("Failed to open shared memory");
        return 1;
    }

    // map the shared memory in the address space of the calling process
    Mem* shared_data = static_cast<Mem*>(mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    // Open the semaphore
    sem_t* sem = sem_open(SEMAPHORE_NAME, O_RDONLY);
    if (sem == SEM_FAILED)
    {
        spdlog::error("Failed to open semaphore");

    }
    Mem mem_struct;
    uint8_t set_read_flag = 0; // reader set the flag to 0

    int read_period = (1.0 / READING_FREQ) * 1000; // Convert to milliseconds

    while (true)
    {


        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += TIMEOUT_NS; // Add the timeout of X seconds
        // need to deal with overflow otherwise will cause undefined behavior at the sem_timedwait
        if (ts.tv_nsec >= 1000000000)
        {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }


        if (sem_timedwait(sem, &ts) == -1) 
        {
            if (errno == ETIMEDOUT) 
            {
                spdlog::error("Semaphore timeout occurred!");
                continue; // Skip 

            } else {
                //print errno linux
                spdlog::error("Semaphore error occurred: {}", strerror(errno));
                break; // Exit 
            }
        }

        spdlog::info("Reader has access to shared memory");


        spdlog::info("Flag read from shared memory: {}", shared_data->change_flag);


        // copy the data from shared memory if needed
        memcpy(&mem_struct, shared_data, sizeof(mem_struct));
        spdlog::info("Data read from shared memory: [{}, {}, {}, {}]", mem_struct.data[0], mem_struct.data[1], mem_struct.data[2], mem_struct.data[3]);
        // now set the flag to 0
        memcpy(shared_data, &set_read_flag, sizeof(mem_struct.change_flag));

        sem_post(sem); // Unlock access


        std::this_thread::sleep_for(std::chrono::milliseconds(read_period));

    }
    




    return 0;
};