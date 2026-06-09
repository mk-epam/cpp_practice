#include "Role.hpp"

#include <boost/interprocess/shared_memory_object.hpp>

Role detect_role(const std::string &shm_name)
{
    using namespace boost::interprocess;
    try
    {
        shared_memory_object test(open_only, shm_name.c_str(), read_only);
        return Role::Writer;
    }
    catch (const interprocess_exception &)
    {
        return Role::Reader;
    }
}
