// Argon: what the front end's shell (ui\fe_shell.hpp) asks of the game around it for its draw; App implements it
// (app\app_menu.cpp). The state the shell reads comes in its own sources (ui\fe_shell_sources.hpp).
#pragma once

class FeCanvas;
struct ImDrawList;

class FeShellHost
{
  public:
    virtual void drawPageModels(ImDrawList *drawList) = 0;   // the page's models (its scene) at this point of the draw list
    virtual void drawBoxModels(ImDrawList *drawList) = 0;    // the page's boxed models (FeScene::boxed) over its frames
    virtual void drawMenuBars(const FeCanvas &canvas, float vw) = 0;

  protected:
    ~FeShellHost() = default;
};
