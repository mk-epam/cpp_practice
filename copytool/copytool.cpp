#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

constexpr size_t BUFFER_SIZE = 8;
std::vector<char> buffer(BUFFER_SIZE);
size_t chunk_size = 0;

bool ready = false;
bool processed = false;

std::mutex mtx;
std::condition_variable cv;

std::atomic<int> error_code{0}; // 0 = OK, 1 = reader error, 2 = writer error

void reader(const std::string &src)
{
    std::ifstream in(src, std::ios::binary);
    if (!in)
    {
        std::cerr << "Error: cannot open source file" << std::endl;
        std::unique_lock<std::mutex> lk(mtx);
        processed = true;
        ready = true;
        error_code = 1;
        lk.unlock();
        cv.notify_one();
        return;
    }

    while (!processed)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, []
                { return !ready; });
        in.read(buffer.data(), buffer.size());
        chunk_size = in.gcount();
        ready = true;
        if (chunk_size == 0)
            processed = true;
        lk.unlock();
        cv.notify_one();
    }
}

void writer(const std::string &dest)
{
    std::ofstream out(dest, std::ios::binary);
    if (!out)
    {
        std::cerr << "Error: cannot open target file" << std::endl;
        std::unique_lock<std::mutex> lk(mtx);
        processed = true;
        ready = true;
        error_code = 2;
        lk.unlock();
        cv.notify_one();
        return;
    }

    while (!processed)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, []
                { return ready; });
        if (chunk_size > 0)
            out.write(buffer.data(), chunk_size);
        ready = false;
        lk.unlock();
        cv.notify_one();
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: copytool <source> <target>" << std::endl;
        return 1;
    }

    std::thread t_reader(reader, argv[1]);
    std::thread t_writer(writer, argv[2]);

    t_reader.join();
    t_writer.join();

    return error_code;
}