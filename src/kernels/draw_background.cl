kernel void draw_background(read_write image2d_t img, uint samples) {
	int2 pos = { get_global_id(0), get_global_id(1) };
	float2 dims  = { get_image_width(img), get_image_height(img) };
	float2 n_pos =  convert_float2(pos) / dims;

	float a = 1-n_pos.y;

	float3 background_gradient = ((1.0f-a) * (float3)(1, 1, 1) + a * (float3)(0.5, 0.7, 1.0));
	float4 new_color = (float4)(background_gradient, 1.0f);

	write_imagef(img, pos, new_color);
}
