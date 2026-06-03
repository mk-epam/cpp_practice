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
};

class SharedChannel
{
public:
    struct Slot
    {
        size_t size{0};
        std::array<char, SharedControl::kSlotSize> data{};
    };

    explicit SharedChannel(SharedControl *control) : ctrl(control) {}

    void signal_reader_init(int status)
    {
        Lock lock(ctrl->mutex);
        ctrl->reader_status = status;
        ctrl->reader_ready = true;
        ctrl->writer_cv.notify_one();
    }

    int wait_for_reader_init()
    {
        Lock lock(ctrl->mutex);
        ctrl->writer_cv.wait(lock, [this]
                              { return ctrl->reader_ready; });
        return ctrl->reader_status;
    }

    void signal_writer_finished()
    {
        Lock lock(ctrl->mutex);
        ctrl->writer_finished = true;
        ctrl->reader_cv.notify_one();
    }

    void wait_for_writer_finished()
    {
        Lock lock(ctrl->mutex);
        ctrl->reader_cv.wait(lock, [this]
                              { return ctrl->writer_finished; });
    }

    bool push(const Slot &in)
    {
        Lock lock(ctrl->mutex);
        ctrl->reader_cv.wait(lock, [this]
                              { return !ctrl->slot_ready[ctrl->read_index] || ctrl->writer_finished; });
        if (ctrl->writer_finished)
            return false;

        const size_t idx = ctrl->read_index;
        ctrl->slot_sizes[idx] = in.size;
        ctrl->slots[idx] = in.data;
        ctrl->slot_ready[idx] = true;
        ctrl->writer_cv.notify_one();
        if (in.size == 0)
            return false;
        ctrl->read_index = (idx + 1) % SharedControl::kSlotCount;
        return true;
    }

    bool pop(Slot &out)
    {
        Lock lock(ctrl->mutex);
        ctrl->writer_cv.wait(lock, [this]
                              { return ctrl->slot_ready[ctrl->write_index]; });
        const size_t idx = ctrl->write_index;
        out.size = ctrl->slot_sizes[idx];
        out.data = ctrl->slots[idx];
        ctrl->slot_ready[idx] = false;
        if (out.size == 0)
        {
            ctrl->writer_finished = true;
            ctrl->reader_cv.notify_one();
            return false;
        }
        ctrl->write_index = (idx + 1) % SharedControl::kSlotCount;
        ctrl->reader_cv.notify_one();
        return true;
    }

private:
    using Lock = boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>;

    SharedControl *ctrl;
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

    ~SharedSegment()
    {
        if (owns_segment && ctrl)
        {
            std::destroy_at(ctrl);
            boost::interprocess::shared_memory_object::remove(shm_name.c_str());
        }
    }

    SharedChannel channel() const { return SharedChannel(ctrl); }

    SharedSegment(const SharedSegment &) = delete;
    SharedSegment &operator=(const SharedSegment &) = delete;

private:
    std::string shm_name;
    bool owns_segment;
    boost::interprocess::shared_memory_object shm;
    boost::interprocess::mapped_region region;
    SharedControl *ctrl;
};
