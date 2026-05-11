#include <array>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

enum class CopyResult
{
    Ok = 0,
    SourceOpenFailed = 1,
    TargetOpenFailed = 2,
    TargetExists = 3,
    InvalidArguments = 4
};

class CopyTool
{
public:
    CopyTool(const std::string &src, const std::string &dest, bool overwrite = false)
        : src_path(src), dest_path(dest), overwrite_target(overwrite)
    {
        for (auto &slot : slots)
            slot.resize(kSlotSize);
    }

    int copy()
    {
        const CopyResult status = init();
        if (status != CopyResult::Ok)
            return static_cast<int>(status);

        std::thread t_reader(&CopyTool::read, this);
        std::thread t_writer(&CopyTool::write, this);
        t_reader.join();
        t_writer.join();
        return static_cast<int>(CopyResult::Ok);
    }

private:
    static constexpr size_t kSlotSize = 8;
    static constexpr size_t kSlotCount = 4;

    std::string src_path;
    std::string dest_path;
    bool overwrite_target{false};

    std::ifstream src_file;
    std::ofstream dest_file;

    std::array<std::vector<char>, kSlotCount> slots{};
    std::array<size_t, kSlotCount> slot_sizes{};
    std::array<bool, kSlotCount> slot_ready{};
    size_t read_index{0};
    size_t write_index{0};

    std::mutex mtx;
    std::condition_variable reader_cv;
    std::condition_variable writer_cv;

    CopyResult init()
    {
        if (!open_source(src_path))
            return CopyResult::SourceOpenFailed;
        if (!overwrite_target && target_exists(dest_path))
            return CopyResult::TargetExists;
        if (!open_target(dest_path))
            return CopyResult::TargetOpenFailed;
        return CopyResult::Ok;
    }

    bool open_source(const std::string &src)
    {
        src_file.open(src, std::ios::binary);
        if (!src_file)
        {
            std::cerr << "Error: cannot open source file" << std::endl;
            return false;
        }
        return true;
    }

    bool target_exists(const std::string &dest) const
    {
        if (std::filesystem::exists(dest))
        {
            std::cerr << "Error: target file already exists (overwrite disabled)" << std::endl;
            return true;
        }
        return false;
    }

    bool open_target(const std::string &dest)
    {
        dest_file.open(dest, std::ios::binary | std::ios::trunc);
        if (!dest_file)
        {
            std::cerr << "Error: cannot open target file" << std::endl;
            return false;
        }
        return true;
    }

    void read()
    {
        while (true)
        {
            {
                std::unique_lock<std::mutex> lk(mtx);
                reader_cv.wait(lk, [this]
                               { return !slot_ready[read_index]; });
            }

            src_file.read(slots[read_index].data(), slots[read_index].size());
            const size_t bytes_read = static_cast<size_t>(src_file.gcount());

            {
                std::unique_lock<std::mutex> lk(mtx);
                slot_sizes[read_index] = bytes_read;
                slot_ready[read_index] = true;
                writer_cv.notify_one();
                if (bytes_read == 0)
                    break;
                read_index = (read_index + 1) % kSlotCount;
            }
        }
    }

    void write()
    {
        while (true)
        {
            size_t bytes_write;
            {
                std::unique_lock<std::mutex> lk(mtx);
                writer_cv.wait(lk, [this]
                               { return slot_ready[write_index]; });
                bytes_write = slot_sizes[write_index];
            }

            if (bytes_write > 0)
                dest_file.write(slots[write_index].data(), static_cast<std::streamsize>(bytes_write));

            {
                std::unique_lock<std::mutex> lk(mtx);
                slot_ready[write_index] = false;
                reader_cv.notify_one();
                if (bytes_write == 0)
                    break;
                write_index = (write_index + 1) % kSlotCount;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: copytool <source> <target>" << std::endl;
        return static_cast<int>(CopyResult::InvalidArguments);
    }

    CopyTool tool(argv[1], argv[2]);
    return tool.copy();
}

