/*
 * Renderer.hpp
 *
 *  Created on: 17.06.2020
 *      Author: jonathan
 */

#ifndef SPRITERENDERER_HPP_
#define SPRITERENDERER_HPP_

#include <cstdint>
#include <vector>
#include <memory>
#include <variant>
#include <functional>
#include <algorithm>

#include "InlineStorageVector.hpp"
#include "IntRectangle.hpp"

using namespace std;
using namespace ttlhacker;

namespace mmo2020 {


/**
 * One pixel of a sprite, which may either be fully opaque or
 * fully transparent.
 */
struct SpritePixel {
	uint8_t r, g, b;
	bool isTransparent;

	/**
	 * Constructs a non-transparent SpritePixel with the given color.
	 *
	 * @param r
	 * @param g
	 * @param b
	 */
	SpritePixel(uint8_t r, uint8_t g, uint8_t b):
		r(r), g(g), b(b), isTransparent(false)
	{
		//Nothing else to do
	}

	/**
	 * Constructs a transparent SpritePixel.
	 */
	SpritePixel():
		r(0), g(0), b(0), isTransparent(true)
	{
		//Nothing else to do
	}
};

/**
 * A sprite to draw to the screen.
 */
struct Sprite {
	/**
	 * Given relative coordinates into the sprite, returns
	 * the pixel at those coordinates.
	 */
	using PixelGetter = function<SpritePixel(int x, int y)>;

	IntRectangle<int32_t> position;
	PixelGetter pixelGetter;
	uint32_t layer;

	Sprite(IntRectangle<int32_t> position, PixelGetter pixelGetter, uint32_t layer):
		position(position), pixelGetter(pixelGetter), layer(layer)
	{
		//Nothing else to initialize
	}
};


/**
 * Packs R, G and B values into an opaque pixel represented as an uint32_t.
 */
using PixelPacker = uint32_t(uint8_t r, uint8_t g, uint8_t b);

template<size_t numInlineSpritesPerPixel = 4>
class SpriteRenderer {
private:

	/**
	 * One pixel on a RasterLine.
	 */
	struct RasterLinePixel {
		/**
		 * All sprites that begin on this exact pixel.
		 * Must be a pointer because reference_wrapper can't be default-constructed.
		 */
		InlineStorageVector<const Sprite *, numInlineSpritesPerPixel> beginningSprites;

		inline void clear() {
			beginningSprites.clear();
		}
	};

	/**
	 * A horizontal line of pixels across the screen.
	 */
	class RasterLine {
	private:
		//using SpriteStack = vector<reference_wrapper<const SpriteOnRasterLine>>;
		using SpriteStack = vector<const Sprite *>;

		/**
		 * The pixels on this RasterLine. Each one contains a (custom) vector
		 * of sprites that begin on that exact pixel.
		 * This vector always has width elements and is never cleared.
		 */
		vector<RasterLinePixel> pixels;

