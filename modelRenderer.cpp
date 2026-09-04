#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <shlwapi.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "main.h"
#include "renderer.h"
#include "modelRenderer.h"

#pragma comment(lib, "shlwapi.lib")

namespace
{
	constexpr size_t kPathBufferSize = MAX_PATH;
	constexpr int kMaxFaceCorners = 64;

	bool ResolveFilePath(
		const char* inputPath,
		char* outPath,
		size_t outPathSize
	)
	{
		if ( inputPath == nullptr ||
			outPath == nullptr ||
			outPathSize == 0 )
		{
			return false;
		}

		outPath[ 0 ] = '\0';

		// 絶対パスなら、そのまま存在確認する
		if ( PathIsRelativeA( inputPath ) == FALSE )
		{
			if ( PathFileExistsA( inputPath ) )
			{
				strcpy_s( outPath, outPathSize, inputPath );
				return true;
			}

			return false;
		}

		// カレントディレクトリ基準で見つかる場合
		if ( PathFileExistsA( inputPath ) )
		{
			strcpy_s( outPath, outPathSize, inputPath );
			return true;
		}

		// 実行ファイルのあるフォルダから上方向へ探索する
		char exePath[ MAX_PATH ]{};

		if ( GetModuleFileNameA( nullptr, exePath, MAX_PATH ) == 0 )
		{
			return false;
		}

		char baseDirectory[ MAX_PATH ]{};
		strcpy_s( baseDirectory, exePath );
		PathRemoveFileSpecA( baseDirectory );

		for ( int depth = 0; depth < 8; ++depth )
		{
			char candidatePath[ MAX_PATH ]{};

			if ( PathCombineA(
				candidatePath,
				baseDirectory,
				inputPath
				) != nullptr &&
				PathFileExistsA( candidatePath ) )
			{
				strcpy_s( outPath, outPathSize, candidatePath );
				return true;
			}

			if ( !PathRemoveFileSpecA( baseDirectory ) )
			{
				break;
			}
		}

		return false;
	}

	void DebugPrint( const char* format, ... )
	{
		char buffer[ 2048 ]{};

		va_list args;
		va_start( args, format );

		vsprintf_s(
			buffer,
			_countof( buffer ),
			format,
			args
		);

		va_end( args );

		OutputDebugStringA( buffer );
	}

	bool ParseObjIndex(
		const char* token,
		int& positionIndex,
		int& texcoordIndex,
		int& normalIndex
	)
	{
		positionIndex = 0;
		texcoordIndex = 0;
		normalIndex = 0;

		if ( token == nullptr )
		{
			return false;
		}

		// v/vt/vn
		if ( sscanf_s(
			token,
			"%d/%d/%d",
			&positionIndex,
			&texcoordIndex,
			&normalIndex
			) == 3 )
		{
			return true;
		}

		// v//vn
		if ( sscanf_s(
			token,
			"%d//%d",
			&positionIndex,
			&normalIndex
			) == 2 )
		{
			texcoordIndex = 0;
			return true;
		}

		// v/vt
		if ( sscanf_s(
			token,
			"%d/%d",
			&positionIndex,
			&texcoordIndex
			) == 2 )
		{
			normalIndex = 0;
			return true;
		}

		// v
		if ( sscanf_s(
			token,
			"%d",
			&positionIndex
			) == 1 )
		{
			texcoordIndex = 0;
			normalIndex = 0;
			return true;
		}

		return false;
	}

	int ConvertObjIndex(
		int objIndex,
		unsigned int arraySize
	)
	{
		if ( objIndex > 0 )
		{
			const int index = objIndex - 1;

			if ( index >= 0 &&
				static_cast<unsigned int>( index ) < arraySize )
			{
				return index;
			}

			return -1;
		}

		// OBJの負インデックス: -1 は末尾要素
		if ( objIndex < 0 )
		{
			const int index =
				static_cast<int>( arraySize ) + objIndex;

			if ( index >= 0 &&
				static_cast<unsigned int>( index ) < arraySize )
			{
				return index;
			}
		}

		return -1;
	}

