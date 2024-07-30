#include "filter_one.h"

FilterOne::FilterOne() {

}

FilterOne::~FilterOne() {

}

void FilterOne::OnCreate() {
    m_vertex_str = GET_STR(
            attribute vec4 v_Position;
            attribute vec2 f_Position;
            varying vec2 ft_Position;
            uniform mat4 u_Matrix;
            void main() {
                ft_Position = f_Position;
                gl_Position = v_Position * u_Matrix;
            });

    m_fragment_str = GET_STR(
            precision mediump float;
            varying vec2 ft_Position;
            uniform sampler2D sTexture;
            void main() {//texture2D表示GPU将输入得图像纹理像素进行读取，读取到GPU的管线中,最后渲染出来
                gl_FragColor = texture2D(sTexture, ft_Position);
            });

    m_program = CreateProgram(m_vertex_str, m_fragment_str, &m_vshader, &m_fshader);
    m_v_position = glGetAttribLocation(m_program, "v_Position");
    m_f_position = glGetAttribLocation(m_program, "f_Position");
    m_sampler_ = glGetUniformLocation(m_program, "sTexture");
    m_u_matrix = glGetUniformLocation(m_program, "u_Matrix");

    //1.生成并设置纹理对象(纹理id是纹理对象的标识)
    glGenTextures(1, &m_texture_id_);
    glBindTexture(GL_TEXTURE_2D, m_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FilterOne OnCreate end");
}

void FilterOne::OnChange(int surface_width, int surface_height) {
    LOGI("FilterOne OnChange in surface_width:%d surface_height:%d", surface_width, surface_height);
    m_surface_width = surface_width;
    m_surface_height = surface_height;
    glViewport(0, 0, surface_width, surface_height);
    LOGI("FilterOne OnChange end");
}

void FilterOne::OnDraw() {
    LOGI("FilterOne OnDraw in");
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);//指定刷屏颜色  1:不透明  0：透明
    glClear(GL_COLOR_BUFFER_BIT);//将刷屏颜色进行刷屏，但此时仍然处于后台缓冲中，需要swapBuffers交换到前台界面显示

    glUseProgram(m_program);

    glUniformMatrix4fv(m_u_matrix, 1, GL_FALSE, m_matrix);
    glEnableVertexAttribArray(m_v_position);
    /**
     * 给着色器的顶点顶点变量赋值
     * 第一个参数是着色器的变量id,第二个参数是每个顶点两个值，第三个参数是值的类型，第四个参数表示是否
     * 归一化，已经有顶点参数，无需自动归一化。第五个参数表示每个顶点的跨度(这里每个顶点跨8个字节)，
     * 第六个参数表示顶点数组
     */
    glVertexAttribPointer(m_v_position, 2, GL_FLOAT, false, 8, m_vertex_array);
    glEnableVertexAttribArray(m_f_position);
    glVertexAttribPointer(m_f_position, 2, GL_FLOAT, false, 8, m_fragment_array);

    //2、将纹理对象绑定到激活的纹理单元
    glBindTexture(GL_TEXTURE_2D, m_texture_id_);
    glActiveTexture(GL_TEXTURE0);
    if (m_pixels_ != NULL) {
        /**
         * 必须是先激活纹理单元，然后将CPU中的图片数据拷贝到GPU中，这样才能显示图片
         */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image_width, m_image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels_);
    }
    //3、将纹理单元绑定到着色器采样器
    glUniform1i(m_sampler_, 0);

    /**
     * opengl绘制
     * 绘制三角形，第二个参数表示从索引0开始，绘制三个顶点
     */
//  glDrawArrays(GL_TRIANGLES, 0, 3);//绘制三角形
//  glDrawArrays(GL_TRIANGLES, 0, 6);//绘制四边形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);//绘制四边形
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FilterOne OnDraw end");
}

/**
 * 释放opengl资源
 */
void FilterOne::Destroy() {
    m_pixels_ = NULL;
    glDeleteTextures(1, &m_texture_id_);
    glDetachShader(m_program, m_vshader);
    glDetachShader(m_program, m_fshader);
    glDeleteShader(m_vshader);
    glDeleteShader(m_fshader);
    glDeleteProgram(m_program);
}

void FilterOne::SetImagePixel(int image_width, int image_height, void *pixels) {
    LOGI("FilterOne SetImagePixel in");
    m_image_width = image_width;
    m_image_height = image_height;
    m_pixels_ = pixels;
    if ((m_surface_width > 0) && (m_surface_height > 0)) {
        SetMatrix();
    }
    LOGI("FilterOne SetImagePixel end");
}