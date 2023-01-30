kernel void pixelwise_add(read_write image2d_t accum_img, read_only image2d_t source_img) {
	int2 pos = { get_global_id(0), get_global_id(1) };
	float4 source_pixel = read_imagef(source_img, pos);
	float4 accum_pixel = read_imagef(accum_img, pos) + source_pixel;

	write_imagef(accum_img, pos, accum_pixel);
}
