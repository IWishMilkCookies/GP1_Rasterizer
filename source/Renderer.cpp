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
#define CUSTOM_MESH
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
	m_AspectRatio = static_cast<float>(m_Width) / m_Height;
	m_Camera.Initialize(m_AspectRatio, 60.f, { .0f,5.f,-30.f });
	m_MeshTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pNormalTexture = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png");
	m_pGlossTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	InitializeMesh();

	m_CurrentRendeMode = RenderMode::Texture;
	m_CurrentColorMode = ColorMode::observedArea;
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_MeshTexture;
	delete m_pNormalTexture;
	delete m_pSpecularTexture;
	delete m_pGlossTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_CanRotate)
	{
		const float meshRotationSpeed{ 50.0f };
		m_MeshWorld.worldMatrix = Matrix::CreateRotationY(meshRotationSpeed * pTimer->GetElapsed() * TO_RADIANS) * m_MeshWorld.worldMatrix;
	}
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	Uint32 clearColor{ 100 };
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, clearColor, clearColor, clearColor));

	std::vector<Vector2> raster_Vertices;

	VertexTransformationFunction();

	for (Vertex_Out& vertex : m_MeshWorld.vertices_out)
	{
		raster_Vertices.push_back(Vector2{ (vertex.position.x + 1) * 0.5f * m_Width, (1 - vertex.position.y) * 0.5f * m_Height });
	}

	std::vector<uint32_t>& meshIndeces = m_MeshWorld.indices;
	
	switch (m_MeshWorld.primitiveTopology)
	{
	case PrimitiveTopology::TriangleStrip:
		for (size_t i = 0; i + 2 < meshIndeces.size(); ++i)
		{
			int idx0{ static_cast<int>(i) };
			int idx1{ static_cast<int>(i+1) };
			int idx2{ static_cast<int>(i+2) };

			if ((i ^ 1) != i + 1)
			{
				int temp = idx1;
				idx1 = idx2;
				idx2 = temp;
			}
			RenderTriangle(idx0, idx1, idx2, raster_Vertices);

		}

		break;
	case PrimitiveTopology::TriangleList:
		for (int i = 0; i + 2 < meshIndeces.size(); i += 3)
		{
			int idx0{ i };
			int idx1{ i + 1 };
			int idx2{ i + 2 };

			RenderTriangle(idx0, idx1, idx2, raster_Vertices);
		}
		break;
	}
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction() 
{
	//Todo > W1 Projection Stage
	m_MeshWorld.vertices_out.clear();
	m_MeshWorld.vertices_out.reserve(m_MeshWorld.vertices.size());
	Matrix worldViewProjectMatrix = m_MeshWorld.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

	for (Vertex currentVertex : m_MeshWorld.vertices)
	{
		Vertex_Out vertexOut{ {}, currentVertex.color, currentVertex.uv, currentVertex.normal, currentVertex.tangent, currentVertex.viewDirection};
		vertexOut.position = worldViewProjectMatrix.TransformPoint({currentVertex.position, 1});
		currentVertex.position.x = currentVertex.position.x / (m_Camera.fov * m_AspectRatio);// / currentVertex.position.z;
		currentVertex.position.y = currentVertex.position.y / m_Camera.fov; // / currentVertex.position.z;
		
		vertexOut.position.x /= vertexOut.position.w;
		vertexOut.position.y /= vertexOut.position.w;
		vertexOut.position.z /= vertexOut.position.w;

		vertexOut.normal = m_MeshWorld.worldMatrix.TransformVector(vertexOut.normal).Normalized();
		vertexOut.viewDirection = Vector3{ vertexOut.position.x, vertexOut.position.y, vertexOut.position.z }.Normalized();
		m_MeshWorld.vertices_out.push_back(vertexOut);
	}
}

void Renderer::InitializeMesh()
{

#ifdef CUSTOM_MESH
	//Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshWorld.vertices, m_MeshWorld.indices);
	Utils::ParseOBJ("Resources/vehicle.obj", m_MeshWorld.vertices, m_MeshWorld.indices);
#else

#ifdef TRIANGLE_STRIP

		m_MeshWorld =
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
		};
#else
	m_MeshWorld =
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
		PrimitiveTopology::TriangleStrip
	};


#endif // TRIANGLE_STRIP

#endif

	const Vector3 position{ m_Camera.origin + Vector3{0, 0, 50}};
	const Vector3 rotation{ };
	const Vector3 scale{ Vector3{ 1, 1, 1 } };
	//const Vector3 scale{ Vector3{ 0.5f, 0.5f, 0.5f } };
	m_MeshWorld.worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);
}

bool dae::Renderer::IsInsideFrustrum(const Vector4& position)
{
	return position.x < -1.f || position.x > 1.f || position.y > 1.f || position.y < -1.f || position.z > 1.0f || position.z < 0.f;
}

