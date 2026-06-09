#pragma once

struct SharedControl;
struct Slot;

class ReaderSharedChannel
{
public:
    explicit ReaderSharedChannel(SharedControl *control);

    void signal_reader_init(int status);
    void wait_for_writer_finished();
    Slot &pop();
    bool writer_aborted() const;
    void push(Slot &slot);

private:
    SharedControl *ctrl;
};
