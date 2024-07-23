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
    vPosition = glGetAttribLocation(program, "v_Position");
    fPosition = glGetAttribLocation(program, "f_Position");
    u_matrix = glGetUniformLocation(program, "u_Matrix");
    sampler_y = glGetUniformLocation(program, "sampler_y");
    sampler_u = glGetUniformLocation(program, "sampler_u");
    sampler_v = glGetUniformLocation(program, "sampler_v");

    glGenTextures(3, samplers);
    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_2D, samplers[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);//解绑纹理
    }
    LOGI("FilterYUV OnCreate end");
}

void FilterYUV::OnChange(int surface_width, int surface_height) {
    LOGI("FilterYUV OnChange in");
    m_surface_width = surface_width;
    m_surface_height = surface_height;
    glViewport(0, 0, surface_width, surface_height);
    LOGI("FilterYUV OnChange end");
}

void FilterYUV::OnDraw() {
    LOGI("FilterYUV OnDraw in");
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

    if ((m_yuv_width_ > 0) && (m_yuv_height_ > 0)) {
        if (m_y_data_ != NULL) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, samplers[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_yuv_width_, m_yuv_height_, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_y_data_);
            glUniform1i(sampler_y, 0);
        }
        if (m_u_data_ != NULL) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, samplers[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_yuv_width_ / 2, m_yuv_height_ / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_u_data_);
            glUniform1i(sampler_u, 1);
        }
        if (m_v_data_ != NULL) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, samplers[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_yuv_width_ / 2, m_yuv_height_ / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_v_data_);
            glUniform1i(sampler_v, 2);
        }

        /*opengl绘制
    * 绘制三角形，第二个参数表示从索引0开始，绘制三个顶点
    */
//    glDrawArrays(GL_TRIANGLES, 0, 3);//绘制三角形
//    glDrawArrays(GL_TRIANGLES, 0, 6);//绘制四边形
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);//绘制四边形
        glBindTexture(GL_TEXTURE_2D, 0);
        LOGI("FilterYUV OnDraw end");
    }
}

void FilterYUV::DestroySource() {
    m_yuv_width_ = 0;
    m_yuv_height_ = 0;
    if (m_y_data_ != NULL) {
        free(m_y_data_);
        m_y_data_ = NULL;
    }
    if (m_u_data_ != NULL) {
        free(m_u_data_);
        m_u_data_ = NULL;
    }
    if (m_v_data_ != NULL) {
        free(m_v_data_);
        m_v_data_ = NULL;
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

void FilterYUV::SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data) {
    if ((yuv_width > 0) && (yuv_height > 0)) {
        if ((m_yuv_width_ != yuv_width) || (m_yuv_height_ != yuv_height)) {
            m_yuv_width_ = yuv_width;
            m_yuv_height_ = yuv_height;
            if (m_y_data_ != NULL) {
                free(m_y_data_);
                m_y_data_ = NULL;
            }
            if (m_u_data_ != NULL) {
                free(m_u_data_);
                m_u_data_ = NULL;
            }
            if (m_v_data_ != NULL) {
                free(m_v_data_);
                m_v_data_ = NULL;
            }
            m_y_data_ = malloc(m_yuv_width_ * m_yuv_height_);
            m_u_data_ = malloc(m_yuv_width_ * m_yuv_height_ / 4);
            m_v_data_ = malloc(m_yuv_width_ * m_yuv_height_ / 4);

            _SetMatrix(m_surface_width, m_surface_height);
        }
        memcpy(m_y_data_, y_data, m_yuv_width_ * m_yuv_height_);
        memcpy(m_u_data_, u_data, m_yuv_width_ * m_yuv_height_ / 4);
        memcpy(m_v_data_, v_data, m_yuv_width_ * m_yuv_height_ / 4);
    }
}

void FilterYUV::_SetMatrix(int width, int height) {
    LOGI("FilterYUV _SetMatrix in");
//    initMatrix(matrix);
    //这里是矩阵投影操作
    //屏幕720*1280 图片:517*685
    if (m_yuv_width_ > 0 && m_yuv_height_ > 0) {
        float screen_r = 1.0 * width / height;
        float picture_r = 1.0 * m_yuv_width_ / m_yuv_height_;
        if (screen_r > picture_r) {//图片宽度缩放
            LOGI("pic scale width");
            float r = width / (1.0 * height / m_yuv_height_ * m_yuv_width_);
            LOGI("pic scale width r: %f", r);
            OrthoM(-r, r, -1, 1, matrix);
        } else {//图片宽的比率大于屏幕，则宽进行直接覆盖屏幕，而图片高度缩放
            LOGI("pic scale height");
            float r = height / (1.0 * width / m_yuv_width_ * m_yuv_height_);
            LOGI("pic scale height r: %f", r);
            OrthoM(-1, 1, -r, r, matrix);
        }
    }
    LOGI("FilterYUV _SetMatrix end");
}