		/**
		 * Inserts all sprites that get activated (have their first pixel) on the given X coordinate
		 * into the given active sprite stack.
		 *
		 * @param spriteStack
		 * @param x
		 * @return True if any new sprites have been inserted, false if not.
		 */
		void insertAllActivatedSprites(SpriteStack& spriteStack, int x) {
			RasterLinePixel& rlPx = pixels[x];

			size_t nSpritesToInsert = rlPx.beginningSprites.size();
			if (nSpritesToInsert == 0) return;

			//Sort the sprites to insert in ascending order.
			auto order = [](const Sprite *a, const Sprite *b) {
				return a->layer < b->layer;
			};
			sort(rlPx.beginningSprites.begin(), rlPx.beginningSprites.end(), order);

			//Turns out inplace_merge is slower than this homegrown monstrosity below.
			//auto middle = spriteStack.insert(spriteStack.end(), rlPx.beginningSprites.begin(), rlPx.beginningSprites.end());
			//inplace_merge(spriteStack.begin(), middle, spriteStack.end(), order);


			//If the stack is empty, just insert the sorted elements directly and be done.
			//The loop below can't merge into an empty vector.
			size_t previousSize = spriteStack.size();
			if (previousSize == 0) {
				spriteStack.insert(spriteStack.begin(), rlPx.beginningSprites.begin(), rlPx.beginningSprites.end());
				return;
			}

			//Append as many nullptrs to the vector as elements that we need to insert.
			spriteStack.resize(previousSize + nSpritesToInsert, nullptr);

			//Finally merge the new sprites into the spriteStack.
			auto previousContentIter = spriteStack.begin() + (previousSize - 1);
			auto writeIter = spriteStack.end();
			auto spritesToInsertIter = rlPx.beginningSprites.end() - 1;

			bool startOfSpriteStackReached = false;
			while (true) {
				writeIter--;
				const Sprite *spriteToInsert = *spritesToInsertIter;

				//We can insert directly at the write position if there's either no elements of the previous stack content left,
				//or if we sort before the largest element of the previous stack content that's still left.
				bool insertHere = startOfSpriteStackReached || (spriteToInsert->layer > (*previousContentIter)->layer);

				if (insertHere) {
					//Consume one of the new sprites to merge in.
					*writeIter = spriteToInsert;
					if (spritesToInsertIter == rlPx.beginningSprites.begin()) {
						//We just inserted the last sprite and are now done.
						break;
					}
					spritesToInsertIter--;
				} else {
					//Move one of the old sprites forward within the vector.
					*writeIter = *previousContentIter;
					*previousContentIter = nullptr;
					if (previousContentIter == spriteStack.begin()) {
						//No more old sprites left to move over; from now on, just insert everything.
						startOfSpriteStackReached = true;
					} else {
						previousContentIter--;
					}
				}
			}

		}

		/**
		 * Removes all inactive sprites from the given sprite stack. That is,
		 * it removes all sprites that the given X coordinate is past of already.
		 *
		 * @param spriteStack
		 * @param x
		 */
		void removeInactiveSpritesFromSpriteStack(SpriteStack& spriteStack, int x) {
			spriteStack.erase(
					remove_if(
							spriteStack.begin(),
							spriteStack.end(),
							[=](const Sprite *sprite) {return x > sprite->position.getLastX();}),
					spriteStack.end());
		}

		/**
		 * Renders one pixel of this RasterLine.
		 * May remove inactive sprites from the spriteStack.
		 *
		 * @param spriteStack The current stack of active sprites.
		 * @param x           The X coordinate of the pixel to render.
		 * @param y           The Y coordinate of the pixel to render.
		 * @return            The pixel. May be transparent if there is no opaque sprite at the given coordinates.
		 */
		SpritePixel renderPixel(SpriteStack& spriteStack, int x, int y) {
			renderPixelRetry:

			for (auto sprIt = spriteStack.rbegin(); sprIt != spriteStack.rend(); sprIt++) {
				const Sprite *spr = *sprIt;
				if (spr->position.getLastX() < x) {
					//Oops, we found an inactive sprite. Remove it and then retry the renderPixel operation.
					if (sprIt == spriteStack.rbegin()) {
						//Avoid iterating through the entire sprite stack if the topmost
						//sprite is inactive.
						spriteStack.pop_back();
					} else {
						//Something in the middle of the sprite stack is inactive,
						//remove everything we can since we have to iterate over it anyway.
						removeInactiveSpritesFromSpriteStack(spriteStack, x);
					}
					//We need to re-run the entire renderPixel function because our iterators
					//just got invalidated, breaking this for loop if we would continue.
					//Please don't hate me for the goto.
					goto renderPixelRetry;
				}

				//If the sprite has a pixel for us, return that.
				const IntRectangle<int32_t>& spritePos = spr->position;
				SpritePixel pix = spr->pixelGetter(x - spritePos.x, y - spritePos.y);
				if (!pix.isTransparent) {
					return pix;
				}
			}

			return SpritePixel();
		}

		int width;


	public:
		RasterLine(int width):
			width(width)
		{
			pixels.resize(width);
		}

		/**
		 * Adds a sprite to be rendered on this line.
		 * The sprite MUST actually have (potentially transparent) pixels on this line.
		 *
		 * @param sprite
		 */
		void addSprite(const Sprite *sprite, int32_t firstX) {
			pixels[firstX].beginningSprites.put(sprite);
		}

		/**
		 * Removes all Sprites from this RasterLine.
		 */
		void clear() {
			for (RasterLinePixel& pixel: pixels) {
				pixel.clear();
			}
		}

