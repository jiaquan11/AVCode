#include "filter_one.h"

FilterOne::FilterOne() {
    InitMatrix(matrix);//初始化为单位矩阵
}

FilterOne::~FilterOne() {

}

void FilterOne::OnCreate() {
    vertexStr = GET_STR(
            attribute vec4 v_Position;
            attribute vec2 f_Position;
            varying vec2 ft_Position;
            uniform mat4 u_Matrix;//4*4的矩阵
            void main() {
                ft_Position = f_Position;
                gl_Position = v_Position * u_Matrix;//将矩阵与顶点坐标进行相乘，用于图像旋转
            });

    fragmentStr = GET_STR(
            precision mediump float;
            varying vec2 ft_Position;
            uniform sampler2D sTexture;
            void main() {//texture2D表示GPU将输入得图像纹理像素进行读取，读取到GPU的管线中,最后渲染出来
                gl_FragColor = texture2D(sTexture, ft_Position);
            });

    program = CreateProgram(vertexStr, fragmentStr, &vShader, &fShader);

    //获取着色器程序中的这个变量a_position，返回一个变量id，用于给这个变量赋值
    vPosition = glGetAttribLocation(program, "v_Position");
    fPosition = glGetAttribLocation(program, "f_Position");
    sampler = glGetUniformLocation(program, "sTexture");
    u_matrix = glGetUniformLocation(program, "u_Matrix");

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    //设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FilterOne OnCreate end");
}

//指定屏幕显示界面的宽高
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

    glUseProgram(program);//使用着色器程序

    //渲染时纹理赋值操作
    glBindTexture(GL_TEXTURE_2D, textureID);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(sampler, 0);//GL_TEXTURE0表示就是第一层纹理

    glUniformMatrix4fv(u_matrix, 1, GL_FALSE, matrix);//给矩阵变量赋值

    if (pixels != NULL) {//为后台缓存显存中设置图片数据
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image_width_, m_image_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

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
    /*opengl绘制
     * 绘制三角形，第二个参数表示从索引0开始，绘制三个顶点
     */
//    glDrawArrays(GL_TRIANGLES, 0, 3);//绘制三角形
//    glDrawArrays(GL_TRIANGLES, 0, 6);//绘制四边形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);//绘制四边形
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FilterOne OnDraw end");
}

void FilterOne::DestroySource() {
    if (pixels != NULL) {
        pixels = NULL;
    }
}

/**
 * 释放opengl资源
 */
void FilterOne::Destroy() {
    glDeleteTextures(1, &textureID);
    glDetachShader(program, vShader);
    glDetachShader(program, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    glDeleteProgram(program);
}

void FilterOne::SetImagePixel(int image_width, int image_height, void *data) {
    LOGI("FilterOne SetImagePixel in");
    m_image_width_ = image_width;
    m_image_height_ = image_height;
    pixels = data;
    if ((m_surface_width > 0) && (m_surface_height > 0)) {
        _SetMatrix(m_surface_width, m_surface_height);
    }
    LOGI("FilterOne SetImagePixel end");
}

//正交投影操作
void FilterOne::_SetMatrix(int surface_width, int surface_height) {
    LOGI("FilterOne _SetMatrix in");
//    initMatrix(matrix);
    //这里是矩阵投影操作   将图片投影到屏幕上
    //屏幕1080*2000=0.54 图片:517*685=0.75
    float screen_r = 1.0 * surface_width / surface_height;
    float picture_r = 1.0 * m_image_width_ / m_image_height_;
    if (screen_r > picture_r) {//图片宽度缩放，图片高铺满屏幕
        LOGI("pic scale width");
        float r = surface_width / (1.0 * surface_height / m_image_height_ * m_image_width_);
        LOGI("pic scale width r: %f", r);
        OrthoM(-r, r, -1, 1, matrix);
    } else {//图片宽的比率大于屏幕，则宽进行直接覆盖屏幕，而图片高度缩放
        LOGI("pic scale height");
        /*
         * 这里解释一下:这里是图片宽高比例大于屏幕宽高比例，则图片的宽进行平铺屏幕，图片高需要进行缩放。
         * 为了正常显示图片，需要进行等比例缩放图片，所以根据图片的宽高等比例  图片宽/图片高=屏幕宽/图片需要占用屏幕的新高度，
         * 通过这里计算就得到：图片需要占用屏幕的新高度，也即下面的(1.0 * width / w * h)，同时为了显示缩放，需要用
         * 原屏幕实际高/图片需要占用屏幕的新高度，这里得到的比例值是大于1的，通过这个比例值映射到屏幕(-1,1)范围内，则会出现一个
         * 实际的缩放的效果
         * */
        float r = surface_height / (1.0 * surface_width / m_image_width_ * m_image_height_);
        LOGI("pic scale height r: %f", r);
        OrthoM(-1, 1, -r, r, matrix);
    }
    LOGI("FilterOne _SetMatrix end");
}
