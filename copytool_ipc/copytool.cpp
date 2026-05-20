#include "shared_segment.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>

enum class CopyResult
{
    Ok = 0,
    SourceOpenFailed = 1,
    TargetOpenFailed = 2,
    TargetExists = 3,
    InvalidArguments = 4,
    SharedMemoryFailed = 5
};

enum class Role
{
    Reader,
    Writer
};

class CopyTool
{
public:
    CopyTool(const std::string &src, const std::string &dest, const std::string &shm,
             Role role, bool overwrite = false)
        : src_path(src), dest_path(dest), shm_name(shm), process_role(role),
          overwrite_target(overwrite)
    {
    }

    int run()
    {
        if (process_role == Role::Reader)
            return run_reader();
        return run_writer();
    }

private:
    std::string src_path;
    std::string dest_path;
    std::string shm_name;
    Role process_role;
    bool overwrite_target{false};

    std::ifstream src_file;
    std::ofstream dest_file;

    int run_reader()
    {
        try
        {
            SharedSegment segment(shm_name, true);
            SharedControl *shared = segment.get();

            const CopyResult status = init_reader(shared);
            if (status != CopyResult::Ok)
                return static_cast<int>(status);

            reader_loop(shared);
            wait_for_writer(shared);
            return static_cast<int>(CopyResult::Ok);
        }
        catch (const boost::interprocess::interprocess_exception &ex)
        {
            std::cerr << "Error: shared memory setup failed: " << ex.what() << std::endl;
            return static_cast<int>(CopyResult::SharedMemoryFailed);
        }
    }

    int run_writer()
    {
        try
        {
            SharedSegment segment(shm_name, false);
            SharedControl *shared = segment.get();

            const int init_code = wait_for_reader_init(shared);
            if (init_code != static_cast<int>(CopyResult::Ok))
                return init_code;

            const CopyResult status = init_writer(shared);
            if (status != CopyResult::Ok)
                return static_cast<int>(status);

            writer_loop(shared);
            return static_cast<int>(CopyResult::Ok);
        }
        catch (const boost::interprocess::interprocess_exception &ex)
        {
            std::cerr << "Error: shared memory open failed: " << ex.what() << std::endl;
            return static_cast<int>(CopyResult::SharedMemoryFailed);
        }
    }

    CopyResult init_reader(SharedControl *shared)
    {
        CopyResult status = CopyResult::Ok;

        if (!open_source(src_path))
            status = CopyResult::SourceOpenFailed;

        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
        shared->reader_status = static_cast<int>(status);
        shared->reader_ready = true;
        shared->writer_cv.notify_one();

        return status;
    }

    CopyResult init_writer(SharedControl *shared)
    {
        CopyResult status = CopyResult::Ok;

        if (!overwrite_target && target_exists(dest_path))
            status = CopyResult::TargetExists;
        else if (!open_target(dest_path))
            status = CopyResult::TargetOpenFailed;

        if (status != CopyResult::Ok)
        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
            shared->writer_finished = true;
            shared->reader_cv.notify_one();
        }

        return status;
    }

    int wait_for_reader_init(SharedControl *shared)
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
        shared->writer_cv.wait(lock, [shared]
                               { return shared->reader_ready; });
        return shared->reader_status;
    }

    void wait_for_writer(SharedControl *shared)
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
        shared->reader_cv.wait(lock, [shared]
                               { return shared->writer_finished; });
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

    void reader_loop(SharedControl *shared)
    {
        while (true)
        {
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
                shared->reader_cv.wait(lock, [shared]
                                       { return !shared->slot_ready[shared->read_index] || shared->writer_finished; });
                if (shared->writer_finished)
                    break;
            }

            src_file.read(shared->slots[shared->read_index].data(), SharedControl::kSlotSize);
            const size_t bytes_read = static_cast<size_t>(src_file.gcount());

            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
                shared->slot_sizes[shared->read_index] = bytes_read;
                shared->slot_ready[shared->read_index] = true;
                shared->writer_cv.notify_one();
                if (bytes_read == 0)
                    break;
                shared->read_index = (shared->read_index + 1) % SharedControl::kSlotCount;
            }
        }
    }

    void writer_loop(SharedControl *shared)
    {
        while (true)
        {
            size_t bytes_write = 0;
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
                shared->writer_cv.wait(lock, [shared]
                                       { return shared->slot_ready[shared->write_index]; });
                bytes_write = shared->slot_sizes[shared->write_index];
            }

            if (bytes_write > 0)
            {
                dest_file.write(shared->slots[shared->write_index].data(),
                                static_cast<std::streamsize>(bytes_write));
            }

            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(shared->mutex);
                shared->slot_ready[shared->write_index] = false;
                if (bytes_write == 0)
                {
                    shared->writer_finished = true;
                    shared->reader_cv.notify_one();
                    break;
                }
                shared->reader_cv.notify_one();
                shared->write_index = (shared->write_index + 1) % SharedControl::kSlotCount;
            }
        }
    }
};

Role detect_role(const std::string &shm_name)
{
    using namespace boost::interprocess;
    try
    {
        shared_memory_object test(open_only, shm_name.c_str(), read_only);
        return Role::Writer;
    }
    catch (const interprocess_exception &)
    {
        return Role::Reader;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "Usage: copytool <source> <target> <shared_memory_name>" << std::endl;
        return static_cast<int>(CopyResult::InvalidArguments);
    }

    const std::string shm_name(argv[3]);
    const Role role = detect_role(shm_name);
    CopyTool tool(argv[1], argv[2], shm_name, role);
    const int code = tool.run();

    return code;
}
