// Argon: the few things the operating system does differently. One implementation per system:
// platform_win32.cpp, platform_linux.cpp (CMakeLists.txt picks one).
#pragma once
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

namespace platform
{

// the running executable's own file
std::filesystem::path executablePath();

// a modal error box on Windows; on Linux nothing, the caller has already written the error to stderr
void showErrorDialog(const std::string &title, const std::string &message);

// the private memory this process has in physical RAM (Windows: private working set, as Task Manager shows; Linux: resident minus shared)
uint64_t processPrivateWorkingSet();

// a C file handle for reading in binary (any characters in the path, also on Windows); null when it cannot be opened
std::FILE *openForReading(const std::filesystem::path &file);

}  // namespace platform
