#include "filter_yuv.h"

FilterYUV::FilterYUV() {
    InitMatrix(matrix);//初始化为单位矩阵
}

FilterYUV::~FilterYUV() {

}

void FilterYUV::OnCreate() {
    vertexStr = GET_STR(
            attribute vec4 v_Position;
            attribute vec2 f_Position;
            varying vec2 ft_Position;
            uniform mat4 u_Matrix;
            void main() {
                ft_Position = f_Position;
                gl_Position = v_Position * u_Matrix;//将矩阵与顶点坐标进行相乘，用于图像旋转
            });

    fragmentStr = GET_STR(
            precision mediump float;
            varying vec2 ft_Position;
            uniform sampler2D sampler_y;
            uniform sampler2D sampler_u;
            uniform sampler2D sampler_v;
            void main() {//texture2D表示GPU将输入得图像纹理像素进行读取，读取到GPU的管线中,最后渲染出来
                float y;
                float u;
                float v;
                y = texture2D(sampler_y, ft_Position).x;
                u = texture2D(sampler_u, ft_Position).x - 0.5;
                v = texture2D(sampler_v, ft_Position).x - 0.5;

                vec3 rgb;
                rgb.r = y + 1.403 * v;
                rgb.g = y - 0.344 * u - 0.714 * v;
                rgb.b = y + 1.770 * u;
                gl_FragColor = vec4(rgb, 1);
            });

    program = CreateProgram(vertexStr, fragmentStr, &vShader, &fShader);
    LOGI("FilterYUV callback_SurfaceCreate GET_STR opengl program: %d", program);

    //获取着色器程序中的这个变量a_position，返回一个变量id，用于给这个变量赋值
    vPosition = glGetAttribLocation(program, "v_Position");//顶点坐标变量
    fPosition = glGetAttribLocation(program, "f_Position");//纹理坐标变量
    u_matrix = glGetUniformLocation(program, "u_Matrix");//着色器矩阵变量
    sampler_y = glGetUniformLocation(program, "sampler_y");//2D纹理变量
    sampler_u = glGetUniformLocation(program, "sampler_u");//2D纹理变量
    sampler_v = glGetUniformLocation(program, "sampler_v");//2D纹理变量

    glGenTextures(3, samplers);
    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_2D, samplers[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);//解绑纹理
    }
    LOGI("FilterYUV::onCreate end");
}

void FilterYUV::OnChange(int width, int height) {
    LOGI("FilterYUV::onChange in");
    surface_width = width;
    surface_height = height;
    glViewport(0, 0, width, height);
    LOGI("FilterYUV::onChange end");
}

void FilterYUV::OnDraw() {
    LOGI("FilterYUV::onDraw in");
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);//指定刷屏颜色  1:不透明  0：透明
    glClear(GL_COLOR_BUFFER_BIT);//将刷屏颜色进行刷屏，但此时仍然处于后台缓冲中，需要swapBuffers交换到前台界面显示

    glUseProgram(program);//使用着色器程序

    glUniformMatrix4fv(u_matrix, 1, GL_FALSE, matrix);//给矩阵变量赋值

    //渲染时顶点赋值操作
    glEnableVertexAttribArray(vPosition);//使能这个着色器变量

    /*给着色器的顶点顶点变量赋值
     * 第一个参数是着色器的变量id,第二个参数是每个顶点两个值，第三个参数是值的类型，第四个参数表示是否
    归一化，已经有顶点参数，无需自动归一化。第五个参数表示每个顶点的跨度(这里每个顶点跨8个字节)，
     第六个参数表示顶点数组
     */
    glVertexAttribPointer(vPosition, 2, GL_FLOAT, false, 8, vertexs);

    glEnableVertexAttribArray(fPosition);
    glVertexAttribPointer(fPosition, 2, GL_FLOAT, false, 8, fragments);

    if ((yuv_width > 0) && (yuv_height > 0)) {
        if (y_data != NULL) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, samplers[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, yuv_width, yuv_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, y_data);
            glUniform1i(sampler_y, 0);
        }
        if (u_data != NULL) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, samplers[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, yuv_width / 2, yuv_height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, u_data);
            glUniform1i(sampler_u, 1);
        }
        if (v_data != NULL) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, samplers[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, yuv_width / 2, yuv_height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, v_data);
            glUniform1i(sampler_v, 2);
        }

        /*opengl绘制
    * 绘制三角形，第二个参数表示从索引0开始，绘制三个顶点
    */
//    glDrawArrays(GL_TRIANGLES, 0, 3);//绘制三角形
//    glDrawArrays(GL_TRIANGLES, 0, 6);//绘制四边形
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);//绘制四边形
        glBindTexture(GL_TEXTURE_2D, 0);
        LOGI("FilterYUV::onDraw end");
    }
}

void FilterYUV::_SetMatrix(int width, int height) {
    LOGI("FilterYUV::setMatrix in");
//    initMatrix(matrix);
    //这里是矩阵投影操作
    //屏幕720*1280 图片:517*685
    if (yuv_width > 0 && yuv_height > 0) {
        float screen_r = 1.0 * width / height;
        float picture_r = 1.0 * yuv_width / yuv_height;
        if (screen_r > picture_r) {//图片宽度缩放
            LOGI("pic scale width");
            float r = width / (1.0 * height / yuv_height * yuv_width);
            LOGI("pic scale width r: %f", r);
            OrthoM(-r, r, -1, 1, matrix);
        } else {//图片宽的比率大于屏幕，则宽进行直接覆盖屏幕，而图片高度缩放
            LOGI("pic scale height");
            float r = height / (1.0 * width / yuv_width * yuv_height);
            LOGI("pic scale height r: %f", r);
            OrthoM(-1, 1, -r, r, matrix);
        }
    }
    LOGI("FilterYUV::setMatrix end");
}

void FilterYUV::SetYuvData(int width, int height, void *y, void *u, void *v) {
    if ((width > 0) && (height > 0)) {
        if ((yuv_width != width) || (yuv_height != height)) {
            yuv_width = width;
            yuv_height = height;
            if (y_data != NULL) {
                free(y_data);
                y_data = NULL;
            }
            if (u_data != NULL) {
                free(u_data);
                u_data = NULL;
            }
            if (v_data != NULL) {
                free(v_data);
                v_data = NULL;
            }
            y_data = malloc(yuv_width * yuv_height);
            u_data = malloc(yuv_width * yuv_height / 4);
            v_data = malloc(yuv_width * yuv_height / 4);

            _SetMatrix(surface_width, surface_height);
        }
        memcpy(y_data, y, yuv_width * yuv_height);
        memcpy(u_data, u, yuv_width * yuv_height / 4);
        memcpy(v_data, v, yuv_width * yuv_height / 4);
    }
}

void FilterYUV::DestroySource() {
    yuv_width = 0;
    yuv_height = 0;
    if (y_data != NULL) {
        free(y_data);
        y_data = NULL;
    }
    if (u_data != NULL) {
        free(u_data);
        u_data = NULL;
    }
    if (v_data != NULL) {
        free(v_data);
        v_data = NULL;
    }
}

void FilterYUV::Destroy() {
    glDeleteTextures(3, samplers);
    glDetachShader(program, vShader);
    glDetachShader(program, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    glDeleteProgram(program);
}


