"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"attribute vec4 vertex;\n"
"attribute vec4 color;\n"
"attribute vec2 tex_coord;\n"
"attribute vec2 tex_coorda;\n"
"uniform mat4 mvp;\n"
"varying vec4 col;\n"
"varying vec2 coord_c;\n"
"varying vec2 coord_a;\n"
"void main()\n"
"{\n"
"   gl_Position = mvp * vertex;\n"
"   col = color;\n"
"   coord_c = tex_coord;\n"
"   coord_a = tex_coorda;\n"
"}\n"
