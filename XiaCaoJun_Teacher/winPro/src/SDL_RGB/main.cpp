#include <iostream>
#include <sdl/SDL.h>
using namespace std;

#pragma comment(lib, "SDL2.lib")
#undef main
int main(int argc, char* argv[]) {
	int w = 800;
	int h = 600;
	//1.初始化SDL video库
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << SDL_GetError() << endl;
		return -1;
	}

	//2.生成SDL，窗口
	auto screen = SDL_CreateWindow("test sdl ffmpeg", 
		SDL_WINDOWPOS_CENTERED,//窗口位置
		SDL_WINDOWPOS_CENTERED,
		w, h,
		SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (!screen) {
		cout << SDL_GetError() << endl;
		return -2;
	}

	//3.生成渲染器
	auto render = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
	if (!render) {
		cout << SDL_GetError() << endl;
		return -3;
	}

	//4.生成材质
	auto texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,//可加锁
		w, h);
	if (!texture) {
		cout << SDL_GetError() << endl;
		return -4;
	}

	//存放图像的数据
	shared_ptr<unsigned char> rgb(new unsigned char[w * h * 4]);
	auto r = rgb.get();
	unsigned char tmp = 255;
	for (;;) {
		//判断退出
		SDL_Event ev;
		SDL_WaitEventTimeout(&ev, 10);
		if (ev.type == SDL_QUIT) {
			SDL_DestroyWindow(screen);
			break;
		}

		tmp--;

		for (int j = 0; j < h; j++) {
			int b = j * w * 4;
			for (int i = 0; i < w * 4; i += 4) {
				r[b + i] = 0;//B
				r[b + i + 1] = 0;//G
				r[b + i + 2] = tmp;//R
				r[b + i + 3] = 0;//A
			}
		}

		//5 内存数据写入材质
		SDL_UpdateTexture(texture, NULL, r, w * 4);

		//6 清理屏幕
		SDL_RenderClear(render);//先将屏幕清理一遍

		//7 复制材质到渲染器
		SDL_Rect sdl_rect;
		sdl_rect.x = 0;
		sdl_rect.y = 0;
		sdl_rect.w = w;
		sdl_rect.h = h;
		SDL_RenderCopy(render, texture,
			NULL,//原图位置和尺寸
			&sdl_rect//目标位置和尺寸
		);

		//8 渲染
		SDL_RenderPresent(render);
	}

	getchar();
	return 0;
}