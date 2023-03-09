#pragma once
#include "Blam/Cache/TagGroups/scenario_structure_bsp_definition.hpp"

struct s_game_cluster_bit_vectors
{
	int cluster_bitvector;
};

#pragma pack(push, 1)
struct s_structure_globals
{
	bool cluster_marker_initialized;
	byte pad[2];
	int cluster_index;
	int cluster_instances[512];
	bool instanced_geometry_marker_initialized;
	byte pad1[3];
	int instanced_geometry_index;
	int instanced_geometry_instances[1024];

	static s_structure_globals* get()
	{
		return Memory::GetAddress<s_structure_globals*>(0x4D12A1, 0x4F5159);
	}
};
#pragma pack(pop)

__int16 get_global_structure_bsp_index();
s_scenario_structure_bsp_group_definition* get_global_structure_bsp();

__int16 structure_clusters_in_bitvector(const s_game_cluster_bit_vectors* cluster_bitvector, short maximum_clusters, short* intersected_indices, short* out_actual_cluster_count);
__int16 __cdecl structure_clusters_in_sphere_recursive(
	const short cluster_reference,
	const real_point3d* position,
	const float radius,
	const short cluster_count,
	short* intersected_indices);
short structure_clusters_in_sphere(__int16 cluster_reference,
	const real_point3d* position,
	const float radius,
	const short cluster_count,
	short* intersected_indices,
	short* intersected_references);