		/**
		 * Renders this RasterLine to the given target line of pixels.
		 *
		 * @param targetPixels The target framebuffer line.
		 * @param y            The Y coordinate of this RasterLine.
		 * @param pixelPacker
		 */
		void render(uint32_t *targetPixels, int y, PixelPacker *pixelPacker) {

			//The stack of currently active sprites, sorted so that the topmost sprite
			//(the one with the largest Z coordinate) is last.
			SpriteStack activeSpriteStack;

			for (int x = 0; x < width; x++) {
				insertAllActivatedSprites(activeSpriteStack, x);
				SpritePixel pix = renderPixel(activeSpriteStack, x, y);
				if (pix.isTransparent) {
					pix = SpritePixel(0, 0, 0);
				}
				targetPixels[x] = pixelPacker(pix.r, pix.g, pix.b);
			}
		}
	};

	int width, height;

	/**
	 * All raster lines (horizontal lines of pixels) of the frame to render.
	 * These all have the same width.
	 */
	vector<RasterLine> rasterLines;

	/**
	 * The function to use for packing RGB values into uint32_t's when putting
	 * pixels into the framebuffer.
	 */
	PixelPacker *pixelPacker;


	/**
	 * Takes the given Sprites and associates them with the RasterLines they
	 * might be visible in.
	 *
	 * @param sprites
	 */
	void distributeSpritesToRasterLines(const vector<Sprite>& sprites) {
		IntRectangle<int32_t> viewport(0, 0, width, height);

		constexpr int blockSize = 8;

		//First sort the incoming sprites into horizontal stripes of blockSize lines.
		//Each of those lines is a "block".
		const int numBlocks = (height + blockSize - 1) / blockSize;

		struct LineBlock {
			vector<reference_wrapper<const Sprite>> sprites;
		};

		vector<LineBlock> blocks;
		blocks.resize(numBlocks);

		for (const Sprite& sprite: sprites) {
			auto visibleRect = viewport.getIntersection(sprite.position);
			if (visibleRect.isEmpty()) {
				continue;
			}

			int32_t firstBlock = visibleRect.y / blockSize;
			int32_t lastBlock = visibleRect.getLastY() / blockSize;

			for (int32_t i = firstBlock; i <= lastBlock; i++) {
				blocks[i].sprites.push_back(sprite);
			}
		}


		//Then put the sprites from each block into the appropriate RasterLines.
		//We can do this for each block in parallel.
#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < numBlocks; i++) {
			IntRectangle<int32_t> blockViewport(0, i * blockSize, width, blockSize);
			blockViewport = blockViewport.getIntersection(viewport);

			for (const Sprite& sprite: blocks[i].sprites) {
				auto visibleRect = blockViewport.getIntersection(sprite.position);
				if (visibleRect.isEmpty()) {
					//No need to waste processor cycles on an invisible sprite
					continue;
				}

				int32_t lastY = visibleRect.getLastY();

				for (int32_t y = visibleRect.y; y <= lastY; y++) {
					rasterLines[y].addSprite(&sprite, visibleRect.x);
				}
			}
		}
	}

public:

	/**
	 * @param width       The width of the target buffer, in pixels.
	 * @param height      The height of the target buffer, in pixels.
	 * @param pixelPacker The function to use for packing RGB values into an uint32_t.
	 */
	SpriteRenderer(int width, int height, PixelPacker *pixelPacker):
		width(width), height(height), pixelPacker(pixelPacker)
	{
		//Argument order is not a typo: We want height RasterLines with width pixels each.
		rasterLines.resize(height, width);
	}


	void render(const vector<Sprite>& sprites, uint8_t *framebuffer, size_t pitch) {
		//First distribute the sprites to the RasterLines that make up the framebuffer
		distributeSpritesToRasterLines(sprites);

		//Then render each RasterLine individually and in parallel
#pragma omp parallel for schedule(dynamic)
		for (int y = 0; y < height; y++) {
			uint8_t *framebufferLine = framebuffer + y * pitch;
			RasterLine& line = rasterLines[y];
			line.render((uint32_t *)framebufferLine, y, pixelPacker);

			//Invariant: All the RasterLines are empty when entering this method.
			//Therefore we have to empty each line again when we're done with it.
			line.clear();
		}
	}


};


}
#endif /* SPRITERENDERER_HPP_ */
