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
            SharedChannel channel = segment.channel();

            const CopyResult status = init_reader();
            channel.signal_reader_init(static_cast<int>(status));
            if (status != CopyResult::Ok)
                return static_cast<int>(status);

            reader_loop(channel);
            channel.wait_for_writer_finished();
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
            SharedChannel channel = segment.channel();

            const int init_code = channel.wait_for_reader_init();
            if (init_code != static_cast<int>(CopyResult::Ok))
                return init_code;

            const CopyResult status = init_writer();
            if (status != CopyResult::Ok)
            {
                channel.signal_writer_finished();
                return static_cast<int>(status);
            }

            writer_loop(channel);
            return static_cast<int>(CopyResult::Ok);
        }
        catch (const boost::interprocess::interprocess_exception &ex)
        {
            std::cerr << "Error: shared memory open failed: " << ex.what() << std::endl;
            return static_cast<int>(CopyResult::SharedMemoryFailed);
        }
    }

    CopyResult init_reader()
    {
        if (!open_source(src_path))
            return CopyResult::SourceOpenFailed;
        return CopyResult::Ok;
    }

    CopyResult init_writer()
    {
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

    void reader_loop(SharedChannel &channel)
    {
        SharedChannel::Slot slot;
        do
        {
            src_file.read(slot.data.data(), slot.data.size());
            slot.size = static_cast<size_t>(src_file.gcount());
        } while (channel.push(slot));
    }

    void writer_loop(SharedChannel &channel)
    {
        SharedChannel::Slot slot;
        while (channel.pop(slot))
        {
            dest_file.write(slot.data.data(),
                            static_cast<std::streamsize>(slot.size));
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
