#pragma once

#include "Slot.hpp"

#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include <array>
#include <cstddef>

struct SharedControl
{
    static constexpr size_t kSlotSize = Slot::kCapacity;
    static constexpr size_t kSlotCount = 4;

    boost::interprocess::interprocess_mutex mutex;
    boost::interprocess::interprocess_condition reader_cv;
    boost::interprocess::interprocess_condition writer_cv;

    bool reader_ready{false};
    int reader_status{0};
    bool writer_finished{false};

    size_t read_index{0};
    size_t write_index{0};
    std::array<Slot, kSlotCount> slots{};
};
