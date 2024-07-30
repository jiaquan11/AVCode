#include "filter_yuv.h"

FilterYUV::FilterYUV() {

}

FilterYUV::~FilterYUV() {

}

void FilterYUV::OnCreate() {
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

    m_program = CreateProgram(m_vertex_str, m_fragment_str, &m_vshader, &m_fshader);
    LOGI("FilterYUV CreateProgram: %d", m_program);
    m_v_position = glGetAttribLocation(m_program, "v_Position");
    m_f_position = glGetAttribLocation(m_program, "f_Position");
    m_u_matrix = glGetUniformLocation(m_program, "u_Matrix");
    m_sampler_y_ = glGetUniformLocation(m_program, "sampler_y");
    m_sampler_u_ = glGetUniformLocation(m_program, "sampler_u");
    m_sampler_v_ = glGetUniformLocation(m_program, "sampler_v");

    glGenTextures(3, m_samplers_);
    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_samplers_[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
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
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program);

    glUniformMatrix4fv(m_u_matrix, 1, GL_FALSE, m_matrix);

    glEnableVertexAttribArray(m_v_position);
    glVertexAttribPointer(m_v_position, 2, GL_FLOAT, false, 8, m_vertex_array);
    glEnableVertexAttribArray(m_f_position);
    glVertexAttribPointer(m_f_position, 2, GL_FLOAT, false, 8, m_fragment_array);
    if ((m_image_width > 0) && (m_image_height > 0)) {
        if (m_y_data_ != NULL) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_samplers_[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_image_width, m_image_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_y_data_);
            glUniform1i(m_sampler_y_, 0);
        }
        if (m_u_data_ != NULL) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_samplers_[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_image_width / 2, m_image_height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_u_data_);
            glUniform1i(m_sampler_u_, 1);
        }
        if (m_v_data_ != NULL) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_samplers_[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_image_width / 2, m_image_height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_v_data_);
            glUniform1i(m_sampler_v_, 2);
        }
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void FilterYUV::Destroy() {
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
    glDeleteTextures(3, m_samplers_);
    glDetachShader(m_program, m_vshader);
    glDetachShader(m_program, m_fshader);
    glDeleteShader(m_vshader);
    glDeleteShader(m_fshader);
    glDeleteProgram(m_program);
}

void FilterYUV::SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data) {
    if ((yuv_width > 0) && (yuv_height > 0)) {
        if ((m_image_width != yuv_width) || (m_image_height != yuv_height)) {
            m_image_width = yuv_width;
            m_image_height = yuv_height;
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
            m_y_data_ = malloc(m_image_width * m_image_height);
            m_u_data_ = malloc(m_image_width * m_image_height / 4);
            m_v_data_ = malloc(m_image_width * m_image_height / 4);
            SetMatrix();
        }
        memcpy(m_y_data_, y_data, m_image_width * m_image_height);
        memcpy(m_u_data_, u_data, m_image_width * m_image_height / 4);
        memcpy(m_v_data_, v_data, m_image_width * m_image_height / 4);
    }
}


