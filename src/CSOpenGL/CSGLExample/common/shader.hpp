#ifndef SHADER_HPP
#define SHADER_HPP

/**
 *  用于读取着色器源代码文件，并创建相应的着色器程序（shader program）
 *  分别表示顶点着色器和片元着色器的文件名
 */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);

#endif
