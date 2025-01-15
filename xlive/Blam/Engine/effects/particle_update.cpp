#include "stdafx.h"
#include "particle_update.h"

#include "particle_system.h"

void __cdecl particle_update_points_interpolate_hook(const real_point3d* previous_point, const real_point3d* target_point, real32 fractional_tick, real_point3d* out)
{
	// The engine would interpolate between the particle_emitters previous and current posisiton to find a rough estimate
	// of where it should be in the world to be as close as possible to the object
	// we can remove this and just force it to be in the previous point as that will always be
	// the interpolated position of the attached object as we let the engine go brrrr
	// this can be fired before the initial position of the emitter is set in some situations
	// there for instead of rendering the particle at the root origin do it at the target (initial) position instead
	// this is only really noticeable on 30 tick gameplay
	if (memcmp(previous_point, global_zero_vector3d, sizeof(real_point3d)) == 0)
		csmemcpy(out, target_point, sizeof(real_point3d));
	else
		csmemcpy(out, previous_point, sizeof(real_point3d));
}

void particle_update(real32 delta)
{
	s_data_iterator<c_particle_system> particle_system_it(get_particle_system_table());
	while (particle_system_it.get_next_datum())
	{
		c_particle_system* particle_system = particle_system_it.get_current_datum();
		if (particle_system->first_particle_index == NONE && !c_particle_system::frame_advance(particle_system, delta))
		{
			if (particle_system->parent_effect_index != NONE)
			{
				particle_system_remove_from_effects_cache(particle_system->parent_effect_index, particle_system_it.get_current_datum_index());
			}
			c_particle_system::destroy(particle_system_it.get_current_datum_index());
		}
	}
}

void apply_particle_update_patches()
{
	apply_particle_system_patches();

	//PatchCall(Memory::GetAddress(0xAA636, 0xB1C5D), particles_update);
  PatchCall(Memory::GetAddress(0x105380), particle_update_points_interpolate_hook);
}
