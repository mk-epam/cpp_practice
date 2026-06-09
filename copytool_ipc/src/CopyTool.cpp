#include "CopyTool.hpp"

#include "ReaderSharedChannel.hpp"
#include "SharedSegment.hpp"
#include "Slot.hpp"
#include "WriterSharedChannel.hpp"

#include <boost/interprocess/exceptions.hpp>

#include <filesystem>
#include <iostream>

CopyTool::CopyTool(const std::string &src, const std::string &dest, const std::string &shm,
                   Role role, bool overwrite)
    : src_path(src), dest_path(dest), shm_name(shm), process_role(role),
      overwrite_target(overwrite)
{
}

int CopyTool::run()
{
    if (process_role == Role::Reader)
        return run_reader();
    return run_writer();
}

int CopyTool::run_reader()
{
    try
    {
        SharedSegment segment(shm_name, true);
        ReaderSharedChannel channel = segment.reader_channel();

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

int CopyTool::run_writer()
{
    try
    {
        SharedSegment segment(shm_name, false);
        WriterSharedChannel channel = segment.writer_channel();

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

CopyResult CopyTool::init_reader()
{
    if (!open_source(src_path))
        return CopyResult::SourceOpenFailed;
    return CopyResult::Ok;
}

CopyResult CopyTool::init_writer()
{
    if (!overwrite_target && target_exists(dest_path))
        return CopyResult::TargetExists;
    if (!open_target(dest_path))
        return CopyResult::TargetOpenFailed;
    return CopyResult::Ok;
}

bool CopyTool::open_source(const std::string &src)
{
    src_file.open(src, std::ios::binary);
    if (!src_file)
    {
        std::cerr << "Error: cannot open source file" << std::endl;
        return false;
    }
    return true;
}

bool CopyTool::target_exists(const std::string &dest) const
{
    if (std::filesystem::exists(dest))
    {
        std::cerr << "Error: target file already exists (overwrite disabled)" << std::endl;
        return true;
    }
    return false;
}

bool CopyTool::open_target(const std::string &dest)
{
    dest_file.open(dest, std::ios::binary | std::ios::trunc);
    if (!dest_file)
    {
        std::cerr << "Error: cannot open target file" << std::endl;
        return false;
    }
    return true;
}

void CopyTool::reader_loop(ReaderSharedChannel &channel)
{
    while (true)
    {
        Slot &slot = channel.pop();
        if (channel.writer_aborted())
            break;

        src_file.read(slot.data(), slot.capacity());
        slot.set_size(static_cast<size_t>(src_file.gcount()));
        channel.push(slot);
        if (slot.empty())
            break;
    }
}

void CopyTool::writer_loop(WriterSharedChannel &channel)
{
    while (true)
    {
        Slot &slot = channel.pop();
        if (slot.empty())
            break;

        dest_file.write(slot.data(), static_cast<std::streamsize>(slot.size()));
        channel.push(slot);
    }
}
