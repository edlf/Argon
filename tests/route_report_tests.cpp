// Argon: tests for the editor's route report (track/route_report.hpp): its summary and the issue tally. Run with
// test.bat / test.sh.
#include "track/route_report.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::RouteIssue;
using argon::RouteReport;

RouteIssue issueOfKind(RouteIssue::Kind kind)
{
    return RouteIssue{kind, 0.f, 10.f, glm::vec3(0.f), 1.f};
}

void aRouteWithoutIssuesIsOk()
{
    RouteReport report;

    report.reportRoute("100 m", {});

    CHECK(report.issueTally() == "route OK");
}

void problemsAreCounted()
{
    RouteReport report;

    report.reportRoute("100 m", {issueOfKind(RouteIssue::Blocked), issueOfKind(RouteIssue::Steep)});

    CHECK(report.issueTally() == "2 problem(s)");
}

void airborneSectionsAreCountedAsJumps()
{
    RouteReport report;

    report.reportRoute("100 m", {issueOfKind(RouteIssue::NoGround), issueOfKind(RouteIssue::Airborne)});

    CHECK(report.issueTally() == "1 problem(s), 1 jump/drop section(s)");
}

void noRouteNamesTheError()
{
    RouteReport report;

    report.reportNoRoute("missing ai file");

    CHECK(report.summary() == "no AI route: missing ai file");
}

void noRouteDropsTheLastTracksIssues()
{
    RouteReport report;
    report.reportRoute("100 m", {issueOfKind(RouteIssue::Blocked)});

    report.reportNoRoute("missing ai file");

    CHECK(report.issues().empty());
}

}  // namespace

int main()
{
    aRouteWithoutIssuesIsOk();
    problemsAreCounted();
    airborneSectionsAreCountedAsJumps();
    noRouteNamesTheError();
    noRouteDropsTheLastTracksIssues();
    std::printf("route_report_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
