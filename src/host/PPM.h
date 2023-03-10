#include <math.h>
#include <stdio.h>

#include "host/CLUtil.h"
#include "host/Utils.h"

#define RGB_CHANNELS 3
#define RGBA_CHANNELS 4

#define RGB_STRIDE 3
#define RGBA_STRIDE 4

/*
 * Originally coded as RGB8. But OpenCL supports RGBA8 at a minimum so converted to that.
 */
class PPMImage {
	public:
		float* data;
		int width, height;

		static PPMImage magenta(int w, int h) {
			PPMImage image;
			image.data = new float[w * h * RGB_CHANNELS];
			
			image.width = w;
			image.height = h;

			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					image.write_pixel_rgb_f32(x, y, 1, 0, 1);
				}	
			}

			return image;
		}

		void write(const char* path, int samples_per_pixel) const {
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
					int index = (y * width + x) * RGB_STRIDE;
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
			int index = (y * width + x) * RGB_STRIDE;
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

					write_pixel_rgb_f32(x, y, r, g, b);
				}
			}
		}
};

