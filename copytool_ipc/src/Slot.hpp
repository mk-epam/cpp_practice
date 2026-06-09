#pragma once

#include <array>
#include <cstddef>

struct Slot
{
    static constexpr size_t kCapacity = 8;

    char *data() { return buffer.data(); }
    const char *data() const { return buffer.data(); }

    static constexpr size_t capacity() { return kCapacity; }

    size_t size() const { return byte_count; }
    void set_size(size_t bytes) { byte_count = bytes; }

    bool empty() const { return byte_count == 0; }
    bool is_ready() const { return ready; }

    void set_ready() { ready = true; }
    void clear_ready() { ready = false; }

private:
    size_t byte_count{0};
    bool ready{false};
    std::array<char, kCapacity> buffer{};
};
