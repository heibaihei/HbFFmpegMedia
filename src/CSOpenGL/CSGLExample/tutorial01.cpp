#include "CSGLExample.h"

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW [一定要先包含glew.h，再包含glfw3.h; glfw3.h会包含gl.h，而这是glew.h不允许的]
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
using namespace glm;

extern GLFWwindow* window;

int CSGL_Demo_Tutorial01(void)
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

    /**
     *  配置环境，限制对应支持的版本:
     *  使用glfwWindowHint()可以设置一些关于窗口的选项:
     *     参数1:是我们要设置的hint的名字，使用GLFW常量（以GLFW_开头）指定；
     *     参数2:是我们要把该hint设置成的值，该值随要设置的hint而异
     */
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed

	/**
     *  Open a window and create its OpenGL context
     *  第一个、第二个是窗口的宽和高，以像素为单位，这里分别是800和600；
     *  第三个是窗口标题，这里是"First window"；
     *  第四个和第五个参数可以忽略，直接传入nullptr；
     */
	window = glfwCreateWindow( 800, 600, "First window", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
    
    /** [重点] 接下来的画图都会画在我们刚刚创建的窗口上 */
	glfwMakeContextCurrent(window);

    /** 把glewExperimental设置为GL_TRUE，这样GLEW就会使用更加新的方法管理OpenGL */
    glewExperimental = GL_TRUE;
	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	/**
     *  Dark blue background, 以gl开头的，是OpenGL的函数
     *  glClearColor用来设置窗口被清除时的颜色，也就是背景颜色。前3个参数分别是背景颜色的R、G、B分量，范围是0~1；
     *  第4个是alpha值表示透明度，这里只是设为1.0f，表示不透明
     **/
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    /** 游戏循环 */
	do{
		/**
         *  Clear the screen. It's not mentioned before Tutorial 02, but it can cause flickering, so it's there nonetheless.
         *  将会清除当前窗口，把所有像素的颜色都设置为前面所设置的清除颜色
         *  GL_COLOR_BUFFER_BIT是OpenGL定义的常量，表示清除颜色缓存;
         */
		glClear( GL_COLOR_BUFFER_BIT );

		// Draw nothing, see you in tutorial 2 !

		
		/**
         * Swap buffers
         * glfwSwapBuffers()用来交换窗口的两个颜色缓冲（color buffer）。
         * 这个概念叫做双缓冲（double buffer）:
         *    如果不使用双缓冲，就可能会出现闪屏现象，因为绘制一般不是一下子就绘制完毕的，而是从左到右、从上到下地绘制。
         *    为了避免这个问题，一般会使用双缓冲，前缓冲（front buffer）是最终的图像，而程序会在后缓冲（back buffer）上绘制。
         *    后缓冲绘制完毕后，就交换两个缓冲，这样就不会有闪屏的问题了。
         */
		glfwSwapBuffers(window);
        
        /**
         *  glfwPollEvents()用来检查是否有事件被触发，例如点击关闭按钮、点击鼠标、按下键盘，等等
         */
		glfwPollEvents();

        /** glfwGetKey()用来判断一个键是否按下。 */
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        /**
         *  glfwWindowShouldClose()检查窗口是否需要关闭
         */
	} // Check if the ESC key was pressed or the window was closed
	while( glfwWindowShouldClose(window) == 0 );

	/**
     *  Close OpenGL window and terminate GLFW
     *  释放前面所申请的资源
     */
	glfwTerminate();

	return 0;
}
