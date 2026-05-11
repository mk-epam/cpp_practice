#include <gtest/gtest.h>
#include <fstream>
#include <vector>
#include <cstdio>

class CopyToolTest : public ::testing::Test
{
protected:
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
    remove_file(in_file); // Ensure it doesn't exist
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 1); // 1 = reader error
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, WriterErrorInvalidPath)
{
    std::string in_file = "test_writer_error_in.txt";
    std::string out_file = "non_existent_dir/test_writer_error_out.txt";
    create_file(in_file, "data");
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 2); // 2 = writer error
    // No need to remove out_file, it won't exist
}

TEST_F(CopyToolTest, TargetFileExists)
{
    std::string in_file = "test_exists_in.txt";
    std::string out_file = "test_exists_out.txt";
    create_file(in_file, "data");
    create_file(out_file, "existing");
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 3); // 3 = target file exists
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, SourceEqualsDestination)
{
    std::string file = "test_same_file.txt";
    create_file(file, "original content");
    int ret = std::system(("copytool.exe " + file + " " + file).c_str());
    EXPECT_EQ(ret, 3); // 3 = target file exists (overwrite disabled)
    EXPECT_EQ(read_file(file), "original content");
}

TEST_F(CopyToolTest, InvalidArguments)
{
    int ret = std::system("copytool.exe");
    EXPECT_EQ(ret, 4); // 4 = invalid arguments
    ret = std::system("copytool.exe only_one_arg.txt");
    EXPECT_EQ(ret, 4); // 4 = invalid arguments
    ret = std::system("copytool.exe a.txt b.txt c.txt");
    EXPECT_EQ(ret, 4); // 4 = invalid arguments
}

TEST_F(CopyToolTest, EmptyFile)
{
    std::string in_file = "test_empty_in.txt";
    std::string out_file = "test_empty_out.txt";
    create_file(in_file, "");
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, SmallTextFile)
{
    std::string in_file = "test_small_in.txt";
    std::string out_file = "test_small_out.txt";
    create_file(in_file, "Hello, world!");
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, BufferEdgeCase)
{
    std::string in_file = "test_edge_in.txt";
    std::string out_file = "test_edge_out.txt";
    create_file(in_file, "1234567890"); // 10 bytes, buffer size is 8
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}

TEST_F(CopyToolTest, LargeFile)
{
    std::string in_file = "test_large_in.txt";
    std::string out_file = "test_large_out.txt";
    std::string large_content(1000, 'A');
    create_file(in_file, large_content);
    int ret = std::system(("copytool.exe " + in_file + " " + out_file).c_str());
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(read_file(in_file), read_file(out_file));
    files_to_remove.push_back(out_file);
}