	void InitializeDefaultMaterial( MODEL_MATERIAL& material )
	{
		material = {};

		material.Material.Ambient =
			XMFLOAT4( 0.2f, 0.2f, 0.2f, 1.0f );

		material.Material.Diffuse =
			XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );

		material.Material.Specular =
			XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f );

		material.Material.Emission =
			XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f );

		material.Material.Shininess = 0.0f;
		material.Material.TextureEnable = FALSE;

		strcpy_s( material.Name, "default" );
		material.TextureName[ 0 ] = '\0';
		material.Texture = nullptr;
	}

	void AssignMaterialByName(
		MODEL_MATERIAL& destination,
		const char* materialName,
		const MODEL_MATERIAL* materialArray,
		unsigned int materialNum
	)
	{
		InitializeDefaultMaterial( destination );

		if ( materialName == nullptr )
		{
			return;
		}

		strcpy_s( destination.Name, materialName );

		for ( unsigned int i = 0; i < materialNum; ++i )
		{
			if ( strcmp( materialName, materialArray[ i ].Name ) == 0 )
			{
				destination = materialArray[ i ];
				destination.Texture = nullptr;
				return;
			}
		}
	}
}

std::unordered_map<std::string, MODEL*> ModelRenderer::m_ModelPool;

void ModelRenderer::Draw()
{
	if ( m_Model == nullptr )
	{
		OutputDebugStringA(
			"[ModelRenderer] Draw skipped: m_Model is nullptr\n"
		);
		return;
	}

	if ( m_Model->VertexBuffer == nullptr )
	{
		OutputDebugStringA(
			"[ModelRenderer] Draw skipped: VertexBuffer is nullptr\n"
		);
		return;
	}

	if ( m_Model->IndexBuffer == nullptr )
	{
		OutputDebugStringA(
			"[ModelRenderer] Draw skipped: IndexBuffer is nullptr\n"
		);
		return;
	}

	if ( m_Model->SubsetArray == nullptr )
	{
		OutputDebugStringA(
			"[ModelRenderer] Draw skipped: SubsetArray is nullptr\n"
		);
		return;
	}

	if ( m_Model->SubsetNum == 0 )
	{
		OutputDebugStringA(
			"[ModelRenderer] Draw skipped: SubsetNum is 0\n"
		);
		return;
	}

	ID3D11DeviceContext* context =
		Renderer::GetDeviceContext();

	// 頂点バッファ設定
	UINT stride = sizeof( VERTEX_3D );
	UINT offset = 0;

	context->IASetVertexBuffers(
		0,
		1,
		&m_Model->VertexBuffer,
		&stride,
		&offset
	);

	// インデックスバッファ設定
	context->IASetIndexBuffer(
		m_Model->IndexBuffer,
		DXGI_FORMAT_R32_UINT,
		0
	);

	// 三角形リストとして描画
	context->IASetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);

	for ( unsigned int subsetIndex = 0;
		 subsetIndex < m_Model->SubsetNum;
		 ++subsetIndex )
	{
		SUBSET& subset =
			m_Model->SubsetArray[ subsetIndex ];

		if ( subset.IndexNum == 0 )
		{
			continue;
		}

		MATERIAL material = subset.Material.Material;

		ID3D11ShaderResourceView* texture =
			subset.Material.Texture;

		material.TextureEnable =
			texture != nullptr ? TRUE : FALSE;

		Renderer::SetMaterial( material );

		// テクスチャ未設定ならt0を明示的にnullptrへする
		context->PSSetShaderResources(
			0,
			1,
			&texture
		);

		context->DrawIndexed(
			subset.IndexNum,
			subset.StartIndex,
			0
		);
	}
}

void ModelRenderer::Preload( const char* fileName )
{
	if ( fileName == nullptr )
	{
		return;
	}

	char resolvedPath[ MAX_PATH ]{};

	if ( !ResolveFilePath(
		fileName,
		resolvedPath,
		_countof( resolvedPath )
		) )
	{
		DebugPrint(
			"[ModelRenderer] Preload failed: file not found: %s\n",
			fileName
		);
		return;
	}

	const std::string cacheKey = resolvedPath;

	if ( m_ModelPool.find( cacheKey ) != m_ModelPool.end() )
	{
		return;
	}

	MODEL* model = new MODEL{};

	if ( !LoadModel( resolvedPath, model ) )
	{
		ReleaseModel( model );

		DebugPrint(
			"[ModelRenderer] Preload failed: %s\n",
			resolvedPath
		);

		return;
	}

	m_ModelPool.emplace( cacheKey, model );

	DebugPrint(
		"[ModelRenderer] Preload success: %s\n",
		resolvedPath
	);
}

