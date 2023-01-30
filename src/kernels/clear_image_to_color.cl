kernel void clear_image_to_color(write_only image2d_t img, float4 color) {
	int2 pos = {get_global_id(0), get_global_id(1)};
	write_imagef(img, pos, color);
}
