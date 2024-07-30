#include "base_opengl.h"

BaseOpengl::BaseOpengl() {//顶点坐标，原点在中间，范围-1-1
    m_vertex_array = new float[8];
    m_fragment_array = new float[8];

    float v[] = {
        1, -1,
        1, 1,
        -1, -1,
        -1, 1
    };
    memcpy(m_vertex_array, v, sizeof(v));

    float f[] = {//纹理坐标：原点在左上角：0-1
        1, 1,
        1, 0,
        0, 1,
        0, 0
    };
    memcpy(m_fragment_array, f, sizeof(f));
}

BaseOpengl::~BaseOpengl() {
    delete[] m_vertex_array;
    delete[] m_fragment_array;
}

void BaseOpengl::OnCreate() {

}

void BaseOpengl::OnChange(int surface_width, int surface_height) {

}

void BaseOpengl::OnDraw() {

}

void BaseOpengl::Destroy() {

}

void BaseOpengl::SetImagePixel(int image_width, int image_height, void *data) {

}

void BaseOpengl::SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data) {

}

/**
 * 设置矩阵(正交投影操作)
 */
void BaseOpengl::SetMatrix() {
    LOGI("BaseOpengl _SetMatrix in");
//  这里是矩阵投影操作   将图片投影到屏幕上
//  屏幕1080*2000=0.54 图片:517*685=0.75
    float screen_r = 1.0 * m_surface_width / m_surface_height;
    float picture_r = 1.0 * m_image_width / m_image_height;
    if (screen_r > picture_r) {//图片宽度缩放，图片高铺满屏幕
        LOGI("pic scale width");
        float r = m_surface_width / (1.0 * m_surface_height / m_image_height * m_image_width);
        LOGI("pic scale width r: %f", r);
        OrthoM(-r, r, -1, 1, m_matrix);
    } else {//图片宽的比率大于屏幕，则宽进行直接覆盖屏幕，而图片高度缩放
        LOGI("pic scale height");
        /**
         * 这里解释一下:这里是图片宽高比例大于屏幕宽高比例，则图片的宽进行平铺屏幕，图片高需要进行缩放。
         * 为了正常显示图片，需要进行等比例缩放图片，所以根据图片的宽高等比例  图片宽/图片高=屏幕宽/图片需要占用屏幕的新高度，
         * 通过这里计算就得到：图片需要占用屏幕的新高度，也即下面的(1.0 * width / w * h)，同时为了显示缩放，需要用
         * 原屏幕实际高/图片需要占用屏幕的新高度，这里得到的比例值是大于1的，通过这个比例值映射到屏幕(-1,1)范围内，则会出现一个
         * 实际的缩放的效果.
         */
        float r = m_surface_height / (1.0 * m_surface_width / m_image_width * m_image_height);
        LOGI("pic scale height r: %f", r);
        OrthoM(-1, 1, -r, r, m_matrix);
    }
    LOGI("BaseOpengl _SetMatrix end");
}







