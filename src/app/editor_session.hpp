// Argon: the editor (the debug view) as App keeps it between visits: whether it is open, the track it last showed
// (Game Demos opens it there again) and which of its tool windows are open.
#pragma once
#include "ui_extras/editor_panel.hpp"

class EditorSession
{
  public:
    void enter()
    {
        active_ = true;
    }
    void leave()
    {
        active_ = false;
    }
    // in the editor: its markings (AI route, grid, ...) are drawn and the mouse moves the camera; not in the menus' races
    bool active() const
    {
        return active_;
    }

    void rememberTrack(int listIndex)
    {
        lastTrack_ = listIndex;
    }
    int lastTrack() const   // -1: none yet
    {
        return lastTrack_;
    }

    EditorWindowsOpen &windows()   // the optional tool windows (EditorPanel), all closed at start
    {
        return windows_;
    }

  private:
    bool active_ = false;
    int lastTrack_ = -1;
    EditorWindowsOpen windows_{};
};
