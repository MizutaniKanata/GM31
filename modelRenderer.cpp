#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "main.h"
#include "renderer.h"
#include "modelRenderer.h"

namespace
{
    bool ResolveFilePath(const char* inputPath, char* outPath, size_t outSize)
    {
        if (inputPath == nullptr || outPath == nullptr || outSize == 0)
        {
            return false;
        }

        outPath[0] = '\0';

        if (PathIsRelativeA(inputPath) == FALSE)
        {
            if (PathFileExistsA(inputPath))
            {
                strcpy_s(outPath, outSize, inputPath);
                return true;
            }
            return false;
        }

        if (PathFileExistsA(inputPath))
        {
            strcpy_s(outPath, outSize, inputPath);
            return true;
        }

        char exePath[MAX_PATH];
        if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) == 0)
        {
            return false;
        }

        char baseDir[MAX_PATH];
        strcpy_s(baseDir, exePath);
        PathRemoveFileSpecA(baseDir);

        for (int depth = 0; depth < 8; depth++)
        {
            char candidate[MAX_PATH];
            if (PathCombineA(candidate, baseDir, inputPath) != nullptr && PathFileExistsA(candidate))
            {
                strcpy_s(outPath, outSize, candidate);
                return true;
            }

            if (!PathRemoveFileSpecA(baseDir))
            {
                break;
            }
        }

        return false;
    }
}


std::unordered_map<std::string, MODEL*> ModelRenderer::m_ModelPool;




void ModelRenderer::Draw()
{
    if (m_Model == nullptr || m_Model->VertexBuffer == nullptr || m_Model->IndexBuffer == nullptr || m_Model->SubsetArray == nullptr || m_Model->SubsetNum == 0)
    {
        return;
    }

    // 頂点バッファ設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_Model->VertexBuffer, &stride, &offset);

    // インデックスバッファ設定
    Renderer::GetDeviceContext()->IASetIndexBuffer(m_Model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // プリミティブトポロジー設定
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (unsigned int i = 0; i < m_Model->SubsetNum; i++)
    {
        if (m_Model->SubsetArray[i].IndexNum == 0)
        {
            continue;
        }

        // マテリアル設定 (shader reads MATERIAL::TextureEnable, not MODEL_MATERIAL::TextureEnable)
        MATERIAL mat = m_Model->SubsetArray[i].Material.Material;
        ID3D11ShaderResourceView* texture = m_Model->SubsetArray[i].Material.Texture;
        mat.TextureEnable = texture ? TRUE : FALSE;
        Renderer::SetMaterial(mat);

        // テクスチャ設定（無い場合はスロットをクリアして他オブジェクトのテクスチャを参照しない）
        if (texture)
        {
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
        }
        else
        {
            ID3D11ShaderResourceView* nullTex = nullptr;
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &nullTex);
        }

        // ポリゴン描画
        Renderer::GetDeviceContext()->DrawIndexed(m_Model->SubsetArray[i].IndexNum, m_Model->SubsetArray[i].StartIndex, 0);
    }
}

void ModelRenderer::Preload(const char* FileName)
{
    if (m_ModelPool.count(FileName) > 0)
    {
        return;
    }

    MODEL* model = new MODEL;
    LoadModel(FileName, model);
}





void ModelRenderer::UnloadAll()
{
    for (std::pair<const std::string, MODEL*> pair : m_ModelPool)
    {
        pair.second->VertexBuffer->Release();
        pair.second->IndexBuffer->Release();

        for (unsigned int i = 0; i < pair.second->SubsetNum; i++)
        {
            if (pair.second->SubsetArray[i].Material.Texture)
            {
                pair.second->SubsetArray[i].Material.Texture->Release();
            }
        }

        delete[] pair.second->SubsetArray;
        delete pair.second;
    }

    m_ModelPool.clear();
}

void ModelRenderer::Load(const char* FileName)
{
    if (m_ModelPool.count(FileName) > 0)
    {
        m_Model = m_ModelPool[FileName];
        return;
    }

    m_Model = new MODEL;
    LoadModel(FileName, m_Model);

    m_ModelPool[FileName] = m_Model;
}


