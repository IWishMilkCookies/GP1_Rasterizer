#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	enum class RenderMode
	{
		Texture,
		DepthBuffer
	};

	enum class ColorMode
	{
		observedArea,
		Diffuse,
		Specular,
		Combined
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();
		bool SaveBufferToImage() const;
		void SwitchRenderMode();
		void SwitchColorMode();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		Texture* m_MeshTexture{ nullptr };
		Texture* m_pNormalTexture{ nullptr };
		Texture* m_pSpecularTexture{ nullptr };
		Texture* m_pGlossTexture{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{  };
		Mesh m_MeshWorld;
		RenderMode m_CurrentRendeMode;
		ColorMode m_CurrentColorMode;

		bool m_CanRotate = true;
		bool m_ShowNormals = false;
		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(); //W1 Version
		void InitializeMesh();
		bool IsInsideFrustrum(const Vector4& position);
		void RenderTriangle(int idx0, int idx1, int idx2, std::vector<Vector2>& screenVertices);
		ColorRGB PixelShading(const Vertex_Out& vertex_out);
		ColorRGB Lambert(float kd, const ColorRGB& cd);
		ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n);
	};
}
