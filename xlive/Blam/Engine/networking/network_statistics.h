#pragma once

class c_network_window_statistics
{
	int32 m_window_size;
	uint8 gap_4[260];
	int32 field_108;
	int32 field_10C;

public:
	real32 average_values_in_window();
};
ASSERT_STRUCT_SIZE(c_network_window_statistics, 272);

class c_network_time_statistics
{
	uint8 gap_0[212];
};
ASSERT_STRUCT_SIZE(c_network_time_statistics, 212);