void ModelRenderer::LoadModel(const char* FileName, MODEL* Model)
{
    char resolvedModelPath[MAX_PATH];
    const char* modelPath = FileName;
    if (ResolveFilePath(FileName, resolvedModelPath, _countof(resolvedModelPath)))
    {
        modelPath = resolvedModelPath;
    }

    MODEL_OBJ modelObj;
    LoadObj(modelPath, &modelObj);

    // Debug info to confirm model data loaded
    {
        char buf[256];
        sprintf_s(buf, "[ModelRenderer] %s V=%u I=%u S=%u\n",
            modelPath ? modelPath : "(null)",
            modelObj.VertexNum, modelObj.IndexNum, modelObj.SubsetNum);
        OutputDebugStringA(buf);
    }

    if (modelObj.VertexNum == 0 || modelObj.IndexNum == 0 || modelObj.VertexArray == nullptr || modelObj.IndexArray == nullptr)
    {
        // Avoid creating invalid buffers; keep model empty
        Model->VertexBuffer = nullptr;
        Model->IndexBuffer = nullptr;
        Model->SubsetArray = nullptr;
        Model->SubsetNum = 0;
        return;
    }

    // 頂点バッファ生成
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(VERTEX_3D) * modelObj.VertexNum;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.pSysMem = modelObj.VertexArray;

        Renderer::GetDevice()->CreateBuffer(&bd, &sd, &Model->VertexBuffer);
    }

    // インデックスバッファ生成
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(unsigned int) * modelObj.IndexNum;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.pSysMem = modelObj.IndexArray;

        Renderer::GetDevice()->CreateBuffer(&bd, &sd, &Model->IndexBuffer);
    }

    // サブセット設定
    {
        Model->SubsetArray = new SUBSET[modelObj.SubsetNum];
        Model->SubsetNum = modelObj.SubsetNum;

        for (unsigned int i = 0; i < modelObj.SubsetNum; i++)
        {
            Model->SubsetArray[i].StartIndex = modelObj.SubsetArray[i].StartIndex;
            Model->SubsetArray[i].IndexNum = modelObj.SubsetArray[i].IndexNum;

            Model->SubsetArray[i].Material.Material = modelObj.SubsetArray[i].Material.Material;
            Model->SubsetArray[i].Material.Material.TextureEnable = FALSE;

            Model->SubsetArray[i].Material.Texture = nullptr;

            // テクスチャ読み込み
            TexMetadata metadata;
            ScratchImage image;
            char resolvedTexPath[MAX_PATH];
            const char* texName = modelObj.SubsetArray[i].Material.TextureName;
            if (ResolveFilePath(texName, resolvedTexPath, _countof(resolvedTexPath)))
            {
                texName = resolvedTexPath;
            }
            if (texName != nullptr && texName[0] != '\0')
            {
                wchar_t wc[MAX_PATH];
                size_t converted = 0;
                wc[0] = L'\0';
                mbstowcs_s(&converted, wc, _countof(wc), texName, _TRUNCATE);

                if (SUCCEEDED(LoadFromWICFile(wc, WIC_FLAGS_NONE, &metadata, image)))
                {
                    CreateShaderResourceView(
                        Renderer::GetDevice(),
                        image.GetImages(),
                        image.GetImageCount(),
                        metadata,
                        &Model->SubsetArray[i].Material.Texture
                    );
                }
            }

            Model->SubsetArray[i].Material.Material.TextureEnable =
                Model->SubsetArray[i].Material.Texture ? TRUE : FALSE;
        }
    }

    delete[] modelObj.VertexArray;
    delete[] modelObj.IndexArray;
    delete[] modelObj.SubsetArray;
}












