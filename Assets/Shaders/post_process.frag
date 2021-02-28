#version 330
layout(location = 0) out vec4 out_color;

in vec2 f_texcoord;

uniform sampler2D uniform_texture;
uniform sampler2D uniform_shadow_map;

uniform sampler2D uniform_tex_pos;
uniform sampler2D uniform_tex_normal;
uniform sampler2D uniform_tex_albedo;
uniform sampler2D uniform_tex_mask;
uniform sampler2D uniform_tex_depth;

uniform int shoot_flag;
uniform int hit_flag;
uniform float gamma = 2.2;

// for perspective projection only
float linearize(const in float depth)
{
	float near = 0.1;
	float far = 30.0;
    float z = depth * 2.0 - 1.0; // [0, 1] -> [-1, 1]
    return ((2.0 * near * far) / (far + near - z * (far - near))) / far;
}

vec3 reinhard(vec3 hdrColor)
{
	return hdrColor / (hdrColor + vec3(1.0));
}

vec3 exposure(vec3 hdrColor)
{
	return vec3(1.0) - exp(-hdrColor * 1.0); // multiplication value is exposure
}

vec3 hdr()
{
	vec3 hdrColor = texture(uniform_texture, f_texcoord).rgb;

	// reinhard tone mapping
	vec3 mappedColor = reinhard(hdrColor);

	// exposure tone mapping
	//vec3 mappedColor = exposure(hdrColor);

	mappedColor = pow(mappedColor, vec3(1.0/gamma));

	return mappedColor;
}

//#define PREVIEW_SHADOW_MAP
#define CROSS_HAIR
#define HDR
#define SHOOT

void main(void)
{
	out_color.xyz = texture2D(uniform_texture, f_texcoord).rgb;

#ifdef PREVIEW_SHADOW_MAP
	out_color = vec4(vec3(linearize(texture2D(uniform_shadow_map, f_texcoord).r)), 1.0);
#endif

#ifdef CROSS_HAIR
	ivec2 halfRes = ivec2(textureSize(uniform_texture, 0) / 2);
	ivec2 pixel = ivec2(gl_FragCoord.xy);

	if(((pixel.x == halfRes.x) || (pixel.y == halfRes.y)) && 
	((pixel.x < halfRes.x + 15) && (pixel.y < halfRes.y + 15)) && 
	((pixel.x > halfRes.x - 15) && (pixel.y > halfRes.y - 15)))
	{
		float g = hit_flag > 0 ? 0.0 : 1.0;
		float b = hit_flag > 0 ? 0.0 : 1.0;
		out_color = vec4(1.0, g, b, 1.0);
	}
#endif

#ifdef HDR
	out_color *= vec4(hdr(), 1.0); // looks too bright if it's not multiplied
#endif

#ifdef SHOOT
	if (shoot_flag > 0)
	{
		vec2 uv = f_texcoord;
		float r = 0.1;
		vec2 center = vec2(0.5);
		vec2 pos = uv - center;
		float dist = sqrt(pos.x * pos.x + pos.y * pos.y);
		if (dist < r) out_color *= vec4(0.96, 0.96, 0.7, 1.0);
	}
#endif
	
}