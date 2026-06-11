#version 130

uniform sampler2D A; // atlas
uniform sampler2D G; // glyphs
uniform sampler2D C; // colors
uniform sampler2D B; // bg colors
uniform vec2 R; // resolution
uniform vec2 S; // char size
uniform vec2 I; // grid size
uniform float T; // top row
uniform vec3 U; // cursor

vec4 tex(sampler2D samp, vec2 pix) { return texture(samp, pix / textureSize(samp, 0)); }

void main() {
//	gl_FragColor = tex(A, gl_FragCoord.xy); return;
	// in pixels, +.5 sample position included
	vec2 screen_pix = vec2(gl_FragCoord.x, R.y - gl_FragCoord.y);
	// do not draw partial glyps outside of the grid
	vec2 grid_texel = floor((screen_pix.xy - .5) / S);
	if (any(greaterThanEqual(grid_texel, I))) { gl_FragColor = vec4(0.); return; }

// Exact pixel sample coordinated (+.5) to read from the grid, offset by T ring buffer
	vec2 grid_sample_texel = grid_texel + .5 + vec2(0., T);
	vec4 char_loc = tex(G, grid_sample_texel);
//	gl_FragColor = char_loc * 16.; return;
	vec4 color = tex(C, grid_sample_texel);
	vec4 bg = tex(B, grid_sample_texel);

// Coordinates within a single on-screen grid char
	vec2 char_pix = mod(screen_pix.xy, S);
// GL coord system to GDI
	char_pix.y = S.y - char_pix.y;

	vec2 atlasGlyphSize = S;
	vec2 glyph_offset = char_loc.rg * 255. * atlasGlyphSize;

	vec2 atlas_pix = char_pix + glyph_offset;
	vec4 glyph = tex(A, atlas_pix);
//	vec4 glyph = tex(A, gl_FragCoord.xy);
	gl_FragColor = mix(bg, color, glyph.r);
	if (U.z > 0) {
		if (all(equal(grid_texel, U.xy))) {
			gl_FragColor.b = 1.;
		}
	}
}
