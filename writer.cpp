#include "spdlog/spdlog.h"
#include <sys/stat.h>        /* For mode constants */
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <thread>
#include <random>
#include <ctime>

struct Mem
{
    uint8_t change_flag;
    uint8_t data[4];
};


const char* SHARED_MEM_NAME = "/little_mem";
const size_t SHARED_MEM_SIZE = sizeof(Mem);
const char* SEMAPHORE_NAME = "/little_sem";




void SetupLogger()
{
    // Set the log pattern

    // %Y-%m-%d %H:%M:%S.%f: timestamp
    // %t: thread id
    // %l: log level
    // %s: source filename
    // %#: source line number
    // %v: the actual text to log
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%^%l%$][thread:%t][%s:%#] %v");
}


void generate_random_numbers(Mem& mem)
{
    for (int i = 0; i < 4; i++)
    {
        mem.data[i] = static_cast<uint8_t>(rand() % 100);
    }
}

// Always start the writer first
int main()
{

    SetupLogger();

    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        spdlog::error("Failed to open shared memory");
        return 1;
    }

    // Set the size of the shared memory
    ftruncate(shm_fd, SHARED_MEM_SIZE);


    // map the shared memory in the address space of the calling process
    Mem* shared_data = static_cast<Mem*>(mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));



    // Create a semaphore
    sem_unlink(SEMAPHORE_NAME); // Remove any existing semaphore before creating a new one
    sem_t* semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED)
    {
        spdlog::error("Failed to create semaphore");
        return 1;
    }


    Mem mem_struct;
    mem_struct.change_flag = 1; // writer set that flag to 1 by default
    
    int write_period = (1.0 / WRITING_FREQ) * 1000; // Convert to milliseconds




    while (true)
    {

        // just populate data with random numbers
        generate_random_numbers(mem_struct);

        sem_wait(semaphore);   // locking access
        // spdlog::info("Writer has access to shared memory");
        memcpy(shared_data, &mem_struct, sizeof(mem_struct)); // copy the data to shared memory - can also just modify the shared data structure directly
        // spdlog::info("Data written to shared memory: [{}, {}, {}, {}]", mem_struct.data[0], mem_struct.data[1], mem_struct.data[2], mem_struct.data[3]);
        sem_post(semaphore); // Unlock access
        // spdlog::info("Writer has released access to shared memory");

        spdlog::info("Data written to shared memory: [{}, {}, {}, {}]", mem_struct.data[0], mem_struct.data[1], mem_struct.data[2], mem_struct.data[3]);


        std::this_thread::sleep_for(std::chrono::milliseconds(write_period));
    }


    return 0;
}