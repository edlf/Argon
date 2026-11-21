// Argon: the tests' check macro and counters (each test is one executable: tests\*_tests.cpp, run by ctest).
#pragma once
#include <cstdio>

namespace argon::test
{
inline int failures = 0, checks = 0;
}  // namespace argon::test

#define CHECK(cond)                                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        ++argon::test::checks;                                                                                                                                 \
        if (!(cond))                                                                                                                                           \
        {                                                                                                                                                      \
            ++argon::test::failures;                                                                                                                           \
            std::printf("%s(%d): CHECK failed: %s\n", __FILE__, __LINE__, #cond);                                                                              \
        }                                                                                                                                                      \
    } while (0)
