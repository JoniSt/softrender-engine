//============================================================================
// Name        : softrender-engine.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <random>

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

#include "SpriteRenderer.hpp"
#include "IntRectangle.hpp"

using namespace std;
using namespace mmo2020;
using namespace ttlhacker;

void printSDLErr(string msg) {
	cerr << msg << ":" << endl << SDL_GetError() << endl;
}


constexpr int windowWidth = 1600;
constexpr int windowHeight = 900;

constexpr int fpsReportIntervalMsec = 5000;

constexpr int numTestSprites = 2000;
constexpr int maxTestSpriteSize = 700;

/**
 * Pixel format to use for sending frames to SDL.
 */
constexpr auto pixelFormat = SDL_PIXELFORMAT_ARGB8888;

/**
 * @param r
 * @param g
 * @param b
 * @return The given color as a packed ARGB8888 value.
 */
uint32_t rgbToARGB8888(uint8_t r, uint8_t g, uint8_t b) {
	return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

/**
 * Handles an incoming SDL event.
 * Returns true if the program should exit.
 */
bool handleEvent(const SDL_Event &event) {
	switch (event.type) {
		default:
			break;
		case SDL_QUIT:
			return true;
	}

	return false;
}

/**
 * Handles incoming SDL events.
 * Returns true if the program should exit.
 */
bool handleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (handleEvent(event)) {
			return true;
		}
	}

	return false;
}



vector<Sprite> makeTestSprites() {
	mt19937 gen;
	uniform_int_distribution<> sizeDistrib(1, maxTestSpriteSize);
	uniform_int_distribution<> xDistrib(0, windowWidth);
	uniform_int_distribution<> yDistrib(0, windowHeight);

	vector<Sprite> sprites;
	for (uint32_t i = 0; i < numTestSprites; i++) {
		int spriteWidth = sizeDistrib(gen);
		int spriteHeight = sizeDistrib(gen);
		int x = xDistrib(gen);
		int y = yDistrib(gen);

		Sprite::PixelGetter pixelGetter;
		if (i % 2) {
			pixelGetter = [=](int x, int y) {return SpritePixel(x * 256 / spriteWidth, y * 256 / spriteHeight, 0);};
		} else {
			pixelGetter = [=](int x, int y) {return SpritePixel(x * 256 / spriteWidth, 0, y * 256 / spriteHeight);};
		}
		//pixelGetter = [=](int x, int y) {return SpritePixel(64, 0, 128);};

		IntRectangle<int32_t> position(x, y, spriteWidth, spriteHeight);
		sprites.emplace_back(position, pixelGetter, i);
	}

	return sprites;
}


int main(int argc, char *argv[]) {
	bool sdlInitialized = false;
	SDL_Window *sdlWindow = nullptr;
	SDL_Renderer *sdlRenderer = nullptr;
	SDL_Texture *sdlTexture = nullptr;
	uint32_t lastTime = 0;
	uint32_t frameCount = 0;
	unique_ptr<SpriteRenderer<>> mmoRenderer;

	vector<Sprite> sprites = makeTestSprites();
	cout << "Got " << sprites.size() << " sprites" << endl;



	if (SDL_Init(SDL_INIT_VIDEO)) {
		printSDLErr("Unable to initialize SDL");
		goto cleanup;
	}
	sdlInitialized = true;

	sdlWindow = SDL_CreateWindow(
			"Really awful game engine lmao",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			windowWidth,
			windowHeight,
			0
	);

	if (!sdlWindow) {
		printSDLErr("Unable to create window");
		goto cleanup;
	}

	sdlRenderer = SDL_CreateRenderer(
			sdlWindow,
			-1,
			0
	);

	if (!sdlRenderer) {
		printSDLErr("Unable to create renderer");
		goto cleanup;
	}

	sdlTexture = SDL_CreateTexture(
			sdlRenderer,
			pixelFormat,
			SDL_TEXTUREACCESS_STREAMING,
			windowWidth,
			windowHeight);

	if (!sdlTexture) {
		printSDLErr("Unable to create texture");
		goto cleanup;
	}


	mmoRenderer = make_unique<SpriteRenderer<>>(
			windowWidth,
			windowHeight,
			rgbToARGB8888);

	lastTime = SDL_GetTicks();
	frameCount = 0;

	while (true) {
		if (handleEvents()) {
			break;
		}


		void *pixels;
		int pitch;
		if (SDL_LockTexture(sdlTexture, nullptr, &pixels, &pitch)) {
			printSDLErr("SDL_LockTexture failed");
			break;
		}

		//TODO: Actually render stuff?
		for (Sprite& sprite: sprites) {
			auto& pos = sprite.position;
			pos.x++;
			pos.y++;
			if (pos.x > windowWidth) pos.x = 0;
			if (pos.y > windowHeight) pos.y = 0;
		}
		mmoRenderer->render(sprites, (uint8_t *)pixels, pitch);

		SDL_UnlockTexture(sdlTexture);

		//Blit the texture to the screen
		if (SDL_RenderClear(sdlRenderer)) {
			printSDLErr("SDL_RenderClear failed");
			break;
		}

		if (SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, nullptr)) {
			printSDLErr("SDL_RenderCopy failed");
			break;
		}

		SDL_RenderPresent(sdlRenderer);

		//Measure FPS
		frameCount++;
		const uint32_t currentTime = SDL_GetTicks();
		const uint32_t timeElapsed = currentTime - lastTime;
		if (timeElapsed >= fpsReportIntervalMsec) {
			double fps = (double)frameCount / timeElapsed * 1000;
			cout << "FPS: " << fps << endl;
			lastTime = currentTime;
			frameCount = 0;
		}
	}


cleanup:
	if (sdlTexture) SDL_DestroyTexture(sdlTexture);
	if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
	if (sdlWindow) SDL_DestroyWindow(sdlWindow);
	if (sdlInitialized) SDL_Quit();
	return 0;
}
