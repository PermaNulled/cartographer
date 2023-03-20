#pragma once
#include "Blam/Cache/DataTypes/BlamTag.h"
#include "Blam/Cache/DataTypes/StringID.h"
#include "Blam/Engine/math/real_math.h"

struct s_event_scenery_placement
{
	short type;
	short variant_id;
	real_point3d position;
	real_vector3d rotation;
	float scale;
};

const string_id new_elite_head_marker(0xFEE01234);

void add_special_event_markers();
void replace_fp_and_3p_models_from_weapon(datum weapon_datum, datum fp_model_datum, datum _3p_model_datum);
void add_hat_to_model(datum player_hlmt_datum, datum hat_scenery_datum, bool is_elite = false);
void add_hat_and_beard_to_model(datum player_hlmt_datum, datum hat_scenery_datum, datum beard_scenery_datum, bool is_elite = false);
