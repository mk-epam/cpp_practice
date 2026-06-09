#include "ReaderSharedChannel.hpp"

#include "SharedControl.hpp"
#include "Slot.hpp"

#include <boost/interprocess/sync/scoped_lock.hpp>

ReaderSharedChannel::ReaderSharedChannel(SharedControl *control) : ctrl(control) {}

void ReaderSharedChannel::signal_reader_init(int status)
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    ctrl->reader_status = status;
    ctrl->reader_ready = true;
    ctrl->writer_cv.notify_one();
}

void ReaderSharedChannel::wait_for_writer_finished()
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    ctrl->reader_cv.wait(lock, [this]
                          { return ctrl->writer_finished; });
}

Slot &ReaderSharedChannel::pop()
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    ctrl->reader_cv.wait(lock, [this]
                          { return !ctrl->slots[ctrl->read_index].is_ready() || ctrl->writer_finished; });
    return ctrl->slots[ctrl->read_index];
}

bool ReaderSharedChannel::writer_aborted() const
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    return ctrl->writer_finished;
}

void ReaderSharedChannel::push(Slot &slot)
{
    boost::interprocess::scoped_lock lock(ctrl->mutex);
    if (ctrl->writer_finished)
        return;

    slot.set_ready();
    ctrl->writer_cv.notify_one();
    if (!slot.empty())
        ctrl->read_index = (ctrl->read_index + 1) % SharedControl::kSlotCount;
}
