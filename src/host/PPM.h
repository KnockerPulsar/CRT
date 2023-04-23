#include <math.h>
#include <stdio.h>

#include "host/CLUtil.h"
#include "host/Utils.h"

#define RGBA_CHANNELS 4
#define RGBA_STRIDE 4

/*
 * Originally coded as RGB8. But OpenCL supports RGBA8 at a minimum so converted to that.
 */
class PPMImage {
	public:
		cl_mem cl_image;
		const int width, height;
	
	private:
		cl_command_queue queue;
		cl_context context;

		float* data;

	public:
		PPMImage(cl_command_queue& queue, cl_context& context, int w, int h, float3 color) : 
			width(w), height(h), queue(queue), context(context)
		{

			cl_int err;

			cl_image_format format;
			format.image_channel_order = CL_RGBA;
			format.image_channel_data_type = CL_FLOAT;

			cl_image_desc desc;
			desc.image_type = CL_MEM_OBJECT_IMAGE2D;
			desc.image_width = this->width;
			desc.image_height = this->height;
			desc.image_row_pitch = 0;
			desc.image_slice_pitch = 0;
			desc.num_mip_levels = 0;
			desc.num_samples = 0;
			desc.buffer = NULL;

			this->cl_image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr, &err);
			clErr(err);

			// OpenCL doesn't support RGB + Floats
			// So we use RGBA + Floats 
			// Also, reading and writing to and from the device is quite a bit easier.
			this->data = new float[w * h * RGBA_CHANNELS];
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					this->write_pixel_rgb_f32(x, y, color.s[0], color.s[1], color.s[2]);
				}	
			}
		}

		static PPMImage magenta(cl_command_queue& queue, cl_context& context, int w, int h) {
			return PPMImage(queue, context, w, h, (float3){1, 0, 1});
		}

		static PPMImage black(cl_command_queue& queue, cl_context& context, int w, int h) {
			return PPMImage(queue, context, w, h, (float3){0});
		}

		void write_to_device() const {
			const std::array<size_t, 3> origin = {0, 0, 0};
			const std::array<size_t, 3> region = {(size_t)this->width, (size_t)this->height, 1};
			
			clErr(clEnqueueWriteImage(this->queue, this->cl_image, CL_TRUE, origin.data(), region.data(), 0, 0, (void*)this->data, 0, NULL, NULL));
		}

		void read_from_device() {
			const std::array<size_t, 3> origin = {0, 0, 0};
			const std::array<size_t, 3> region = {(size_t)this->width, (size_t)this->height, 1};
			
			clErr(clEnqueueReadImage(this->queue, this->cl_image, CL_TRUE, origin.data(), region.data(), 0, 0, this->data, 0, NULL, NULL));
		}

		void write_to_file(const char* path, int samples_per_pixel) const {
			FILE* f = fopen(path, "w");

			if(f == NULL) {
				fprintf(stderr, "Failed to open file at %s for writing", path);
				return;
			}

			fprintf(f, "P3\n");
			fprintf(f, "%d %d\n" , width, height);
			fprintf(f, "255\n");

			float gamma_scale = 1.0f / samples_per_pixel;

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int index = (y * width + x) * RGBA_STRIDE;
					float r = data[index + 0];
					float g = data[index + 1];
					float b = data[index + 2];

					r = std::clamp(sqrt(gamma_scale * r), 0.0f, 0.999f);
					g = std::clamp(sqrt(gamma_scale * g), 0.0f, 0.999f);
					b = std::clamp(sqrt(gamma_scale * b), 0.0f, 0.999f);
					
					u8 r_u8 = 256 * r;
					u8 g_u8 = 256 * g;
					u8 b_u8 = 256 * b;

					fprintf(f, "%d %d %d\n", r_u8, g_u8, b_u8);
				}	
			}

			fclose(f);

			printf("Image successfully written at %s\n", path);
		}

		void write_pixel_rgb_f32(int x, int y, float r, float g, float b) {
			int index = (y * width + x) * RGBA_STRIDE;
			data[index + 0] = r;
			data[index + 1] = g;
			data[index + 2] = b;
		}

		void from_rgb_f32(float* new_data) {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int index = (y * width + x) * RGBA_STRIDE;
					float r = new_data[index + 0];
					float g = new_data[index + 1];
					float b = new_data[index + 2];

					/* if (std::isnan(r) || std::isnan(g) || std::isnan(b)) { */
					/* 	printf("NAN Encountered at %d, %d\n", x, y); */
					/* } */

					write_pixel_rgb_f32(x, y, r, g, b);
				}
			}
		}
};