void ModelRenderer::UnloadAll()
{
	for ( auto& pair : m_ModelPool )
	{
		ReleaseModel( pair.second );
	}

	m_ModelPool.clear();
}

void ModelRenderer::Load( const char* fileName )
{
	m_Model = nullptr;

	if ( fileName == nullptr )
	{
		return;
	}

	char resolvedPath[ MAX_PATH ]{};

	if ( !ResolveFilePath(
		fileName,
		resolvedPath,
		_countof( resolvedPath )
		) )
	{
		DebugPrint(
			"[ModelRenderer] Load failed: file not found: %s\n",
			fileName
		);

		return;
	}

	const std::string cacheKey = resolvedPath;

	const auto found =
		m_ModelPool.find( cacheKey );

	if ( found != m_ModelPool.end() )
	{
		m_Model = found->second;
		return;
	}

	MODEL* model = new MODEL{};

	if ( !LoadModel( resolvedPath, model ) )
	{
		ReleaseModel( model );

		DebugPrint(
			"[ModelRenderer] LoadModel failed: %s\n",
			resolvedPath
		);

		return;
	}

	m_ModelPool.emplace( cacheKey, model );
	m_Model = model;

	DebugPrint(
		"[ModelRenderer] Load success: %s\n",
		resolvedPath
	);
}

