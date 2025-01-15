#pragma once

#define k_maximum_hs_unit_seat_mappings 65536

enum e_seat_mapping_bitfield : uint32
{
	seat_mapping_bitfield_seat_0 = FLAG(0),
	seat_mapping_bitfield_seat_1 = FLAG(1),
	seat_mapping_bitfield_seat_2 = FLAG(2),
	seat_mapping_bitfield_seat_3 = FLAG(3),
	seat_mapping_bitfield_seat_4 = FLAG(4),
	seat_mapping_bitfield_seat_5 = FLAG(5),
	seat_mapping_bitfield_seat_6 = FLAG(6),
	seat_mapping_bitfield_seat_7 = FLAG(7),
	seat_mapping_bitfield_seat_8 = FLAG(8),
	seat_mapping_bitfield_seat_9 = FLAG(9),
	seat_mapping_bitfield_seat_10 = FLAG(10),
	seat_mapping_bitfield_seat_11 = FLAG(11),
	seat_mapping_bitfield_seat_12 = FLAG(12),
	seat_mapping_bitfield_seat_13 = FLAG(13),
	seat_mapping_bitfield_seat_14 = FLAG(14),
	seat_mapping_bitfield_seat_15 = FLAG(15),
	seat_mapping_bitfield_seat_16 = FLAG(16),
	seat_mapping_bitfield_seat_17 = FLAG(17),
	seat_mapping_bitfield_seat_18 = FLAG(18),
	seat_mapping_bitfield_seat_19 = FLAG(19),
	seat_mapping_bitfield_seat_20 = FLAG(20),
	seat_mapping_bitfield_seat_21 = FLAG(21),
	seat_mapping_bitfield_seat_22 = FLAG(22),
	seat_mapping_bitfield_seat_23 = FLAG(23),
	seat_mapping_bitfield_seat_24 = FLAG(24),
	seat_mapping_bitfield_seat_25 = FLAG(25),
	seat_mapping_bitfield_seat_26 = FLAG(26),
	seat_mapping_bitfield_seat_27 = FLAG(27),
	seat_mapping_bitfield_seat_28 = FLAG(28),
	seat_mapping_bitfield_seat_29 = FLAG(29),
	seat_mapping_bitfield_seat_30 = FLAG(30),
	seat_mapping_bitfield_seat_31 = FLAG(31)
};

// max: k_maximum_hs_unit_seat_mappings
struct s_hs_unit_seat_mapping
{
	datum unit_tag_datum_index;
	e_seat_mapping_bitfield seats;
};
ASSERT_STRUCT_SIZE(s_hs_unit_seat_mapping, 8);