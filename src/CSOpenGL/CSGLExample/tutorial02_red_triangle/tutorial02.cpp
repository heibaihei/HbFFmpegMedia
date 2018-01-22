#include "CSGLExample.h"
// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
using namespace glm;

#include <common/shader.hpp>
extern GLFWwindow* window;

#define CSGL_Demo_Tutorial02_Root_Path "/Users/zj-db0519/work/code/github/HbFFmpegMedia/src/CSOpenGL/CSGLExample/tutorial02_red_triangle"
int CSGL_Demo_Tutorial02(void)
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Tutorial 02 - Red triangle", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( CSGL_Demo_Tutorial02_Root_Path"/SimpleVertexShader.vertexshader", \
                                    CSGL_Demo_Tutorial02_Root_Path"/SimpleFragmentShader.fragmentshader" );

    /** 定点数据： 因为 g_vertex_buffer_data 数组不需要被修改，因此将其声明为const
     *           数组的每一行分别表示三角形每个顶点的x、y、z 坐标
     *  NDC 坐标，[-1.0, 1.0], 只提供x、y坐标时，position的z、w分量就会被设置为默认的0.0和1.0
     */
	static const GLfloat g_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f,
	};

    /**
     **  生成新缓存对象\ 绑定缓存对象\ 将顶点数据拷贝到缓存对象中
     */
	GLuint vertexbuffer;
    /**
     *  创建缓存对象并且返回缓存对象的标示符。它需要2个参数：第一个为需要创建的缓存数量，第二个为用于存储单一ID或多个ID的GLuint变量或数组的地址。
     */
	glGenBuffers(1, &vertexbuffer);
    /**
     *  当缓存对象创建之后，在使用缓存对象之前，我们需要将缓存对象连接到相应的缓存上: 2个参数：target与buffer
     *  target告诉VBO该缓存对象将保存顶点数组数据还是索引数组数据：GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY。任何顶点属性，如顶点坐标、纹理坐标、法线与颜色分量数组都使用GL_ARRAY_BUFFER。
     *  用于glDraw[Range]Elements()的索引数据需要使用GL_ELEMENT_ARRAY绑定。
     *  注意，target标志帮助VBO确定缓存对象最有效的位置，如有些系统将索引保存AGP或系统内存中，将顶点保存在显卡内存中。
     */
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    /**
     *  当缓存初始化之后，你可以使用glBufferData()将数据拷贝到缓存对象
     *  第一个参数target可以为GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY。
     *  size为待传递数据字节数量。第三个参数为源数据数组指针，如data为NULL，则VBO仅仅预留给定数据大小的内存空间。
     *  最后一个参数usage标志位VBO的另一个性能提示，它提供缓存对象将如何使用：static、dynamic或stream、与read、copy或draw。
     *      ”static“表示VBO中的数据将不会被改动（一次指定多次使用），
     *      ”dynamic“表示数据将会被频繁改动（反复指定与使用），
     *      ”stream“表示每帧数据都要改变（一次指定一次使用）。
     *      ”draw“表示数据将被发送到GPU以待绘制（应用程序到GL），
     *      ”read“表示数据将被客户端程序读取（GL到应用程序），
     *      ”copy“表示数据可用于绘制与读取（GL到GL）。
     *  注意，仅仅draw标志对VBO有用，copy与read标志对顶点/帧缓存对象（PBO或FBO）更有意义，如GL_STATIC_DRAW与GL_STREAM_DRAW使用显卡内存，
     *    GL_DYNAMIC使用AGP内存。_READ_相关缓存更适合在系统内存或AGP内存，因为这样数据更易访问
     *    将数据拷贝到VBO 缓存空间后，即可删除外部数据
     */
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	do{
        // http://blog.csdn.net/candycat1992/article/details/39676669
		// Clear the screen， 清空窗口
		glClear( GL_COLOR_BUFFER_BIT );

		// Use our shader
		glUseProgram(programID);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        /** 把需要的数据和shader程序中的变量关联在一起 */
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangle !， 发起OpenGL调用来请求渲染对象
		glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

		glDisableVertexAttribArray(0);

		// Swap buffers, 请求将图像绘制到窗口 
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO
    /**
     *  在VBO不再使用时，你可以使用glDeleteBuffers()删除一个VBO或多个VBO。在缓存对象删除之后，它的内容将丢失。
     */
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
