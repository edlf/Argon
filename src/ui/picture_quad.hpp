// Argon: a picture drawn through hud_draw_rotated_quad (0xcbe50, ui\rotated_quad.hpp) the way FE CHOOSESAVE
// (the ID card, 0x1be00) and FE TRACK INFO (the track, 0x36840) draw theirs: additive (ONE / ONE, op ADD), clipped
// by a viewport box of the 640 x 480 layout
#pragma once
#include "ui/fe_canvas.hpp"
#include "hud/hud_painter.hpp"
#include "ui/rotated_quad.hpp"

struct PictureQuad
{
    Found<const HudTexture> picture;
    menu::RotatedQuadSpec spec;
    ImVec4 viewport;   // x, y, w, h: the box widens from margin to margin in a wide window, the picture with it
};

void drawPictureQuad(HudPainter &painter, const FeCanvas &canvas, const PictureQuad &quad);
