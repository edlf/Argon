// Argon: the front-end pages a game mode leads to once its picks are made (the front end's shell opens them,
// ui\fe_shell.hpp): FE COURSE SELECT after the character, FE TOURNEYTYPE for a tournament, FE GHOSTSELECT for Time
// Attack.
#pragma once

class ModePages
{
  public:
    virtual void openCourseSelect() = 0;
    virtual void openTourneyType() = 0;
    virtual void openGhostSelect(int pilot, int trackId) = 0;

  protected:
    ~ModePages() = default;
};