void dae::Renderer::RenderTriangle(int idx0, int idx1, int idx2, std::vector<Vector2>& screenVertices)
{

	if (IsInsideFrustrum(m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].position) ||
		IsInsideFrustrum(m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].position) ||
		IsInsideFrustrum(m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].position))
		return;

	Vector2 p0{ screenVertices[m_MeshWorld.indices[idx0]] };
	Vector2 p1{ screenVertices[m_MeshWorld.indices[idx1]] };
	Vector2 p2{ screenVertices[m_MeshWorld.indices[idx2]] };

	Vector2 e0{ p1 - p0 };
	Vector2 e1{ p2 - p1 };
	Vector2 e2{ p0 - p2 };


	float triangleArea = Vector2::Cross(e0, e1);

	Vector2 Min{ Vector2::Min(p0,Vector2::Min(p1,p2)) };
	Vector2 Max{ Vector2::Max(p0,Vector2::Max(p1,p2)) };

	const int startX{ std::clamp(static_cast<int>(Min.x) - 1, 0, m_Width) };
	const int startY{ std::clamp(static_cast<int>(Min.y) - 1, 0, m_Height) };
	const int endX{ std::clamp(static_cast<int>(Max.x) + 1, 0, m_Width) };
	const int endY{ std::clamp(static_cast<int>(Max.y) + 1, 0, m_Height) };

	//RENDER LOGIC
	for (int px{ startX }; px < endX; ++px)
	{
		for (int py{ startY }; py < endY; ++py)
		{


			Vector2 currentPixel{ static_cast<float>(px), static_cast<float>(py) };
			float currPixMin0Crossv0 = Vector2::Cross(e0, currentPixel - p0);
			float currPixMin1Crossv1 = Vector2::Cross(e1, currentPixel - p1);
			float currPixMin2Crossv2 = Vector2::Cross(e2, currentPixel - p2);

			if (!(currPixMin0Crossv0 > 0 && currPixMin1Crossv1 > 0 && currPixMin2Crossv2 > 0))
				continue;

			float weight0 = currPixMin1Crossv1 / triangleArea;
			float weight1 = currPixMin2Crossv2 / triangleArea;
			float weight2 = currPixMin0Crossv0 / triangleArea;

			const float depthZV0{ (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].position.z) };
			const float depthZV1{ (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].position.z) };
			const float depthZV2{ (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].position.z) };
			// Calculate the Z depth at this pixel
			const float interpolatedZDepth
			{
				1.0f /
					(weight0 / depthZV0 +
					weight1 / depthZV1 +
					weight2 / depthZV2)
			};

			int pixelIdx = px + (py * m_Width);
			if (m_pDepthBufferPixels[pixelIdx] < interpolatedZDepth)
				continue;

			m_pDepthBufferPixels[pixelIdx] = interpolatedZDepth;

			switch (m_CurrentRendeMode)
			{
			case dae::RenderMode::Texture:
			{
				//W Depth
				const float depthWV0{ (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].position.w) };
				const float depthWV1{ (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].position.w) };
				const float depthWV2{ (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].position.w) };


				// Calculate the W depth at this pixel
				const float interpolatedWDepth
				{
					1.0f /
						(weight0 / depthWV0 +
						weight1 / depthWV1 +
						weight2 / depthWV2)
				};

				Vertex_Out interpolatedVertex{};

				//UV interpolate
				Vector2 uvInterpolate1{ weight0 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].uv / depthWV0) };
				Vector2 uvInterpolate2{ weight1 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].uv / depthWV1) };
				Vector2 uvInterpolate3{ weight2 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].uv / depthWV2) };

				Vector2 uvInterpolateTotal{ uvInterpolate1 + uvInterpolate2 + uvInterpolate3 };

				Vector2 uvInterpolated{ interpolatedWDepth * uvInterpolateTotal };

				interpolatedVertex.uv = uvInterpolated;

				//Normal interpolate
				Vector3 normalInterpolate1{ weight0 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].normal / depthWV0) };
				Vector3 normalInterpolate2{ weight1 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].normal / depthWV1) };
				Vector3 normalInterpolate3{ weight2 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].normal / depthWV2) };

				Vector3 normalInterpolateTotal{ normalInterpolate1 + normalInterpolate2 + normalInterpolate3 };
				Vector3 normalInterpolated{ interpolatedWDepth * normalInterpolateTotal };

				interpolatedVertex.normal = normalInterpolated.Normalized();

				//Tangent interpolate
				Vector3 tangentInterpolate1{ weight0 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].tangent / depthWV0) };
				Vector3 tangentInterpolate2{ weight1 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].tangent / depthWV1) };
				Vector3 tangentInterpolate3{ weight2 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].tangent / depthWV2) };

				Vector3 tangentInterpolateTotal{ tangentInterpolate1 + tangentInterpolate2 + tangentInterpolate3 };
				Vector3 tangentInterpolated{ interpolatedWDepth * tangentInterpolateTotal };

				interpolatedVertex.tangent = tangentInterpolated.Normalized();

				//viewdirection interpolate
				Vector3 viewDirectionInterpolate1{ weight0 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx0]].viewDirection / depthWV0) };
				Vector3 viewDirectionInterpolate2{ weight1 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx1]].viewDirection / depthWV1) };
				Vector3 viewDirectionInterpolate3{ weight2 * (m_MeshWorld.vertices_out[m_MeshWorld.indices[idx2]].viewDirection / depthWV2) };

				Vector3 viewDirectionInterpolateTotal{ viewDirectionInterpolate1 + viewDirectionInterpolate2 + viewDirectionInterpolate3 };
				Vector3 viewDirectionInterpolated{ interpolatedWDepth * viewDirectionInterpolateTotal };

				interpolatedVertex.viewDirection = viewDirectionInterpolated.Normalized();


				ColorRGB finalColor{ PixelShading(interpolatedVertex) };

				finalColor.MaxToOne();


				//Update Color in Buffer
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));

			}
			break;
			case dae::RenderMode::DepthBuffer:
			{
				float depthColor = Utils::Remap(interpolatedZDepth, 0.985f, 1.f);


				ColorRGB finalColor{ depthColor, depthColor, depthColor };


				//Update Color in Buffer
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
			break;
			}

		}
	}
}

