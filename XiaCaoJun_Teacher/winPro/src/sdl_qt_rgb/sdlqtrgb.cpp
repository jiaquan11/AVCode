#include "sdlqtrgb.h"
#include <sdl/SDL.h>

#pragma comment(lib, "SDL2.lib")

static SDL_Window* sdl_win = NULL;
static SDL_Renderer* sdl_render = NULL;
static SDL_Texture* sdl_texture = NULL;
static int sdl_width = 0;
static int sdl_height = 0;
static unsigned char* rgb = NULL;
static int pix_size = 4;

SDLQtRGB::SDLQtRGB(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    sdl_width = ui.label->width();
    sdl_height = ui.label->height();
    SDL_Init(SDL_INIT_VIDEO);
    sdl_win = SDL_CreateWindowFrom((void*)ui.label->winId());
    sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    sdl_texture = SDL_CreateTexture(sdl_render,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        sdl_width,
        sdl_height);

    rgb = new unsigned char[sdl_width * sdl_height * pix_size];

    startTimer(10);
}

SDLQtRGB::~SDLQtRGB()
{}

void SDLQtRGB::timerEvent(QTimerEvent* ev) {
	static unsigned char tmp = 255;
	tmp--;
	for (int j = 0; j < sdl_height; j++) {
		int b = j * sdl_width * pix_size;
		for (int i = 0; i < sdl_width * pix_size; i += pix_size) {
			rgb[b + i] = 0;//B
			rgb[b + i + 1] = 0;//G
			rgb[b + i + 2] = tmp;//R
			rgb[b + i + 3] = 0;//A
		}
	}

	//5 内存数据写入材质
	SDL_UpdateTexture(sdl_texture, NULL, rgb, sdl_width * pix_size);

	//6 清理屏幕
	SDL_RenderClear(sdl_render);//先将屏幕清理一遍

	//7 复制材质到渲染器
	SDL_Rect sdl_rect;
	sdl_rect.x = 0;
	sdl_rect.y = 0;
	sdl_rect.w = sdl_width;
	sdl_rect.h = sdl_height;
	SDL_RenderCopy(sdl_render, sdl_texture,
		NULL,//原图位置和尺寸
		&sdl_rect//目标位置和尺寸
	);

	//8 渲染
	SDL_RenderPresent(sdl_render);
}
