// the order-independent translucency's split of an alpha-blended translucent mesh (render\order_independent_translucency.hpp):
// its pixels at least this opaque are drawn as solid surfaces first (mesh_oit_opaque.frag, depth written), the rest
// blended in the OIT targets (mesh_oit.frag). Weighted blending would let a far layer show through a near one at
// alpha 1 (Draco's terrain layers, faded only at their edges)
const float kOitOpaqueAlpha = 250.0 / 255.0;
