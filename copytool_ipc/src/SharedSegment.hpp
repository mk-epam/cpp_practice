#pragma once

#include "ReaderSharedChannel.hpp"
#include "WriterSharedChannel.hpp"

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <string>

struct SharedControl;

class SharedSegment
{
public:
    SharedSegment(const std::string &name, bool create);
    ~SharedSegment();

    ReaderSharedChannel reader_channel() const;
    WriterSharedChannel writer_channel() const;

    SharedSegment(const SharedSegment &) = delete;
    SharedSegment &operator=(const SharedSegment &) = delete;

private:
    std::string shm_name;
    bool owns_segment;
    boost::interprocess::shared_memory_object shm;
    boost::interprocess::mapped_region region;
    SharedControl *ctrl;
};
