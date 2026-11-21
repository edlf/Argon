// A mesh draw's own data, by its instance index (src\render\mesh_draw_records.hpp: MeshDraw).
// ambient / light0 with w = 1: the draw's own scene lights (a racing craft's, 0x59bc0) instead of the frame's
// stageRates: per stage the rate an earlier draw's still-bound texture picked its frame with (< 0: the stage's own)
struct MeshDraw { vec4 ambient; vec4 light0; vec4 stageRates; int material; };
layout(std430, set = 1, binding = 4) readonly buffer MeshDraws { MeshDraw d[]; } draws;
