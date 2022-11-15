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

//#define TRIANGLE_STRIP
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
	m_pUVGrid = Texture::LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pUVGrid;
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
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	Uint32 clearColor{ 100 };
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, clearColor, clearColor, clearColor));

	std::vector<Vertex> vertices_ndc;
	std::vector<Vector2> raster_Vertices;



#ifdef TRIANGLE_STRIP

	std::vector<Mesh> meshes_world{
		Mesh
		{
			{
				Vertex{
					{-3.f,3.f,-2.f},
					{1},
					{0, 0}},
				Vertex{
					{0.f,3.f,-2.f},
					{1},
					{0.5f, 0}},
				Vertex{
					{3.f,3.f,-2.f},
					{1},
					{1, 0}},
				Vertex{
					{-3.f,0.f,-2.f},
					{1},
					{0, 0.5f}},
				Vertex{
					{0.f,0.f,-2.f},
					{1},
					{0.5f, 0.5f}},
				Vertex{
					{3.f,0.f,-2.f},
					{1},
					{1, 0.5f}},
				Vertex{
					{-3.f,-3.f,-2.f},
					{1},
					{0, 1}},
				Vertex{
					{0.f,-3.f,-2.f},
					{1},
					{0.5f, 1}},
				Vertex{
					{3.f,-3.f,-2.f},
					{1},
					{1,1}}
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleStrip
		}
	};
#else
	std::vector<Mesh> meshes_world{
	Mesh
	{
		{
			Vertex{
				{-3.f,3.f,-2.f},
				{1},
				{0, 0}
			},
			Vertex{
				{0.f,3.f,-2.f},
				{1},
				{0.5f, 0}},
			Vertex{
				{3.f,3.f,-2.f},
				{1},
				{1, 0}},
			Vertex{
				{-3.f,0.f,-2.f},
				{1},
				{0, 0.5f}},
			Vertex{
				{0.f,0.f,-2.f},
				{1},
				{0.5f, 0.5f}},
			Vertex{
				{3.f,0.f,-2.f},
				{1},
				{1, 0.5f}},
			Vertex{
				{-3.f,-3.f,-2.f},
				{1},
				{0, 1}},
			Vertex{
				{0.f,-3.f,-2.f},
				{1},
				{0.5f, 1}},
			Vertex{
				{3.f,-3.f,-2.f},
				{1},
				{1,1}}
		},
		{
			3,0,1,	1,4,3,	4,1,2,
			2,5,4,	6,3,4,	4,7,6,
			7,4,5,	5,8,7
		},
		PrimitiveTopology::TriangleList
	}
	};

#endif // TRIANGLE_STRIP



	VertexTransformationFunction(meshes_world[0].vertices, vertices_ndc);

	for (auto& vertex : vertices_ndc)
	{
		raster_Vertices.push_back(Vector2{ (vertex.position.x + 1) * 0.5f * m_Width, (1 - vertex.position.y) * 0.5f * m_Height });
	}

	std::vector<uint32_t>& meshIndeces = meshes_world[0].indices;
#ifdef TRIANGLE_STRIP
	for (size_t i = 0; i + 2 < meshIndeces.size(); ++i)
#else
	for (size_t i = 0; i + 2 < meshIndeces.size(); i += 3)
#endif
	{

		Vector2 p0{ raster_Vertices[meshIndeces[i]] };
		Vector2 p1{ };
		Vector2 p2{ };

		Vector2 e0{};
		Vector2 e1{};
		Vector2 e2{};

#ifdef TRIANGLE_STRIP
		if ((i ^ 1) == i + 1) //If true = even, else odd.
		{
			p1 = raster_Vertices[meshIndeces[i + 1]];
			p2 = raster_Vertices[meshIndeces[i + 2]];
		}
		else
		{
			p1 = raster_Vertices[meshIndeces[i + 2]];
			p2 = raster_Vertices[meshIndeces[i + 1]];

		}
#else
		p1 = raster_Vertices[meshIndeces[i + 1]];
		p2 = raster_Vertices[meshIndeces[i + 2]];
#endif

		e0 = p1 - p0;
		e1 = p2 - p1;
		e2 = p0 - p2;


		float triangleArea = Vector2::Cross(e0, e1);

		Vector2 Min{ Vector2::Min(p0,Vector2::Min(p1,p2)) };

		Vector2 Max{ Vector2::Max(p0,Vector2::Max(p1,p2)) };

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

				Vector2 currentPixel{ static_cast<float>(px), static_cast<float>(py) };
				float currPixMin0Crossv0 = Vector2::Cross(e0, currentPixel - p0);
				float currPixMin1Crossv1 = Vector2::Cross(e1, currentPixel - p1);
				float currPixMin2Crossv2 = Vector2::Cross(e2, currentPixel - p2);

				if (!(currPixMin0Crossv0 > 0 && currPixMin1Crossv1 > 0 && currPixMin2Crossv2 > 0))
					continue;

				float weight0 = currPixMin1Crossv1 / triangleArea;
				float weight1 = currPixMin2Crossv2 / triangleArea;
				float weight2 = currPixMin0Crossv0 / triangleArea;

				//float distanceWeight{ (weight0 * (meshes_world[0].vertices[meshIndeces[i]].position.z) - m_Camera.origin.z) +
				//(weight1 * (meshes_world[0].vertices[meshIndeces[i + 1]].position.z) - m_Camera.origin.z) +
				//(weight2 * (meshes_world[0].vertices[meshIndeces[i + 2]].position.z) - m_Camera.origin.z)
				//};

				float zInterpolate1{ 1 / (meshes_world[0].vertices[meshIndeces[i]].position.z - m_Camera.origin.z) * weight0 };
				float zInterpolate2{ 1 / (meshes_world[0].vertices[meshIndeces[i + 1]].position.z - m_Camera.origin.z) * weight1};
				float zInterpolate3{ 1 / (meshes_world[0].vertices[meshIndeces[i + 2]].position.z - m_Camera.origin.z)* weight2};
				float zInterpolateTotal{1 / (zInterpolate1 + zInterpolate2 + zInterpolate3)};

				int pixelIdx = px + (py * m_Width);
				if (m_pDepthBufferPixels[pixelIdx] < zInterpolateTotal)
					continue;

				Vector2 uvInterpolate1{ weight0 * (meshes_world[0].vertices[meshIndeces[i]].uv/ meshes_world[0].vertices[meshIndeces[i]].position.z)};
				Vector2 uvInterpolate2{ weight1 * (meshes_world[0].vertices[meshIndeces[i + 1]].uv / meshes_world[0].vertices[meshIndeces[i+1]].position.z)};
				Vector2 uvInterpolate3{ weight2 * (meshes_world[0].vertices[meshIndeces[i + 2]].uv / meshes_world[0].vertices[meshIndeces[i+2]].position.z )};

				Vector2 uvInterpolateTotal{uvInterpolate1 + uvInterpolate2 + uvInterpolate3};

				auto uvInterpolated{zInterpolateTotal * uvInterpolateTotal};

				m_pDepthBufferPixels[pixelIdx] = zInterpolateTotal;

				//ColorRGB finalColor = 
				//	weight0 * meshes_world[0].vertices[meshIndeces[i]].color +
				//	weight1 * meshes_world[0].vertices[meshIndeces[i+1]].color +
				//	weight2 * meshes_world[0].vertices[meshIndeces[i+2]].color;
				//Vector2 temp{ weight0 * meshes_world[0].vertices[meshIndeces[i]].uv };
				//Vector2 temp1{ weight1 * meshes_world[0].vertices[meshIndeces[i + 1]].uv };
				//Vector2 temp2{ weight2 * meshes_world[0].vertices[meshIndeces[i + 2]].uv };
				//
				//
				//Vector2 uvInter{ temp +
				//				temp1 +
				//				temp2 };

				ColorRGB finalColor{ m_pUVGrid->Sample(uvInterpolated) };


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
