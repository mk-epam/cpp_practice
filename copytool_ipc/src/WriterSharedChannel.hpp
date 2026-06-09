#pragma once

struct SharedControl;
struct Slot;

class WriterSharedChannel
{
public:
    explicit WriterSharedChannel(SharedControl *control);

    int wait_for_reader_init();
    void signal_writer_finished();
    Slot &pop();
    void push(Slot &slot);

private:
    SharedControl *ctrl;
};
