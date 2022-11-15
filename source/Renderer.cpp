//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <iostream>

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
	m_AspectRatio = static_cast<float>(m_Width) / m_Height;
	
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	std::fill_n(m_pDepthBufferPixels, m_Width*m_Height, FLT_MAX);

	Uint32 clearColor{100};
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, clearColor, clearColor, clearColor));

	std::vector<Vertex> vertices_ndc;
	std::vector<Vector2> raster_Vertices;
	std::vector<Vertex> vertices_world
	{
		//Triangle 1
		{{0.f, 2.f, 0.f}, { 1.0f, 0.0f, 0.0f } },
		{{1.5f, -1.f, 0.f}, { 1.0f, 0.0f, 0.0f } },
		{{-1.5f, -1.f, 0.f}, { 1.0f, 0.0f, 0.0f }},

		//Triangle 2
		{ { 0.0f, 4.0f, 2.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 3.0f, -2.0f, 2.0f }, { 0.0f, 1.0f, 0.0f } },
		{ { -3.0f, -2.0f, 2.0f }, { 0.0f, 0.0f, 1.0f } }
	};


	VertexTransformationFunction(vertices_world, vertices_ndc);

	for (auto& vertex : vertices_ndc)
	{
		raster_Vertices.push_back(Vector2{ (vertex.position.x + 1) * 0.5f * m_Width, (1 - vertex.position.y) * 0.5f * m_Height });
	}
	
	
	
	for (size_t i = 0; i + 2 < raster_Vertices.size(); i+=3)
	{
		Vector2& p0{ raster_Vertices[i]}; 
		Vector2& p1{ raster_Vertices[i+1]};
		Vector2& p2{ raster_Vertices[i+2]};

		Vector2 e0{p1 - p0};
		Vector2 e1{p2 - p1};
		Vector2 e2{p0 - p2};

		float triangleArea = Vector2::Cross(e0, e1);

		Vector2 Min{Vector2::Min(p0,Vector2::Min(p1,p2))};

		Vector2 Max{ Vector2::Max(p0,Vector2::Max(p1,p2))};

		//Check if bounding box parameters are within the screen.
		bool topLeftXInScreen = (0 <= Min.x);
		bool topLeftYInScreen = (Min.y >= 0);
		bool bottomRightXInScreen = (Max.x <= (m_Width - 1));
		bool bottomRightYInScreen = (Max.y <= (m_Height - 1));
		
		if (!(topLeftXInScreen && topLeftYInScreen && bottomRightXInScreen && bottomRightYInScreen))
			continue;

		//RENDER LOGIC
		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				
				if ((px < Min.x || px > Max.x) || (py < Min.y || py > Max.y))
					continue;

				int pixelIdx = px + (py * m_Width);
				Vector2 currentPixel{static_cast<float>(px), static_cast<float>(py)};
				float currPixMin0Crossv0 = Vector2::Cross(e0, currentPixel - p0);
				float currPixMin1Crossv1 = Vector2::Cross(e1, currentPixel - p1);
				float currPixMin2Crossv2 = Vector2::Cross(e2, currentPixel - p2);

				if (!(currPixMin0Crossv0 > 0 && currPixMin1Crossv1 > 0 && currPixMin2Crossv2 > 0))
					continue;
				
				float weight0 = currPixMin0Crossv0 / triangleArea;
				float weight1 = currPixMin1Crossv1 / triangleArea;
				float weight2 = currPixMin2Crossv2 / triangleArea;
				
				float distanceWeight{ (weight0 * (vertices_world[i].position.z) - m_Camera.origin.z) + 
				(weight1 * (vertices_world[i+1].position.z) - m_Camera.origin.z) + 
				(weight2 * (vertices_world[i+2].position.z) - m_Camera.origin.z)
				};


				if (m_pDepthBufferPixels[pixelIdx] < distanceWeight)
					continue;

				m_pDepthBufferPixels[pixelIdx] = distanceWeight;

				ColorRGB finalColor = weight1* vertices_world[i].color +
					weight2 * vertices_world[i + 1].color +
					weight0 * vertices_world[i + 2].color;
				

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
			}
		}

	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	vertices_out.reserve(vertices_in.size());
	for (auto currentVertex : vertices_in)
	{
		currentVertex.position = m_Camera.invViewMatrix.TransformPoint(currentVertex.position);
		float invVertexX = currentVertex.position.x / (m_Camera.fov * m_AspectRatio) / currentVertex.position.z;
		float invVertexY = currentVertex.position.y / m_Camera.fov / currentVertex.position.z;


		vertices_out.push_back(Vertex{ Vector3{ invVertexX, invVertexY, currentVertex.position.z } });
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