// モデル読込 ///////////////////////////////////////////////////////////
void ModelRenderer::LoadObj(const char* FileName, MODEL_OBJ* ModelObj)
{
    // Initialize outputs for safe early returns
    ModelObj->VertexArray = nullptr;
    ModelObj->VertexNum = 0;
    ModelObj->IndexArray = nullptr;
    ModelObj->IndexNum = 0;
    ModelObj->SubsetArray = nullptr;
    ModelObj->SubsetNum = 0;

    char resolvedObjPath[MAX_PATH];
    const char* objPath = FileName;
    if (ResolveFilePath(FileName, resolvedObjPath, _countof(resolvedObjPath)))
    {
        objPath = resolvedObjPath;
    }

    char dir[MAX_PATH];
    strcpy(dir, objPath);
    PathRemoveFileSpec(dir);

    XMFLOAT3* positionArray;
    XMFLOAT3* normalArray;
    XMFLOAT2* texcoordArray;

    unsigned int    positionNum = 0;
    unsigned int    normalNum = 0;
    unsigned int    texcoordNum = 0;
    unsigned int    vertexNum = 0;
    unsigned int    indexNum = 0;
    unsigned int    in = 0;
    unsigned int    subsetNum = 0;

    MODEL_MATERIAL* materialArray = nullptr;
    unsigned int    materialNum = 0;

    char str[256];
    char* s;
    char c;

    FILE* file;
    file = nullptr;
    if (fopen_s(&file, objPath, "rt") != 0 || file == nullptr)
    {
        char buf[1024];
        char cwd[MAX_PATH];
        cwd[0] = '\0';
        GetCurrentDirectoryA(MAX_PATH, cwd);
        sprintf_s(buf, "[ModelRenderer] Failed to open OBJ: %s (cwd=%s)\n", objPath ? objPath : "(null)", cwd);
        OutputDebugStringA(buf);
        return;
    }

    // 要素数カウント
    while (true)
    {
        fscanf(file, "%s", str);

        if (feof(file) != 0)
            break;

        if (strcmp(str, "v") == 0)
        {
            positionNum++;
        }
        else if (strcmp(str, "vn") == 0)
        {
            normalNum++;
        }
        else if (strcmp(str, "vt") == 0)
        {
            texcoordNum++;
        }
        else if (strcmp(str, "usemtl") == 0)
        {
            subsetNum++;
        }
        else if (strcmp(str, "f") == 0)
        {
            // Read rest of face line, count corner tokens
            char lineBuf[1024];
            if (fgets(lineBuf, sizeof(lineBuf), file))
            {
                int corners = 0;
                char* ctx = nullptr;
                char* tok = strtok_s(lineBuf, " \t\r\n", &ctx);
                while (tok && tok[0] != '\0')
                {
                    corners++;
                    vertexNum++;
                    tok = strtok_s(nullptr, " \t\r\n", &ctx);
                }
                if (corners >= 3)
                {
                    // fan-triangulate: (corners-2) triangles
                    indexNum += (corners - 2) * 3;
                    in += (corners - 2) * 3;
                }
            }
        }
    }

    // Some OBJ files have no "usemtl". Ensure at least one subset so we can draw.
    if (subsetNum == 0)
    {
        subsetNum = 1;
    }

    // メモリ確保
    positionArray = new XMFLOAT3[positionNum];
    normalArray = new XMFLOAT3[normalNum];
    texcoordArray = new XMFLOAT2[texcoordNum];

    ModelObj->VertexArray = new VERTEX_3D[vertexNum];
    ModelObj->VertexNum = vertexNum;

    ModelObj->IndexArray = new unsigned int[indexNum];
    ModelObj->IndexNum = indexNum;

    ModelObj->SubsetArray = new SUBSET[subsetNum];
    ModelObj->SubsetNum = subsetNum;

    // ファイルポインタを先頭に戻す

















    XMFLOAT3* position = positionArray;
    XMFLOAT3* normal = normalArray;
    XMFLOAT2* texcoord = texcoordArray;

    unsigned int vc = 0;
    unsigned int ic = 0;
    unsigned int sc = 0;
    bool hasActiveSubset = false;

    fseek(file, 0, SEEK_SET);

    while (true)
    {
        fscanf(file, "%s", str);

        if (feof(file) != 0)
            break;

        if (strcmp(str, "mtllib") == 0)
        {
            //マテリアルファイル
            fscanf(file, "%s", str);

            char path[MAX_PATH];
            if (PathCombineA(path, dir, str) == nullptr)
            {
                // Path too long or invalid; skip loading materials safely
                continue;
            }

            LoadMaterial(path, &materialArray, &materialNum);
            if (materialNum == 0)
            {
                char fallbackMtl[MAX_PATH];
                if (PathCombineA(fallbackMtl, dir, "player.mtl") != nullptr)
                {
                    LoadMaterial(fallbackMtl, &materialArray, &materialNum);
                }
            }
        }
        else if (strcmp(str, "o") == 0)
        {
            //オブジェクト名
            fscanf(file, "%s", str);
        }
        else if (strcmp(str, "v") == 0)
        {
            //頂点座標
            fscanf(file, "%f", &position->x);
            fscanf(file, "%f", &position->y);
            fscanf(file, "%f", &position->z);
            position++;
        }
        else if (strcmp(str, "vn") == 0)
        {
            //法線
            fscanf(file, "%f", &normal->x);
            fscanf(file, "%f", &normal->y);
            fscanf(file, "%f", &normal->z);
            normal++;
        }
        else if (strcmp(str, "vt") == 0)
        {
            //テクスチャ座標
            fscanf(file, "%f", &texcoord->x);
            fscanf(file, "%f", &texcoord->y);
            // OBJ (bottom-left origin) -> DirectX texture space (top-left V)
            texcoord->y = 1.0f - texcoord->y;
            texcoord++;
        }
        else if (strcmp(str, "usemtl") == 0)
        {
            //マテリアル
            fscanf(file, "%s", str);

            // Safety: subsetNum may be wrong for malformed OBJ; avoid overflow/null access
            if (sc >= ModelObj->SubsetNum || ModelObj->SubsetArray == nullptr)
            {
                // Skip defining subset to avoid crashing
                continue;
            }

            if (sc != 0)
            {
                ModelObj->SubsetArray[sc - 1].IndexNum = ic - ModelObj->SubsetArray[sc - 1].StartIndex;
            }

            ModelObj->SubsetArray[sc].StartIndex = ic;
            hasActiveSubset = true;

            ModelObj->SubsetArray[sc].Material.Material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
            ModelObj->SubsetArray[sc].Material.Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            ModelObj->SubsetArray[sc].Material.Material.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            ModelObj->SubsetArray[sc].Material.Material.Emission = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            ModelObj->SubsetArray[sc].Material.Material.Shininess = 0.0f;
            ModelObj->SubsetArray[sc].Material.Material.TextureEnable = FALSE;
            strcpy(ModelObj->SubsetArray[sc].Material.TextureName, "");
            strcpy(ModelObj->SubsetArray[sc].Material.Name, str);

            for (unsigned int i = 0; i < materialNum; i++)
            {
                if (strcmp(str, materialArray[i].Name) == 0)
                {
                    ModelObj->SubsetArray[sc].Material.Material = materialArray[i].Material;
                    strcpy(ModelObj->SubsetArray[sc].Material.TextureName, materialArray[i].TextureName);
                    strcpy(ModelObj->SubsetArray[sc].Material.Name, materialArray[i].Name);

                    break;
                }
            }

            sc++;
        }
        else if (strcmp(str, "f") == 0)
        {
            //面 — read whole line then fan-triangulate (supports tri, quad, n-gon)

            if (!hasActiveSubset && ModelObj->SubsetArray != nullptr && ModelObj->SubsetNum > 0)
            {
                ModelObj->SubsetArray[0].StartIndex = 0;
                ModelObj->SubsetArray[0].IndexNum = 0;
                ModelObj->SubsetArray[0].Material.Material.Diffuse = { 1.0f,1.0f,1.0f,1.0f };
                ModelObj->SubsetArray[0].Material.Material.Ambient = { 0.2f,0.2f,0.2f,1.0f };
                ModelObj->SubsetArray[0].Material.Material.Emission = { 0.0f,0.0f,0.0f,0.0f };
                ModelObj->SubsetArray[0].Material.Material.TextureEnable = FALSE;
                strcpy(ModelObj->SubsetArray[0].Material.TextureName, "");
                strcpy(ModelObj->SubsetArray[0].Material.Name, "default");
                hasActiveSubset = true;
                sc = 1;
            }

            char lineBuf[1024];
            if (!fgets(lineBuf, sizeof(lineBuf), file))
            {
                continue;
            }

            // collect corner specs from the line
            static const int MAX_CORNERS = 32;
            int cornerV[MAX_CORNERS], cornerVt[MAX_CORNERS], cornerVn[MAX_CORNERS];
            int corners = 0;

            char* ctx2 = nullptr;
            char* tok = strtok_s(lineBuf, " \t\r\n", &ctx2);
            while (tok && tok[0] != '\0' && corners < MAX_CORNERS)
            {
                int vIdx = -1, vtIdx = -1, vnIdx = -1;
                const char* p = tok;
                char* endp = nullptr;
                vIdx = (int)strtol(p, &endp, 10);
                p = endp;
                if (p && *p == '/')
                {
                    p++;
                    if (*p != '/')
                    {
                        vtIdx = (int)strtol(p, &endp, 10);
                        p = endp;
                    }
                    if (p && *p == '/')
                    {
                        p++;
                        vnIdx = (int)strtol(p, nullptr, 10);
                    }
                }
                cornerV[corners] = vIdx;
                cornerVt[corners] = vtIdx;
                cornerVn[corners] = vnIdx;
                corners++;
                tok = strtok_s(nullptr, " \t\r\n", &ctx2);
            }

            if (corners < 3) continue;

            // Write all corner vertices first
            unsigned int baseVc = vc;
            for (int ci = 0; ci < corners; ci++)
            {
                int vIdx = cornerV[ci];
                int vtIdx = cornerVt[ci];
                int vnIdx = cornerVn[ci];

                if (vIdx <= 0 || (unsigned int)vIdx > positionNum || vc >= vertexNum)
                {
                    vc++;
                    continue;
                }
                ModelObj->VertexArray[vc].Position = positionArray[vIdx - 1];
                if (vtIdx > 0 && (unsigned int)vtIdx <= texcoordNum)
                    ModelObj->VertexArray[vc].TexCoord = texcoordArray[vtIdx - 1];
                else
                    ModelObj->VertexArray[vc].TexCoord = XMFLOAT2(0.0f, 0.0f);
                if (vnIdx > 0 && (unsigned int)vnIdx <= normalNum)
                    ModelObj->VertexArray[vc].Normal = normalArray[vnIdx - 1];
                else
                    ModelObj->VertexArray[vc].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                ModelObj->VertexArray[vc].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                vc++;
            }

            // Fan triangulation: (0,1,2), (0,2,3), (0,3,4) ...
            for (int ci = 1; ci < corners - 1; ci++)
            {
                if (ic + 2 >= indexNum) break;
                ModelObj->IndexArray[ic++] = baseVc;
                ModelObj->IndexArray[ic++] = baseVc + ci;
                ModelObj->IndexArray[ic++] = baseVc + ci + 1;
            }
        }
    }


    // finalize last subset
    if (ModelObj->SubsetArray != nullptr && ModelObj->SubsetNum > 0)
    {
        if (sc != 0)
        {
            ModelObj->SubsetArray[sc - 1].IndexNum = ic - ModelObj->SubsetArray[sc - 1].StartIndex;
        }
        else
        {
            // no usemtl and no faces (or malformed), keep a safe default
            ModelObj->SubsetArray[0].StartIndex = 0;
            ModelObj->SubsetArray[0].IndexNum = ic;
        }
    }

        fclose(file);

        delete[] positionArray;
        delete[] normalArray;
        delete[] texcoordArray;
        delete[] materialArray;
    
}
//マテリアル読み込み
void ModelRenderer::LoadMaterial(const char* FileName, MODEL_MATERIAL * *MaterialArray, unsigned int* MaterialNum)
{
    if (MaterialArray) *MaterialArray = nullptr;
    if (MaterialNum) *MaterialNum = 0;

    char resolvedMtlPath[MAX_PATH];
    const char* mtlPath = FileName;
    if (ResolveFilePath(FileName, resolvedMtlPath, _countof(resolvedMtlPath)))
    {
        mtlPath = resolvedMtlPath;
    }

    char dir[MAX_PATH];
    strcpy(dir, mtlPath);
    PathRemoveFileSpec(dir);






    char str[256];

    FILE* file;
    file = nullptr;
    if (fopen_s(&file, mtlPath, "rt") != 0 || file == nullptr)
    {
        char buf[1024];
        char cwd[MAX_PATH];
        cwd[0] = '\0';
        GetCurrentDirectoryA(MAX_PATH, cwd);
        sprintf_s(buf, "[ModelRenderer] Failed to open MTL: %s (cwd=%s)\n", mtlPath ? mtlPath : "(null)", cwd);
        OutputDebugStringA(buf);
        return;
    }

    MODEL_MATERIAL* materialArray;
    unsigned int materialNum = 0;

    //要素数カウント
    while (true)
    {
        fscanf(file, "%s", str);

        if (feof(file) != 0)
            break;

        if (strcmp(str, "newmtl") == 0)
        {
            materialNum++;
        }
    }

    //メモリ確保
    materialArray = new MODEL_MATERIAL[materialNum];

    //要素読込
    int mc = -1;

    fseek(file, 0, SEEK_SET);

    while (true)
    {
        fscanf(file, "%s", str);

        if (feof(file) != 0)
            break;

        if (strcmp(str, "newmtl") == 0)
        {
            //マテリアル名
            mc++;
            fscanf(file, "%s", materialArray[mc].Name);
            strcpy(materialArray[mc].TextureName, "");
            materialArray[mc].Material.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
            materialArray[mc].Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            materialArray[mc].Material.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            materialArray[mc].Material.Emission = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            materialArray[mc].Material.Shininess = 0.0f;
            materialArray[mc].Material.TextureEnable = FALSE;
            materialArray[mc].Texture = nullptr;
        }
        else if (strcmp(str, "Ka") == 0)
        {
            //アンビエント
            fscanf(file, "%f", &materialArray[mc].Material.Ambient.x);
            fscanf(file, "%f", &materialArray[mc].Material.Ambient.y);
            fscanf(file, "%f", &materialArray[mc].Material.Ambient.z);
            materialArray[mc].Material.Ambient.w = 1.0f;
        }
        else if (strcmp(str, "Kd") == 0)
        {
            //ディフューズ
            fscanf(file, "%f", &materialArray[mc].Material.Diffuse.x);
            fscanf(file, "%f", &materialArray[mc].Material.Diffuse.y);
            fscanf(file, "%f", &materialArray[mc].Material.Diffuse.z);
            materialArray[mc].Material.Diffuse.w = 1.0f;
        }
        else if (strcmp(str, "Ks") == 0)
        {
            //スペキュラ
            fscanf(file, "%f", &materialArray[mc].Material.Specular.x);
            fscanf(file, "%f", &materialArray[mc].Material.Specular.y);
            fscanf(file, "%f", &materialArray[mc].Material.Specular.z);
            materialArray[mc].Material.Specular.w = 1.0f;
        }
        else if (strcmp(str, "Ns") == 0)
        {
            fscanf(file, "%f", &materialArray[mc].Material.Shininess);
        }
        else if (strcmp(str, "d") == 0)
        {
            fscanf(file, "%f", &materialArray[mc].Material.Diffuse.w);
        }
        else if (strcmp(str, "Tr") == 0)
        {
            // MTL Tr is inverse alpha
            fscanf(file, "%f", &materialArray[mc].Material.Diffuse.w);
            materialArray[mc].Material.Diffuse.w = 1.0f - materialArray[mc].Material.Diffuse.w;
        }
        else if (strcmp(str, "map_Kd") == 0)
        {
            char texFile[MAX_PATH];
            fscanf(file, "%s", texFile);
            if (PathIsRelativeA(texFile))
            {
                char fullPath[MAX_PATH];
                if (PathCombineA(fullPath, dir, texFile) != nullptr)
                {
                    strcpy(materialArray[mc].TextureName, fullPath);
                }
                else
                {
                    strcpy(materialArray[mc].TextureName, texFile);
                }
            }
            else
            {
                strcpy(materialArray[mc].TextureName, texFile);
            }
        }
    }

    fclose(file);
    if (MaterialArray) *MaterialArray = materialArray;
    if (MaterialNum) *MaterialNum = materialNum;
}
      