bool ModelRenderer::LoadModel(
	const char* fileName,
	MODEL* model
)
{
	if ( fileName == nullptr || model == nullptr )
	{
		return false;
	}

	MODEL_OBJ modelObj{};

	if ( !LoadObj( fileName, &modelObj ) )
	{
		return false;
	}

	DebugPrint(
		"[ModelRenderer] OBJ loaded: %s, vertices=%u, indices=%u, subsets=%u\n",
		fileName,
		modelObj.VertexNum,
		modelObj.IndexNum,
		modelObj.SubsetNum
	);

	if ( modelObj.VertexArray == nullptr ||
		modelObj.IndexArray == nullptr ||
		modelObj.SubsetArray == nullptr ||
		modelObj.VertexNum == 0 ||
		modelObj.IndexNum == 0 ||
		modelObj.SubsetNum == 0 )
	{
		delete[] modelObj.VertexArray;
		delete[] modelObj.IndexArray;
		delete[] modelObj.SubsetArray;

		return false;
	}

	ID3D11Device* device = Renderer::GetDevice();

	if ( device == nullptr )
	{
		delete[] modelObj.VertexArray;
		delete[] modelObj.IndexArray;
		delete[] modelObj.SubsetArray;

		OutputDebugStringA(
			"[ModelRenderer] LoadModel failed: D3D device is nullptr\n"
		);

		return false;
	}

	// 頂点バッファ作成
	{
		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth =
			sizeof( VERTEX_3D ) * modelObj.VertexNum;
		vertexBufferDesc.BindFlags =
			D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexData{};
		vertexData.pSysMem = modelObj.VertexArray;

		const HRESULT result = device->CreateBuffer(
			&vertexBufferDesc,
			&vertexData,
			&model->VertexBuffer
		);

		if ( FAILED( result ) )
		{
			DebugPrint(
				"[ModelRenderer] Create vertex buffer failed: 0x%08X\n",
				static_cast<unsigned int>( result )
			);

			delete[] modelObj.VertexArray;
			delete[] modelObj.IndexArray;
			delete[] modelObj.SubsetArray;

			return false;
		}
	}

	// インデックスバッファ作成
	{
		D3D11_BUFFER_DESC indexBufferDesc{};
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth =
			sizeof( unsigned int ) * modelObj.IndexNum;
		indexBufferDesc.BindFlags =
			D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA indexData{};
		indexData.pSysMem = modelObj.IndexArray;

		const HRESULT result = device->CreateBuffer(
			&indexBufferDesc,
			&indexData,
			&model->IndexBuffer
		);

		if ( FAILED( result ) )
		{
			DebugPrint(
				"[ModelRenderer] Create index buffer failed: 0x%08X\n",
				static_cast<unsigned int>( result )
			);

			model->VertexBuffer->Release();
			model->VertexBuffer = nullptr;

			delete[] modelObj.VertexArray;
			delete[] modelObj.IndexArray;
			delete[] modelObj.SubsetArray;

			return false;
		}
	}

	model->SubsetArray =
		new SUBSET[ modelObj.SubsetNum ]{};

	model->SubsetNum = modelObj.SubsetNum;

	for ( unsigned int subsetIndex = 0;
		 subsetIndex < modelObj.SubsetNum;
		 ++subsetIndex )
	{
		model->SubsetArray[ subsetIndex ] =
			modelObj.SubsetArray[ subsetIndex ];

		model->SubsetArray[ subsetIndex ]
			.Material.Texture = nullptr;

		const char* texturePath =
			modelObj.SubsetArray[ subsetIndex ]
			.Material.TextureName;

		if ( texturePath == nullptr || texturePath[ 0 ] == '\0' )
		{
			model->SubsetArray[ subsetIndex ]
				.Material.Material.TextureEnable = FALSE;

			continue;
		}

		char resolvedTexturePath[ MAX_PATH ]{};

		if ( !ResolveFilePath(
			texturePath,
			resolvedTexturePath,
			_countof( resolvedTexturePath )
			) )
		{
			DebugPrint(
				"[ModelRenderer] Texture not found: %s\n",
				texturePath
			);

			model->SubsetArray[ subsetIndex ]
				.Material.Material.TextureEnable = FALSE;

			continue;
		}

		wchar_t wideTexturePath[ MAX_PATH ]{};

		const size_t converted =
			mbstowcs(
				wideTexturePath,
				resolvedTexturePath,
				_countof( wideTexturePath ) - 1
			);

		if ( converted == static_cast<size_t>( -1 ) )
		{
			DebugPrint(
				"[ModelRenderer] Texture path conversion failed: %s\n",
				resolvedTexturePath
			);

			continue;
		}

		wideTexturePath[ converted ] = L'\0';

		TexMetadata metadata{};
		ScratchImage image{};

		HRESULT result = LoadFromWICFile(
			wideTexturePath,
			WIC_FLAGS_NONE,
			&metadata,
			image
		);

		if ( FAILED( result ) )
		{
			DebugPrint(
				"[ModelRenderer] Load texture failed: 0x%08X, %s\n",
				static_cast<unsigned int>( result ),
				resolvedTexturePath
			);

			continue;
		}

		result = CreateShaderResourceView(
			device,
			image.GetImages(),
			image.GetImageCount(),
			metadata,
			&model->SubsetArray[ subsetIndex ]
				.Material.Texture
		);

		if ( FAILED( result ) )
		{
			DebugPrint(
				"[ModelRenderer] Create texture SRV failed: 0x%08X, %s\n",
				static_cast<unsigned int>( result ),
				resolvedTexturePath
			);

			continue;
		}

		model->SubsetArray[ subsetIndex ]
			.Material.Material.TextureEnable = TRUE;
	}

	delete[] modelObj.VertexArray;
	delete[] modelObj.IndexArray;
	delete[] modelObj.SubsetArray;

	return true;
}

