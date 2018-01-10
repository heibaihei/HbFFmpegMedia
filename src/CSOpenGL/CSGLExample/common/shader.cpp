#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "shader.hpp"

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders glCreateShader()创建一个着色器对象（shader object），并返回其ID
    // GL_VERTEX_SHADER和GL_FRAGMENT_SHADER，表示顶点着色器和片元着色器;
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
    // 定点着色器的源码，文件内容被保存在 VertexShaderCode 其中
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
    /** glShaderSource  给着色器对象提供源代码
     * shader：着色器对象。
     * count：string包含的字符串个数。此处只用了一个字符串表示着色器源代码，因此传入1。
     * string：一个GLchar二级指针，可以理解为一个字符串数组（数组的每个元素都是一个字符串），组合成着色器源代码。这里传入source的地址&source，表示该数组（虽然只有一个元素）。
     * length：有些复杂，暂不解释。这里直接传入nullptr，表示每一个字符串（这里只有一个）都以空字符结尾。
     */
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    /**
     *  编译shader。注意，着色器的编译和一般编程语言的编译类似，但有不同。着色器在程序的运行时间（runtime）编译;
     */
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
    /** 对于glGetShaderiv()，pname为GL_COMPILE_STATUS时，*param将为GL_TRUE或GL_FALSE表示编译是否成功； */
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



    // Link the program; 创建一个着色器程序
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
    /**
     * glAttachShader(GLuint program, GLuint shader)将shader与program关联。
     * 这里我们调用了两次glAttachShader()，分别将顶点着色器（vShader）、片元着色器（fShader）和着色器对象关联。
     */
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
    
    /**
     * 关联完着色器后，需要使用glLinkProgram()链接着色器程序的着色器对象，这类似于编译器的链接（linking）。
     * 编译器的链接将源代码文件、.lib文件链接成一个.exe，OpenGL将着色器链接成一个着色器程序。
     */
	glLinkProgram(ProgramID);

    /** Check the program
     *  两个函数分别用来获取着色器和着色器程序的一些信息，并且该信息可以用一个整数表达（结尾的iv，i表示GLint，v表示指针）。
     *  第一个参数是相应的对象；第二个参数是要获取的信息类型，对于glGetShaderiv()，GL_COMPILE_STATUS表示着色器编译情况，
     *  对于glGetProgramiv()，GL_LINK_STATUS表示着色器程序链接情况。第三个参数是一个GLint指针，用于存储相应的信息。
     *  对于glGetProgramiv()，pname为GL_LINK_STATUS时，*param也是GL_TRUE或GL_FALSE表示链接是否成功。因此status为GL_TRUE时，就说明成功
     */
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	/**
     *  链接完毕，两个着色器就不需要了，因此应该将它们删除。glDeleteShader()用于删除着色器。最后返回着色器程序program
     */
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
    /** 删除着色器 */
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}


