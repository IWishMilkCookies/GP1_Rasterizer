#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <cassert>
#include <iostream>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		SDL_Surface* loadSurface = IMG_Load(path.c_str());

		assert(loadSurface && "Image failed to load.");

		Texture* toReturn{ new Texture{ loadSurface } };
		return toReturn;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		Uint32 x{Uint32( uv.x * m_pSurface->w) }, y{ Uint32( uv.y * m_pSurface->h) };

		//std::cout << "x: " << uv.x << " y: " << uv.y << "\n";

		uint8_t r, g, b;

		SDL_GetRGB(m_pSurfacePixels[static_cast<uint32_t>(x + (y * m_pSurface->w))],
			m_pSurface->format,
			&r,
			&g,
			&b);

		ColorRGB pixelColor{ r/255.f, g / 255.f, b / 255.f };
		return pixelColor;
	}
}