bool ModelRenderer::LoadObj(
	const char* fileName,
	MODEL_OBJ* modelObj
)
{
	if ( fileName == nullptr || modelObj == nullptr )
	{
		return false;
	}

	*modelObj = {};

	char resolvedObjPath[ MAX_PATH ]{};
	const char* objPath = fileName;

	if ( ResolveFilePath(
		fileName,
		resolvedObjPath,
		_countof( resolvedObjPath )
		) )
	{
		objPath = resolvedObjPath;
	}

	FILE* file = nullptr;

	if ( fopen_s( &file, objPath, "rt" ) != 0 ||
		file == nullptr )
	{
		DebugPrint(
			"[ModelRenderer] Failed to open OBJ: %s\n",
			objPath
		);

		return false;
	}

	char directoryPath[ MAX_PATH ]{};
	strcpy_s( directoryPath, objPath );
	PathRemoveFileSpecA( directoryPath );

	unsigned int positionCount = 0;
	unsigned int normalCount = 0;
	unsigned int texcoordCount = 0;
	unsigned int vertexCount = 0;
	unsigned int indexCount = 0;
	unsigned int subsetCount = 0;

	char line[ 4096 ]{};

	// 1回目: 必要な配列サイズを数える
	while ( fgets( line, sizeof( line ), file ) != nullptr )
	{
		if ( strncmp( line, "v ", 2 ) == 0 )
		{
			++positionCount;
		}
		else if ( strncmp( line, "vn ", 3 ) == 0 )
		{
			++normalCount;
		}
		else if ( strncmp( line, "vt ", 3 ) == 0 )
		{
			++texcoordCount;
		}
		else if ( strncmp( line, "usemtl ", 7 ) == 0 )
		{
			++subsetCount;
		}
		else if ( strncmp( line, "f ", 2 ) == 0 )
		{
			int cornerCount = 0;

			char* context = nullptr;
			char* token = strtok_s(
				line + 2,
				" \t\r\n",
				&context
			);

			while ( token != nullptr )
			{
				++cornerCount;

				token = strtok_s(
					nullptr,
					" \t\r\n",
					&context
				);
			}

			if ( cornerCount >= 3 )
			{
				vertexCount +=
					static_cast<unsigned int>( cornerCount );

				indexCount +=
					static_cast<unsigned int>(
						( cornerCount - 2 ) * 3
					);
			}
		}
	}

	if ( subsetCount == 0 )
	{
		subsetCount = 1;
	}

	if ( positionCount == 0 ||
		vertexCount == 0 ||
		indexCount == 0 )
	{
		DebugPrint(
			"[ModelRenderer] OBJ has no drawable faces: %s "
			"(positions=%u, vertices=%u, indices=%u)\n",
			objPath,
			positionCount,
			vertexCount,
			indexCount
		);

		fclose( file );
		return false;
	}

	XMFLOAT3* positions =
		new XMFLOAT3[ positionCount ]{};

	XMFLOAT3* normals =
		normalCount > 0
		? new XMFLOAT3[ normalCount ]{}
	: nullptr;

	XMFLOAT2* texcoords =
		texcoordCount > 0
		? new XMFLOAT2[ texcoordCount ]{}
	: nullptr;

	modelObj->VertexArray =
		new VERTEX_3D[ vertexCount ]{};

	modelObj->VertexNum = vertexCount;

	modelObj->IndexArray =
		new unsigned int[ indexCount ] {};

	modelObj->IndexNum = indexCount;

	modelObj->SubsetArray =
		new SUBSET[ subsetCount ]{};

	modelObj->SubsetNum = subsetCount;

	fseek( file, 0, SEEK_SET );

	unsigned int positionIndex = 0;
	unsigned int normalIndex = 0;
	unsigned int texcoordIndex = 0;

	unsigned int vertexIndex = 0;
	unsigned int indexIndex = 0;
	unsigned int subsetIndex = 0;

	bool hasActiveSubset = false;

	MODEL_MATERIAL* materialArray = nullptr;
	unsigned int materialNum = 0;

	while ( fgets( line, sizeof( line ), file ) != nullptr )
	{
		if ( strncmp( line, "mtllib ", 7 ) == 0 )
		{
			char materialFileName[ MAX_PATH ]{};

			if ( sscanf_s(
				line + 7,
				"%259s",
				materialFileName,
				static_cast<unsigned>(
				_countof( materialFileName )
				)
				) == 1 )
			{
				char materialPath[ MAX_PATH ]{};

				if ( PathCombineA(
					materialPath,
					directoryPath,
					materialFileName
					) != nullptr )
				{
					delete[] materialArray;
					materialArray = nullptr;
					materialNum = 0;

					LoadMaterial(
						materialPath,
						&materialArray,
						&materialNum
					);
				}
			}
		}
		else if ( strncmp( line, "v ", 2 ) == 0 )
		{
			if ( positionIndex < positionCount )
			{
				sscanf_s(
					line + 2,
					"%f %f %f",
					&positions[ positionIndex ].x,
					&positions[ positionIndex ].y,
					&positions[ positionIndex ].z
				);

				++positionIndex;
			}
		}
		else if ( strncmp( line, "vn ", 3 ) == 0 )
		{
			if ( normals != nullptr &&
				normalIndex < normalCount )
			{
				sscanf_s(
					line + 3,
					"%f %f %f",
					&normals[ normalIndex ].x,
					&normals[ normalIndex ].y,
					&normals[ normalIndex ].z
				);

				++normalIndex;
			}
		}
		else if ( strncmp( line, "vt ", 3 ) == 0 )
		{
			if ( texcoords != nullptr &&
				texcoordIndex < texcoordCount )
			{
				sscanf_s(
					line + 3,
					"%f %f",
					&texcoords[ texcoordIndex ].x,
					&texcoords[ texcoordIndex ].y
				);

				texcoords[ texcoordIndex ].y =
					1.0f - texcoords[ texcoordIndex ].y;

				++texcoordIndex;
			}
		}
		else if ( strncmp( line, "usemtl ", 7 ) == 0 )
		{
			if ( subsetIndex >= modelObj->SubsetNum )
			{
				continue;
			}

			if ( hasActiveSubset )
			{
				SUBSET& previousSubset =
					modelObj->SubsetArray[
						subsetIndex - 1
					];

				previousSubset.IndexNum =
					indexIndex -
					previousSubset.StartIndex;
			}

			char materialName[ 256 ]{};

			sscanf_s(
				line + 7,
				"%255s",
				materialName,
				static_cast<unsigned>(
				_countof( materialName )
			)
			);

			SUBSET& subset =
				modelObj->SubsetArray[ subsetIndex ];

			subset.StartIndex = indexIndex;
			subset.IndexNum = 0;

			AssignMaterialByName(
				subset.Material,
				materialName,
				materialArray,
				materialNum
			);

			hasActiveSubset = true;
			++subsetIndex;
		}
		else if ( strncmp( line, "f ", 2 ) == 0 )
		{
			if ( !hasActiveSubset )
			{
				SUBSET& subset =
					modelObj->SubsetArray[ 0 ];

				subset.StartIndex = indexIndex;
				subset.IndexNum = 0;

				InitializeDefaultMaterial(
					subset.Material
				);

				hasActiveSubset = true;
				subsetIndex = 1;
			}

			int positionIndices[ kMaxFaceCorners ]{};
			int texcoordIndices[ kMaxFaceCorners ]{};
			int normalIndices[ kMaxFaceCorners ]{};

			int cornerCount = 0;

			char* context = nullptr;
			char* token = strtok_s(
				line + 2,
				" \t\r\n",
				&context
			);

			while ( token != nullptr &&
				   cornerCount < kMaxFaceCorners )
			{
				if ( ParseObjIndex(
					token,
					positionIndices[ cornerCount ],
					texcoordIndices[ cornerCount ],
					normalIndices[ cornerCount ]
					) )
				{
					++cornerCount;
				}

				token = strtok_s(
					nullptr,
					" \t\r\n",
					&context
				);
			}

			if ( cornerCount < 3 )
			{
				continue;
			}

			const unsigned int faceStartVertex =
				vertexIndex;

			for ( int corner = 0;
				 corner < cornerCount;
				 ++corner )
			{
				if ( vertexIndex >= modelObj->VertexNum )
				{
					break;
				}

				VERTEX_3D& vertex =
					modelObj->VertexArray[ vertexIndex ];

				const int positionArrayIndex =
					ConvertObjIndex(
						positionIndices[ corner ],
						positionCount
					);

				const int texcoordArrayIndex =
					ConvertObjIndex(
						texcoordIndices[ corner ],
						texcoordCount
					);

				const int normalArrayIndex =
					ConvertObjIndex(
						normalIndices[ corner ],
						normalCount
					);

				if ( positionArrayIndex >= 0 )
				{
					vertex.Position =
						positions[ positionArrayIndex ];
				}

				if ( texcoords != nullptr &&
					texcoordArrayIndex >= 0 )
				{
					vertex.TexCoord =
						texcoords[ texcoordArrayIndex ];
				}
				else
				{
					vertex.TexCoord =
						XMFLOAT2( 0.0f, 0.0f );
				}

				if ( normals != nullptr &&
					normalArrayIndex >= 0 )
				{
					vertex.Normal =
						normals[ normalArrayIndex ];
				}
				else
				{
					vertex.Normal =
						XMFLOAT3( 0.0f, 1.0f, 0.0f );
				}

				vertex.Diffuse =
					XMFLOAT4(
						1.0f,
						1.0f,
						1.0f,
						1.0f
					);

				++vertexIndex;
			}

			// N角形を扇形分割して三角形リストにする
			for ( int corner = 1;
				 corner < cornerCount - 1;
				 ++corner )
			{
				if ( indexIndex + 2 >=
					modelObj->IndexNum )
				{
					break;
				}

				modelObj->IndexArray[ indexIndex++ ] =
					faceStartVertex;

				modelObj->IndexArray[ indexIndex++ ] =
					faceStartVertex + corner;

				modelObj->IndexArray[ indexIndex++ ] =
					faceStartVertex + corner + 1;
			}
		}
	}

	fclose( file );

	if ( hasActiveSubset && subsetIndex > 0 )
	{
		SUBSET& lastSubset =
			modelObj->SubsetArray[ subsetIndex - 1 ];

		lastSubset.IndexNum =
			indexIndex - lastSubset.StartIndex;
	}

	delete[] positions;
	delete[] normals;
	delete[] texcoords;
	delete[] materialArray;

	if ( vertexIndex == 0 || indexIndex == 0 )
	{
		delete[] modelObj->VertexArray;
		delete[] modelObj->IndexArray;
		delete[] modelObj->SubsetArray;

		*modelObj = {};

		DebugPrint(
			"[ModelRenderer] OBJ parse produced no render data: %s\n",
			objPath
		);

		return false;
	}

	return true;
}

