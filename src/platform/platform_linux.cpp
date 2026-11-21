#include "platform/platform.hpp"

#include <unistd.h>

#include <fstream>

namespace platform
{

std::filesystem::path executablePath()
{
    return std::filesystem::read_symlink("/proc/self/exe");
}

void showErrorDialog(const std::string &, const std::string &)
{
}

// /proc/self/statm: size resident shared ... in pages
uint64_t processPrivateWorkingSet()
{
    std::ifstream statm("/proc/self/statm");
    uint64_t sizePages = 0, residentPages = 0, sharedPages = 0;
    if (!(statm >> sizePages >> residentPages >> sharedPages))
    {
        return 0;
    }
    const uint64_t privatePages = residentPages > sharedPages ? residentPages - sharedPages : 0;
    return privatePages * uint64_t(sysconf(_SC_PAGESIZE));
}

std::FILE *openForReading(const std::filesystem::path &file)
{
    return std::fopen(file.c_str(), "rb");
}

}  // namespace platform
