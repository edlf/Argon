#include "track/route_report.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace argon
{

void RouteReport::reportNoRoute(const std::string &error)
{
    summary_ = "no AI route: " + error;
    issues_.clear();
}

void RouteReport::reportRoute(std::string summary, std::vector<RouteIssue> issues)
{
    summary_ = std::move(summary);
    issues_ = std::move(issues);
}

std::string RouteReport::issueTally() const
{
    const auto problems = std::ranges::count_if(issues_, &RouteIssue::problem);
    const auto jumps = std::ssize(issues_) - problems;
    std::string tally = problems ? std::to_string(problems) + " problem(s)" : "route OK";
    if (jumps)
    {
        tally += ", " + std::to_string(jumps) + " jump/drop section(s)";
    }
    return tally;
}

} // namespace argon
