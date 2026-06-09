#include "SharedSegment.hpp"

#include "SharedControl.hpp"

#include <new>

SharedSegment::SharedSegment(const std::string &name, bool create)
    : shm_name(name), owns_segment(create)
{
    using namespace boost::interprocess;

    if (create)
    {
        shared_memory_object::remove(shm_name.c_str());
        shm = shared_memory_object(create_only, shm_name.c_str(), read_write);
        shm.truncate(sizeof(SharedControl));
    }
    else
    {
        shm = shared_memory_object(open_only, shm_name.c_str(), read_write);
    }

    region = mapped_region(shm, read_write);
    void *address = region.get_address();
    if (owns_segment)
        ctrl = new (address) SharedControl();
    else
        ctrl = static_cast<SharedControl *>(address);
}

SharedSegment::~SharedSegment()
{
    if (owns_segment && ctrl)
    {
        std::destroy_at(ctrl);
        boost::interprocess::shared_memory_object::remove(shm_name.c_str());
    }
}

ReaderSharedChannel SharedSegment::reader_channel() const
{
    return ReaderSharedChannel(ctrl);
}

WriterSharedChannel SharedSegment::writer_channel() const
{
    return WriterSharedChannel(ctrl);
}
