#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <cstdio>
#include <string>

#include <windows.h>

class CopyToolTest : public ::testing::Test
{
protected:
    struct IpcCopyResult
    {
        int reader_exit{0};
        int writer_exit{0};
    };

    std::vector<std::string> files_to_remove;

    void create_file(const std::string &filename, const std::string &content)
    {
        std::ofstream out(filename, std::ios::binary);
        out << content;
        files_to_remove.push_back(filename);
    }

    std::string read_file(const std::string &filename)
    {
        std::ifstream in(filename, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }

    void remove_file(const std::string &filename)
    {
        std::remove(filename.c_str());
    }

    std::string next_shm_name()
    {
        static int shm_counter = 0;
        const unsigned long pid = GetCurrentProcessId();
        return "copytool_test_shm_" + std::to_string(pid) + "_" + std::to_string(++shm_counter);
    }

    IpcCopyResult run_ipc_copy(const std::string &in_file, const std::string &out_file,
                               const std::string &shm_name)
    {
        const std::string command =
            "copytool.exe " + in_file + " " + out_file + " " + shm_name;

        STARTUPINFOA reader_startup{};
        PROCESS_INFORMATION reader_process{};
        reader_startup.cb = sizeof(reader_startup);
        std::string reader_command = command;
        if (!CreateProcessA(nullptr, reader_command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                            &reader_startup, &reader_process))
        {
            return {-1, -1};
        }

        Sleep(300);

        STARTUPINFOA writer_startup{};
        PROCESS_INFORMATION writer_process{};
        writer_startup.cb = sizeof(writer_startup);
        std::string writer_command = command;
        if (!CreateProcessA(nullptr, writer_command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                            &writer_startup, &writer_process))
        {
            TerminateProcess(reader_process.hProcess, 1);
            CloseHandle(reader_process.hProcess);
            CloseHandle(reader_process.hThread);
            return {-1, -1};
        }

        WaitForSingleObject(writer_process.hProcess, INFINITE);
        WaitForSingleObject(reader_process.hProcess, INFINITE);

        DWORD reader_exit = 0;
        DWORD writer_exit = 0;
        GetExitCodeProcess(reader_process.hProcess, &reader_exit);
        GetExitCodeProcess(writer_process.hProcess, &writer_exit);

        CloseHandle(reader_process.hProcess);
        CloseHandle(reader_process.hThread);
        CloseHandle(writer_process.hProcess);
        CloseHandle(writer_process.hThread);

        return {static_cast<int>(reader_exit), static_cast<int>(writer_exit)};
    }

    void TearDown() override
    {
        for (const auto &f : files_to_remove)
        {
            std::remove(f.c_str());
        }
    }
};

TEST_F(CopyToolTest, ReaderError)
{
    std::string in_file = "test_no_such_file.txt";
    std::string out_file = "test_reader_error_out.txt";
    remove_file(in_file);
    remove_file(out_file);
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 1);
    EXPECT_EQ(ret.writer_exit, 1);
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, WriterErrorInvalidPath)
{
    std::string in_file = "test_writer_error_in.txt";
    std::string out_file = "non_existent_dir/test_writer_error_out.txt";
    create_file(in_file, "data");
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 2);
}

TEST_F(CopyToolTest, TargetFileExists)
{
    std::string in_file = "test_exists_in.txt";
    std::string out_file = "test_exists_out.txt";
    create_file(in_file, "data");
    create_file(out_file, "existing");
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 3);
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, SourceEqualsDestination)
{
    std::string file = "test_same_file.txt";
    create_file(file, "original content");
    const IpcCopyResult ret = run_ipc_copy(file, file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 3);
    EXPECT_EQ(read_file(file), "original content");
}

TEST_F(CopyToolTest, SharedMemoryFailed)
{
    std::string in_file = "test_no_such_file.txt";
    std::string out_file = "test_shared_memory_failed_out.txt";
    remove_file(in_file);
    remove_file(out_file);
    const int ret = std::system(("copytool.exe " + in_file + " " + out_file + " bad\\name").c_str());
    EXPECT_EQ(ret, 5);
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, InvalidArguments)
{
    int ret = std::system("copytool.exe");
    EXPECT_EQ(ret, 4);
    ret = std::system("copytool.exe only_one_arg.txt");
    EXPECT_EQ(ret, 4);
    ret = std::system("copytool.exe a.txt b.txt");
    EXPECT_EQ(ret, 4);
    ret = std::system("copytool.exe a.txt b.txt c.txt d.txt");
    EXPECT_EQ(ret, 4);
}

TEST_F(CopyToolTest, EmptyFile)
{
    std::string in_file = "test_empty_in.txt";
    std::string out_file = "test_empty_out.txt";
    create_file(in_file, "");
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, SmallTextFile)
{
    std::string in_file = "test_small_in.txt";
    std::string out_file = "test_small_out.txt";
    create_file(in_file, "Hello, world!");
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, BufferEdgeCase)
{
    std::string in_file = "test_edge_in.txt";
    std::string out_file = "test_edge_out.txt";
    create_file(in_file, "1234567890");
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, LargeFile)
{
    std::string in_file = "test_large_in.txt";
    std::string out_file = "test_large_out.txt";
    std::string large_content(1000, 'A');
    create_file(in_file, large_content);
    const IpcCopyResult ret = run_ipc_copy(in_file, out_file, next_shm_name());
    EXPECT_EQ(ret.reader_exit, 0);
    EXPECT_EQ(ret.writer_exit, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}
