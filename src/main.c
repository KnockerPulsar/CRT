#include <bits/types/FILE.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define u8 uint8_t 
#define RGB8_STRIDE 3

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

// Data should be an RGB8 pixel buffer
void to_ppm(u8* data, int width, int height, const char* path) {
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
			int index = (y * width + x) * RGB8_STRIDE;
			u8 r = data[index + 0];
			u8 g = data[index + 1];
			u8 b = data[index + 2];
			fprintf(f, "%d %d %d\n", r, g, b);
		}	
	}

	fclose(f);
}

void write_pixel_rgb_u8(int x, int y, u8* data, int width, int height, u8 r, u8 g, u8 b) {
	int index = (y * width + x) * RGB8_STRIDE;
	data[index + 0] = r;
	data[index + 1] = g;
	data[index + 2] = b;
}

void write_pixel_rgb_f32(int x, int y, u8* data, int width, int height, float r, float g, float b) {
	u8 red = (u8) (clamp(r, 0.0f, 1.0f) * 255);
	u8 green = (u8) (clamp(g, 0.0f, 1.0f) * 255);
	u8 blue = (u8) (clamp(b, 0.0f, 1.0f) * 255);

	write_pixel_rgb_u8(x, y, data, width, height, red, green, blue);
}

int main(void) {
	int width = 100, height = 100;
	u8* pixels = (u8*) malloc(width * height * 3 * sizeof(u8));

	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float u = (float)(x) / width;
			float v = (float)(y) / height;
			write_pixel_rgb_f32(x, y, pixels, width, height, u, v, 0);
		}	
	}

	to_ppm(pixels, width, height, "./test.ppm");

	return 1;
}
