#pragma once

enum class CopyResult
{
    Ok = 0,
    SourceOpenFailed = 1,
    TargetOpenFailed = 2,
    TargetExists = 3,
    InvalidArguments = 4,
    SharedMemoryFailed = 5
};
