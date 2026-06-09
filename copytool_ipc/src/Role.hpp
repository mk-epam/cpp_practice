#pragma once

#include <string>

enum class Role
{
    Reader,
    Writer
};

Role detect_role(const std::string &shm_name);
