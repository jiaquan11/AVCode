#include "base_opengl.h"

BaseOpengl::BaseOpengl() {//顶点坐标，原点在中间，范围-1-1
    vertexs = new float[8];
    fragments = new float[8];

    float v[] = {
            1, -1,
            1, 1,
            -1, -1,
            -1, 1
    };
    memcpy(vertexs, v, sizeof(v));

    float f[] = {//纹理坐标：原点在左上角：0-1
            1, 1,
            1, 0,
            0, 1,
            0, 0
    };
    memcpy(fragments, f, sizeof(f));
}

BaseOpengl::~BaseOpengl() {
    delete[]vertexs;
    delete[]fragments;
}

void BaseOpengl::OnCreate() {

}

void BaseOpengl::OnChange(int width, int height) {

}

void BaseOpengl::OnDraw() {

}

void BaseOpengl::SetImagePixel(int image_width, int image_height, void *data) {

}

void BaseOpengl::SetYuvData(int width, int height, void *y, void *u, void *v) {

}

void BaseOpengl::DestroySource() {

}

void BaseOpengl::Destroy() {

}





