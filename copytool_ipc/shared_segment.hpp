#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <array>
#include <cstddef>
#include <new>
#include <string>

struct SharedControl
{
    static constexpr size_t kSlotSize = 8;
    static constexpr size_t kSlotCount = 4;

    static constexpr size_t segment_size() { return sizeof(SharedControl); }

    boost::interprocess::interprocess_mutex mutex;
    boost::interprocess::interprocess_condition reader_cv;
    boost::interprocess::interprocess_condition writer_cv;

    bool reader_ready{false};
    int reader_status{0};
    bool writer_finished{false};

    size_t read_index{0};
    size_t write_index{0};
    std::array<size_t, kSlotCount> slot_sizes{};
    std::array<bool, kSlotCount> slot_ready{};
    std::array<std::array<char, kSlotSize>, kSlotCount> slots{};

    SharedControl() = default;

    ~SharedControl() = default;
};

class SharedSegment
{
public:
    SharedSegment(const std::string &name, bool create)
        : shm_name(name), owns_segment(create)
    {
        using namespace boost::interprocess;

        if (create)
        {
            shared_memory_object::remove(shm_name.c_str());
            shm = shared_memory_object(create_only, shm_name.c_str(), read_write);
            shm.truncate(SharedControl::segment_size());
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

    ~SharedSegment()
    {
        if (owns_segment && ctrl)
        {
            ctrl->~SharedControl();
            boost::interprocess::shared_memory_object::remove(shm_name.c_str());
        }
    }

    SharedControl *get() const { return ctrl; }

    SharedSegment(const SharedSegment &) = delete;
    SharedSegment &operator=(const SharedSegment &) = delete;

private:
    std::string shm_name;
    bool owns_segment;
    boost::interprocess::shared_memory_object shm;
    boost::interprocess::mapped_region region;
    SharedControl *ctrl;
};
