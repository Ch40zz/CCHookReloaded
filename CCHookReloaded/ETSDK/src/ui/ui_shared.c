#include "../game/q_shared.h"
#include "../cgame/tr_types.h"
#include "keycodes.h"

#include "../../etmain/ui/menudef.h"

void MiniAngleToAxis(vec_t angle, vec2_t axes[2])
{
	axes[0][0] = (vec_t)sin( -angle );
	axes[0][1] = -(vec_t)cos( -angle );

	axes[1][0] = -axes[0][1];
	axes[1][1] = axes[0][0];
}
void SetupRotatedThing(polyVert_t* verts, vec2_t org, float w, float h, vec_t angle)
{
	vec2_t axes[2];

	MiniAngleToAxis( angle, axes );

	verts[0].xyz[0] = org[0] - ( w * 0.5f ) * axes[0][0];
	verts[0].xyz[1] = org[1] - ( w * 0.5f ) * axes[0][1];
	verts[0].xyz[2] = 0;
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	verts[1].xyz[0] = verts[0].xyz[0] + w * axes[0][0];
	verts[1].xyz[1] = verts[0].xyz[1] + w * axes[0][1];
	verts[1].xyz[2] = 0;
	verts[1].st[0] = 1;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	verts[2].xyz[0] = verts[1].xyz[0] + h * axes[1][0];
	verts[2].xyz[1] = verts[1].xyz[1] + h * axes[1][1];
	verts[2].xyz[2] = 0;
	verts[2].st[0] = 1;
	verts[2].st[1] = 0;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	verts[3].xyz[0] = verts[2].xyz[0] - w * axes[0][0];
	verts[3].xyz[1] = verts[2].xyz[1] - w * axes[0][1];
	verts[3].xyz[2] = 0;
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;
}
