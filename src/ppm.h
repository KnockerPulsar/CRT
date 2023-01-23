#include <stdio.h>
#include "vec3.h"
#include "common.h"

#define RGB8_STRIDE 3
#define RGBA8_STRIDE 4
#define RGBA_CHANNELS 4

/*
 * Originally coded as RGB8. But OpenCL supports RGBA8 at a minimum so converted to that.
 */
class PPMImage {
	public:
		u8* data;
		int width, height;

		static PPMImage magenta(int w, int h) {
			PPMImage image;
			image.data = new u8[w * h * RGBA_CHANNELS];
			
			image.width = w;
			image.height = h;

			return image;
		}

		void write(const char* path) const {
			FILE* f = fopen(path, "w");

			if(f == NULL) {
				fprintf(stderr, "Failed to open file at %s for writing", path);
				return;
			}

			fprintf(f, "P3\n");
			fprintf(f, "%d %d\n" , width, height);
			fprintf(f, "255\n");


			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int index = (y * width + x) * RGBA8_STRIDE;
					u8 r = data[index + 0];
					u8 g = data[index + 1];
					u8 b = data[index + 2];
					fprintf(f, "%d %d %d\n", r, g, b);
				}	
			}

			fclose(f);
		}

		// Data should be an RGB8 pixel buffer
		void write_pixel_rgb_u8(int x, int y, u8 r, u8 g, u8 b, int samples_per_pixel) {
			int index = (y * width + x) * RGBA8_STRIDE;
			data[index + 0] = r;
			data[index + 1] = g;
			data[index + 2] = b;
		}
		
		void write_pixel_rgb_f32(int x, int y, float r, float g, float b, int samples_per_pixel) {
			u8 red 		= (u8) (clamp(r / (float)samples_per_pixel, 0.0f, 1.0f) * 255);
			u8 green 	= (u8) (clamp(g / (float)samples_per_pixel, 0.0f, 1.0f) * 255);
			u8 blue 	= (u8) (clamp(b / (float)samples_per_pixel, 0.0f, 1.0f) * 255);
		
			write_pixel_rgb_u8(x, y, red, green, blue, samples_per_pixel);
		}
		
		void write_pixel_rgb_vec3(int x, int y, Vec3 color, int samples_per_pixel) {
			write_pixel_rgb_f32(x, y, color.x, color.y, color.z, samples_per_pixel);
		}
};

