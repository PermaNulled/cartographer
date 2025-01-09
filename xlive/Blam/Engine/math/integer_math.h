#pragma once

union point2d
{
	int16 v[2];
	struct { int16 x, y; };
};
ASSERT_STRUCT_SIZE(point2d, sizeof(int16) * 2);

union short_bounds
{
	int16 v[2];
	struct { int16 lower, upper; };
};
ASSERT_STRUCT_SIZE(short_bounds, sizeof(int16) * 2);

union rectangle2d
{
	int16 v[4];
	struct { int16 top, left, bottom, right; };
	struct { int16 y0, x0, y1, x1; };
};
ASSERT_STRUCT_SIZE(rectangle2d, sizeof(int16) * 4);


static BLAM_MATH_INL void point2d_scale(point2d* point, int16 scale)
{
	point->v[0] *= scale;
	point->v[1] *= scale;
}

static BLAM_MATH_INL int16 rectangle2d_width(const rectangle2d* rect)
{
	return rect->right - rect->left;
}

static BLAM_MATH_INL int16 rectangle2d_height(const rectangle2d* rect)
{
	return rect->bottom - rect->top;
}

static BLAM_MATH_INL void rectangle2d_to_rect(const rectangle2d* rect2d, RECT* rect)
{
	const int16 width = rectangle2d_width(rect2d);
	const int16 height = rectangle2d_height(rect2d);
	*rect =
	{
		rect2d->left,
		rect2d->top,
		rect2d->left + width,
		rect2d->top + height
	};
	return;
}

static BLAM_MATH_INL void rectangle2d_scale(rectangle2d* rect, int16 scale)
{
	rect->v[0] *= scale;
	rect->v[1] *= scale;
	rect->v[2] *= scale;
	rect->v[3] *= scale;
}