ColorRGB dae::Renderer::PixelShading(const Vertex_Out& vertex_out)
{
	Vector3 pixelNormal{ vertex_out.normal };
	//Normal calculations
	if (m_ShowNormals)
	{
		Vector3 binormal = Vector3::Cross(vertex_out.normal, vertex_out.tangent);
		Matrix tangentSpaceAxis = Matrix{ vertex_out.tangent, binormal, vertex_out.normal, Vector3::Zero};
		auto sampledNormal{ m_pNormalTexture->Sample(vertex_out.uv) };
		
		sampledNormal = (2.f * sampledNormal) - ColorRGB{1.f, 1.f, 1.f}; // [0, 1] -> [-1, 1]

		Vector3 sampledNormalVector{sampledNormal.r, sampledNormal.g, sampledNormal.b};
		pixelNormal = tangentSpaceAxis.TransformVector(sampledNormalVector);
	}

	Vector3 lightDirection = Vector3{ .577f, -.577f , .577f }.Normalized();
	ColorRGB finalColor{ };
	float lightIntensity{ 7.f };
	float glossiness{ 25.f };
	Vector3 ambient{ .025f, .025f, .025f };
	float observedArea = std::max(Vector3::Dot(-lightDirection, pixelNormal), 0.f);


	switch (m_CurrentColorMode)
	{
	case dae::ColorMode::observedArea:
	{

		finalColor += ColorRGB{ observedArea, observedArea, observedArea };
		return finalColor;
		break;
	}
	case dae::ColorMode::Diffuse:
	{

		finalColor = Lambert(lightIntensity, m_MeshTexture->Sample(vertex_out.uv));
		return finalColor * observedArea;
		break;
		}
	case dae::ColorMode::Specular:
	{
		float exponent{m_pGlossTexture->Sample(vertex_out.uv).r * glossiness };
		finalColor = Phong(1.0f, exponent, -lightDirection, vertex_out.viewDirection, pixelNormal) * m_pSpecularTexture->Sample(vertex_out.uv);
		return finalColor;
		break;
	}
	case dae::ColorMode::Combined:
	{
		float exponent{ m_pGlossTexture->Sample(vertex_out.uv).r * glossiness };
		auto phong{ m_pSpecularTexture->Sample(vertex_out.uv) * Phong(1.0f, exponent, -lightDirection, vertex_out.viewDirection, pixelNormal) };
		auto lambert{ Lambert(lightIntensity, m_MeshTexture->Sample(vertex_out.uv)) };
		
		return (lightIntensity * lambert + phong) * observedArea;


		//const ColorRGB lambert{ 1.0f * m_MeshTexture->Sample(vertex_out.uv) / PI };
		//
		//const float phongExponent{ m_pGlossTexture->Sample(vertex_out.uv).r * glossiness };
		//
		//const ColorRGB specular{ m_pSpecularTexture->Sample(vertex_out.uv) * Phong(1.0f, phongExponent, -lightDirection, vertex_out.viewDirection, pixelNormal) };
		//
		//return (lightIntensity * lambert + specular) * observedArea;
		break;
	}
	default:
		//finalColor += ambient;
		return finalColor;
	}


}

ColorRGB Renderer::Lambert(float kd, const ColorRGB& cd)
{
	return (kd * cd) / static_cast<float>(M_PI);
}

ColorRGB Renderer::Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
{

	Vector3 reflect{ Vector3::Reflect(l,n) };
	float angle = std::max(Vector3::Dot(reflect, v), 0.f);

	float specularReflection = ks * powf(angle, exp);

	return ColorRGB{ specularReflection, specularReflection, specularReflection };
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::SwitchRenderMode()
{
	m_CurrentRendeMode = static_cast<RenderMode>((static_cast<int>(m_CurrentRendeMode) + 1)  % (static_cast<int>(RenderMode::DepthBuffer) + 1));


}

void dae::Renderer::SwitchColorMode()
{
	m_CurrentColorMode = static_cast<ColorMode>((static_cast<int>(m_CurrentColorMode) + 1) % (static_cast<int>(ColorMode::Combined) + 1));
}
