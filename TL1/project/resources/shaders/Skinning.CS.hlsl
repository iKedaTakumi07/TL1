struct Vertex
{
    float32_t4 position;
    float32_t2 texcoord;
    float32_t3 normal;
};
struct VertexInfluence
{
    float32_t4 weight;
    int32_t4 index;
};
struct SkinningIngormaion
{
    uint32_t numVertices;
};
struct Well
{
    float32_t4x4 skeletonSpaceMatrix;
    float32_t4x4 skeletonSpaceInverseTransPoseMatrix;
};

StructuredBuffer<Well> gMatrixPalette : register(t0); // skinningObject3d.VS.hlslで作ったものと同じPalette
StructuredBuffer<Vertex> gInputVertices : register(t1); // VertexBufferViewのstream0として利用していた入力頂点
StructuredBuffer<VertexInfluence> gInfluences : register(t2); // VertexBufferViewのstream1として入力インフルエンス
RWStructuredBuffer<Vertex> gOutputVertices : register(u0); // skinning計算後の頂点データ。skinnedVertex
ConstantBuffer<SkinningIngormaion> gSkinningIngormaion : register(b0); // skinningに関するちょっと舌情報

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint32_t vertexIndex = DTid.x;
    if (vertexIndex < gSkinningIngormaion.numVertices)
    {
        // skinning処理
        Vertex input = gInputVertices[vertexIndex];
        VertexInfluence influence = gInfluences[vertexIndex];
        
        // skinning五の頂点を計算
        Vertex skinned;
        skinned.texcoord = input.texcoord;
        
        // 計算方法はSkinningObject3d.VSと同じ
         
    // 位置の変換
        skinned.position = mul(input.position, gMatrixPalette[influence.index.x].skeletonSpaceMatrix) * influence.weight.x;
        skinned.position += mul(input.position, gMatrixPalette[influence.index.y].skeletonSpaceMatrix) * influence.weight.y;
        skinned.position += mul(input.position, gMatrixPalette[influence.index.z].skeletonSpaceMatrix) * influence.weight.z;
        skinned.position += mul(input.position, gMatrixPalette[influence.index.w].skeletonSpaceMatrix) * influence.weight.w;
        skinned.position.w = 1.0f;
    
    // 法線の変換
        skinned.normal = mul(input.normal, (float32_t3x3) gMatrixPalette[influence.index.x].skeletonSpaceInverseTransPoseMatrix) * influence.weight.x;
        skinned.normal += mul(input.normal, (float32_t3x3) gMatrixPalette[influence.index.y].skeletonSpaceInverseTransPoseMatrix) * influence.weight.y;
        skinned.normal += mul(input.normal, (float32_t3x3) gMatrixPalette[influence.index.z].skeletonSpaceInverseTransPoseMatrix) * influence.weight.z;
        skinned.normal += mul(input.normal, (float32_t3x3) gMatrixPalette[influence.index.w].skeletonSpaceInverseTransPoseMatrix) * influence.weight.w;
        skinned.normal = normalize(skinned.normal); // 正規化
        
        // Skinning五の頂点データを格納、書き込み!!
        gOutputVertices[vertexIndex] = skinned;
    }
}