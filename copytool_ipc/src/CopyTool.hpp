#pragma once

#include "CopyResult.hpp"
#include "Role.hpp"

#include <fstream>
#include <string>

class ReaderSharedChannel;

class CopyTool
{
public:
    CopyTool(const std::string &src, const std::string &dest, const std::string &shm,
             Role role, bool overwrite = false);

    int run();

private:
    std::string src_path;
    std::string dest_path;
    std::string shm_name;
    Role process_role;
    bool overwrite_target{false};

    std::ifstream src_file;
    std::ofstream dest_file;

    int run_reader();
    int run_writer();
    CopyResult init_reader();
    CopyResult init_writer();
    bool open_source(const std::string &src);
    bool target_exists(const std::string &dest) const;
    bool open_target(const std::string &dest);
    void reader_loop(ReaderSharedChannel &channel);
    void writer_loop(class WriterSharedChannel &channel);
};
