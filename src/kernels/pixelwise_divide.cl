kernel void pixelwise_divide(read_write image2d_t img, float scale) {
	int x = get_global_id(0);
	int y = get_global_id(1);
	int2 pos = (int2)(x, y);


	float4 old_value = read_imagef(img, pos);
	float4 new_value = old_value / scale;

	new_value.w = 1.0;

	write_imagef(img, pos, new_value);
}