bool ModelRenderer::LoadMaterial(
	const char* fileName,
	MODEL_MATERIAL** materialArray,
	unsigned int* materialNum
)
{
	if ( materialArray == nullptr ||
		materialNum == nullptr )
	{
		return false;
	}

	*materialArray = nullptr;
	*materialNum = 0;

	if ( fileName == nullptr )
	{
		return false;
	}

	char resolvedMtlPath[ MAX_PATH ]{};
	const char* materialPath = fileName;

	if ( ResolveFilePath(
		fileName,
		resolvedMtlPath,
		_countof( resolvedMtlPath )
		) )
	{
		materialPath = resolvedMtlPath;
	}

	FILE* file = nullptr;

	if ( fopen_s( &file, materialPath, "rt" ) != 0 ||
		file == nullptr )
	{
		DebugPrint(
			"[ModelRenderer] Failed to open MTL: %s\n",
			materialPath
		);

		return false;
	}

	char materialDirectory[ MAX_PATH ]{};
	strcpy_s( materialDirectory, materialPath );
	PathRemoveFileSpecA( materialDirectory );

	unsigned int count = 0;
	char line[ 4096 ]{};

	// 1回目: newmtl の数を数える
	while ( fgets( line, sizeof( line ), file ) != nullptr )
	{
		if ( strncmp( line, "newmtl ", 7 ) == 0 )
		{
			++count;
		}
	}

	if ( count == 0 )
	{
		fclose( file );
		return false;
	}

	MODEL_MATERIAL* materials =
		new MODEL_MATERIAL[ count ]{};

	fseek( file, 0, SEEK_SET );

	int currentMaterial = -1;

	while ( fgets( line, sizeof( line ), file ) != nullptr )
	{
		if ( strncmp( line, "newmtl ", 7 ) == 0 )
		{
			++currentMaterial;

			if ( currentMaterial >=
				static_cast<int>( count ) )
			{
				break;
			}

			InitializeDefaultMaterial(
				materials[ currentMaterial ]
			);

			sscanf_s(
				line + 7,
				"%255s",
				materials[ currentMaterial ].Name,
				static_cast<unsigned>(
				_countof(
				materials[ currentMaterial ].Name
			)
			)
			);
		}
		else if ( currentMaterial < 0 )
		{
			continue;
		}
		else if ( strncmp( line, "Ka ", 3 ) == 0 )
		{
			sscanf_s(
				line + 3,
				"%f %f %f",
				&materials[ currentMaterial ]
					.Material.Ambient.x,
				&materials[ currentMaterial ]
					.Material.Ambient.y,
				&materials[ currentMaterial ]
					.Material.Ambient.z
			);

			materials[ currentMaterial ]
				.Material.Ambient.w = 1.0f;
		}
		else if ( strncmp( line, "Kd ", 3 ) == 0 )
		{
			sscanf_s(
				line + 3,
				"%f %f %f",
				&materials[ currentMaterial ]
					.Material.Diffuse.x,
				&materials[ currentMaterial ]
					.Material.Diffuse.y,
				&materials[ currentMaterial ]
					.Material.Diffuse.z
			);

			materials[ currentMaterial ]
				.Material.Diffuse.w = 1.0f;
		}
		else if ( strncmp( line, "Ks ", 3 ) == 0 )
		{
			sscanf_s(
				line + 3,
				"%f %f %f",
				&materials[ currentMaterial ]
					.Material.Specular.x,
				&materials[ currentMaterial ]
					.Material.Specular.y,
				&materials[ currentMaterial ]
					.Material.Specular.z
			);

			materials[ currentMaterial ]
				.Material.Specular.w = 1.0f;
		}
		else if ( strncmp( line, "Ns ", 3 ) == 0 )
		{
			sscanf_s(
				line + 3,
				"%f",
				&materials[ currentMaterial ]
					.Material.Shininess
			);
		}
		else if ( strncmp( line, "d ", 2 ) == 0 )
		{
			sscanf_s(
				line + 2,
				"%f",
				&materials[ currentMaterial ]
					.Material.Diffuse.w
			);
		}
		else if ( strncmp( line, "Tr ", 3 ) == 0 )
		{
			float transparency = 0.0f;

			sscanf_s(
				line + 3,
				"%f",
				&transparency
			);

			materials[ currentMaterial ]
				.Material.Diffuse.w =
				1.0f - transparency;
		}
		else if ( strncmp( line, "map_Kd ", 7 ) == 0 )
		{
			char textureFileName[ MAX_PATH ]{};

			if ( sscanf_s(
				line + 7,
				"%259s",
				textureFileName,
				static_cast<unsigned>(
				_countof( textureFileName )
				)
				) == 1 )
			{
				if ( PathIsRelativeA( textureFileName ) )
				{
					char texturePath[ MAX_PATH ]{};

					if ( PathCombineA(
						texturePath,
						materialDirectory,
						textureFileName
						) != nullptr )
					{
						strcpy_s(
							materials[ currentMaterial ]
								.TextureName,
							texturePath
						);
					}
				}
				else
				{
					strcpy_s(
						materials[ currentMaterial ]
							.TextureName,
						textureFileName
					);
				}
			}
		}
	}

	fclose( file );

	*materialArray = materials;
	*materialNum = count;

	return true;
}

void ModelRenderer::ReleaseModel( MODEL* model )
{
	if ( model == nullptr )
	{
		return;
	}

	if ( model->VertexBuffer != nullptr )
	{
		model->VertexBuffer->Release();
		model->VertexBuffer = nullptr;
	}

	if ( model->IndexBuffer != nullptr )
	{
		model->IndexBuffer->Release();
		model->IndexBuffer = nullptr;
	}

	if ( model->SubsetArray != nullptr )
	{
		for ( unsigned int subsetIndex = 0;
			 subsetIndex < model->SubsetNum;
			 ++subsetIndex )
		{
			ID3D11ShaderResourceView* texture =
				model->SubsetArray[ subsetIndex ]
				.Material.Texture;

			if ( texture != nullptr )
			{
				texture->Release();

				model->SubsetArray[ subsetIndex ]
					.Material.Texture = nullptr;
			}
		}

		delete[] model->SubsetArray;
		model->SubsetArray = nullptr;
	}

	model->SubsetNum = 0;

	delete model;
}