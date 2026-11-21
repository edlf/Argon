// Argon: what the editor shows of the loaded track's AI route (and the start-up prints): its summary line and the
// issues Route::check found along it.
#pragma once
#include <string>
#include <vector>

#include "craft/drive.hpp"

namespace argon
{

class RouteReport
{
  public:
    void reportNoRoute(const std::string &error);
    void reportRoute(std::string summary, std::vector<RouteIssue> issues);

    const std::string &summary() const
    {
        return summary_;
    }
    const std::vector<RouteIssue> &issues() const
    {
        return issues_;
    }
    std::string issueTally() const;   // "route OK" or "N problem(s)", then the jump / drop sections

  private:
    std::string summary_;
    std::vector<RouteIssue> issues_;
};

} // namespace argon
