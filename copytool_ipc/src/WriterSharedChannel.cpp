#include "WriterSharedChannel.hpp"

#include "SharedControl.hpp"
#include "Slot.hpp"

#include <boost/interprocess/sync/scoped_lock.hpp>

WriterSharedChannel::WriterSharedChannel(SharedControl *control) : ctrl(control) {}

int WriterSharedChannel::wait_for_reader_init()
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    ctrl->writer_cv.wait(lock, [this]
                          { return ctrl->reader_ready; });
    return ctrl->reader_status;
}

void WriterSharedChannel::signal_writer_finished()
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    ctrl->writer_finished = true;
    ctrl->reader_cv.notify_one();
}

Slot &WriterSharedChannel::pop()
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    ctrl->writer_cv.wait(lock, [this]
                          { return ctrl->slots[ctrl->write_index].is_ready(); });
    Slot &slot = ctrl->slots[ctrl->write_index];
    if (slot.empty())
    {
        ctrl->writer_finished = true;
        ctrl->reader_cv.notify_one();
    }
    return slot;
}

void WriterSharedChannel::push(Slot &slot)
{
    if (slot.empty())
        return;

    boost::interprocess::scoped_lock lock(ctrl->mutex);
    slot.clear_ready();
    ctrl->write_index = (ctrl->write_index + 1) % SharedControl::kSlotCount;
    ctrl->reader_cv.notify_one();
}
