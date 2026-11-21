#include "platform/platform.hpp"

#include <windows.h>
#include <psapi.h>

#include <vector>

namespace platform
{

std::filesystem::path executablePath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    while (GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size())) == buffer.size())
    {
        buffer.resize(buffer.size() * 2);
    }
    return std::filesystem::path(buffer.data());
}

void showErrorDialog(const std::string &title, const std::string &message)
{
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_ICONERROR);
}

uint64_t processPrivateWorkingSet()
{
    PROCESS_MEMORY_COUNTERS_EX2 counters{};
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof counters);
    return counters.PrivateWorkingSetSize;
}

std::FILE *openForReading(const std::filesystem::path &file)
{
    return _wfopen(file.c_str(), L"rb");
}

}  // namespace platform
