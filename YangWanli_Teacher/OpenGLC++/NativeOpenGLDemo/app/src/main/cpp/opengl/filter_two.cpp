#include "filter_two.h"

FilterTwo::FilterTwo() {
    InitMatrix(m_matrix);
}

FilterTwo::~FilterTwo() {

}

void FilterTwo::OnCreate() {
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
                lowp vec4 textureColor = texture2D(sTexture, ft_Position);
                float gray = textureColor.r * 0.2125 + textureColor.g * 0.7154 + textureColor.b * 0.0721;//将RGB图像转为灰度图
                gl_FragColor = vec4(gray, gray, gray, textureColor.w);
            });

    m_program = CreateProgram(m_vertex_str, m_fragment_str, &m_vshader, &m_fshader);
    m_v_position = glGetAttribLocation(m_program, "v_Position");
    m_f_position = glGetAttribLocation(m_program, "f_Position");
    m_sampler_ = glGetUniformLocation(m_program, "sTexture");
    m_u_matrix = glGetUniformLocation(m_program, "u_Matrix");

    glGenTextures(1, &m_texture_id_);
    glBindTexture(GL_TEXTURE_2D, m_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FilterTwo OnCreate end");
}

void FilterTwo::OnChange(int surface_width, int surface_height) {
    LOGI("FilterTwo OnChange in");
    m_surface_width = surface_width;
    m_surface_height = surface_height;
    glViewport(0, 0, surface_width, surface_height);
    LOGI("FilterTwo OnChange end");
}

void FilterTwo::OnDraw() {
    LOGI("FilterTwo OnDraw in");
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);//指定刷屏颜色  1:不透明  0：透明
    glClear(GL_COLOR_BUFFER_BIT);//将刷屏颜色进行刷屏，但此时仍然处于后台缓冲中，需要swapBuffers交换到前台界面显示

    glUseProgram(m_program);

    glUniformMatrix4fv(m_u_matrix, 1, GL_FALSE, m_matrix);
    glEnableVertexAttribArray(m_v_position);
    glVertexAttribPointer(m_v_position, 2, GL_FLOAT, false, 8, m_vertex_array);
    glEnableVertexAttribArray(m_f_position);
    glVertexAttribPointer(m_f_position, 2, GL_FLOAT, false, 8, m_fragment_array);

    glBindTexture(GL_TEXTURE_2D, m_texture_id_);
    glActiveTexture(GL_TEXTURE0);
    if (m_pixels_ != NULL) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image_width, m_image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels_);
    }
    glUniform1i(m_sampler_, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FilterTwo OnDraw end");
}

void FilterTwo::Destroy() {
    m_pixels_ = NULL;
    glDeleteTextures(1, &m_texture_id_);
    glDetachShader(m_program, m_vshader);
    glDetachShader(m_program, m_fshader);
    glDeleteShader(m_vshader);
    glDeleteShader(m_fshader);
    glDeleteProgram(m_program);
}

void FilterTwo::SetImagePixel(int image_width, int image_height, void *pixels) {
    LOGI("FilterTwo SetImagePixel in");
    m_image_width = image_width;
    m_image_height = image_height;
    m_pixels_ = pixels;
    if ((m_surface_width > 0) && (m_surface_height > 0)) {
        SetMatrix();
    }
    LOGI("FilterTwo SetImagePixel end");
}