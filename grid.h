// Generated with Shader Minifier 1.6.0 (https://github.com/laurentlb/Shader_Minifier/)
#ifndef GRID_H_
# define GRID_H_

const char *grid_glsl =
 "#version 130\n"
 "uniform sampler2D A,G,C,B;"
 "uniform vec2 R,S,I;"
 "uniform float T;"
 "uniform vec3 U;"
 "vec4 f(sampler2D A,vec2 S)"
 "{"
   "return texture(A,S/textureSize(A,0));"
 "}"
 "void main()"
 "{"
   "vec2 r=vec2(gl_FragCoord.x,R.y-gl_FragCoord.y),v=floor((r.xy-.5)/S);"
   "if(any(greaterThanEqual(v,I)))"
     "{"
       "gl_FragColor=vec4(0);"
       "return;"
     "}"
   "vec2 u=v+.5+vec2(0,T);"
   "vec4 m=f(G,u),y=f(C,u),a=f(B,u);"
   "u=mod(r.xy,S);"
   "u.y=S.y-u.y;"
   "u+=m.xy*255.*S;"
   "m=f(A,u);"
   "gl_FragColor=mix(a,y,m.x);"
   "if(U.z>0)"
     "if(all(equal(v,U.xy)))"
       "gl_FragColor.z=1.;"
 "}";

#endif // GRID_H_
