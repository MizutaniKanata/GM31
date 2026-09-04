#pragma once

#include <string>
#include <unordered_map>

#include "component.h"
#include "renderer.h"

// OBJ / MTLのマテリアル情報
struct MODEL_MATERIAL
{
	char Name[ 256 ]{};
	MATERIAL Material{};
	char TextureName[ MAX_PATH ]{};

	ID3D11ShaderResourceView* Texture = nullptr;
};

// OBJ内のマテリアル単位の描画範囲
struct SUBSET
{
	unsigned int StartIndex = 0;
	unsigned int IndexNum = 0;

	MODEL_MATERIAL Material{};
};

// OBJ読込中だけ使うCPU側のモデルデータ
struct MODEL_OBJ
{
	VERTEX_3D* VertexArray = nullptr;
	unsigned int VertexNum = 0;

	unsigned int* IndexArray = nullptr;
	unsigned int IndexNum = 0;

	SUBSET* SubsetArray = nullptr;
	unsigned int SubsetNum = 0;
};

// GPU上へ作成したモデルデータ
struct MODEL
{
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	SUBSET* SubsetArray = nullptr;
	unsigned int SubsetNum = 0;
};

class ModelRenderer : public Component
{
private:
	static std::unordered_map<std::string, MODEL*> m_ModelPool;

	static bool LoadModel( const char* fileName, MODEL* model );
	static bool LoadObj( const char* fileName, MODEL_OBJ* modelObj );

	static bool LoadMaterial(
		const char* fileName,
		MODEL_MATERIAL** materialArray,
		unsigned int* materialNum
	);

	static void ReleaseModel( MODEL* model );

	MODEL* m_Model = nullptr;

public:
	using Component::Component;

	static void Preload( const char* fileName );
	static void UnloadAll();

	void Load( const char* fileName );
	void Draw() override;
};