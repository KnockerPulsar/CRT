#pragma once
#include "common/common_defs.h"

#ifndef OPENCL
#include "host/Utils.h"
#include "host/CLUtil.h"
#include <cstdlib>
#include <algorithm>
#include <optional>
#endif

typedef enum TextureType { TextureSolidColor, TextureChecker } TextureType;

SHARED_STRUCT_START(CheckerTexture) {
  float inv_scale;
  uint even_texture_index, odd_texture_index;
} SHARED_STRUCT_END(CheckerTexture);

SHARED_STRUCT_START(Texture) {
	TextureType type;

  // Might need it for image textures.
	/* uint texture_index;  	// For the texture data we'll be sampling from */
	union {
		float3 as_solid_color;
    CheckerTexture as_checker_texture;
	};

#ifndef OPENCL
  public:                                                              
  inline static std::vector<Texture> instances;                           

  static uint SolidColor(float3 albedo) {
    return Texture::push_back({
        .type = TextureSolidColor,
        .as_solid_color = {albedo}
    });
  }

  static uint CheckerTexture(float scale, uint evenIndex, uint oddIndex) {
    struct CheckerTexture checker = {
      .inv_scale = 1/scale,
      .even_texture_index = evenIndex,
      .odd_texture_index = oddIndex,
    };

    Texture c = (Texture){ .type = TextureChecker, .as_checker_texture = checker };
    return Texture::push_back(c);
  }

  static uint CheckerTexture(float scale, float3 evenColor, float3 oddColor) {
    uint evenIndex = SolidColor(evenColor);
    uint oddIndex = SolidColor(oddColor);

    return CheckerTexture(scale, evenIndex, oddIndex);
  }

  static std::optional<uint> getId(const Texture& tex) {
    auto texIter = std::find(instances.begin(), instances.end(), tex); 
                                                                       
    if(texIter != instances.end()) {                                   
      return std::make_optional(texIter - instances.begin());
    }                                                                  

    return std::nullopt;
  }
                                                                       
  static uint push_back(Texture tex) {                              
    auto optId = Texture::getId(tex);

    if(optId.has_value()) {
      return optId.value();
    }

    return createAndPush(tex);                                         
  }                                                                    
                                                                       
  bool operator==(const Texture& other) const {
    if(type != other.type) return false;
    switch(type) {
      case TextureSolidColor: return float3Equals(as_solid_color, other.as_solid_color);
      case TextureChecker: {
         auto self = as_checker_texture;
         auto oth = other.as_checker_texture;

         return (self.inv_scale == oth.inv_scale) &&
                (self.even_texture_index == oth.even_texture_index) &&
                (self.odd_texture_index == oth.odd_texture_index);

      } break;
      default:  {
          std::cerr << "Unimplemented texture type\n";
          std::exit(EXIT_FAILURE);
      } break;
    }
  }
                                                                       
  private:                                                             
  static uint createAndPush(Texture tex) {                          
    uint id = instances.size();
    instances.push_back(tex);                                          
    return id;                                                     
  }                                                                    

#endif
} SHARED_STRUCT_END(Texture);


#ifdef OPENCL
#include "device/cl_util.cl"

// Forward declarations
float3 texture_sample(uint texture_index, global const Texture* textures, float2 uv, float3 p);

float3 texture_solid_color_sample(global const Texture* tex) {
	return tex->as_solid_color;
}

uint texture_checker_texture_sample(global const Texture* tex, float3 p) {
  CheckerTexture self = tex->as_checker_texture;
  int x_integer = (int)(floor(p.x * self.inv_scale));
  int y_integer = (int)(floor(p.y * self.inv_scale));
  int z_integer = (int)(floor(p.z * self.inv_scale));

  bool is_even = (x_integer + y_integer + z_integer) % 2 == 0;
  uint tex_to_sample = is_even? self.even_texture_index : self.odd_texture_index;

  return tex_to_sample;
}

// tex is the texture we're currently evaluating
// textures is the whole array of textures for "recursive" textures like checker textures 
// uv is the uv of the hit point
// p is the position of the hit point in world space.
float3 texture_sample(uint texture_index, global const Texture* textures, float2 uv, float3 p) {
#define MAX_TEXTURE_DEPTH 4

  uint stack[MAX_TEXTURE_DEPTH];
  stack[0] = texture_index;
  uint stack_ptr = 1;

  while(1) {
    POP_STACK(texture_index);
    global const Texture* tex = &textures[texture_index];

    switch(tex->type) {
      case TextureSolidColor: return texture_solid_color_sample(tex);
      case TextureChecker: {
        uint tex_to_sample = texture_checker_texture_sample(tex, p);
        PUSH_STACK(tex_to_sample);
     } break;
      default: return (float3)(0);
    }
  }
  return (float3)(.8, .2, .8);
}
#endif
