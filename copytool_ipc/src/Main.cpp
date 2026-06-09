#include "CopyResult.hpp"
#include "CopyTool.hpp"
#include "Role.hpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "Usage: copytool <source> <target> <shared_memory_name>" << std::endl;
        return static_cast<int>(CopyResult::InvalidArguments);
    }

    const std::string shm_name(argv[3]);
    const Role role = detect_role(shm_name);
    CopyTool tool(argv[1], argv[2], shm_name, role);
    return tool.run();
}
