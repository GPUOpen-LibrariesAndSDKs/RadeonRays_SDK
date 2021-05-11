#define TRIANGLES_PER_THREAD 16
#define GROUP_SIZE 256
#define INVALID_IDX 0xffffffff

struct CompressedBlock
{
    float3 v0;
    float3 v1;
    float3 v2;
    float3 v3;
    float3 v4;
    uint   triangle_id;
};

struct Constants
{
    uint vertex_stride;
    uint triangle_count;
};

ConstantBuffer<Constants>           g_constants : register(b0);
RWStructuredBuffer<float>           g_vertices : register(u0);
RWStructuredBuffer<uint>            g_indices : register(u1);
RWStructuredBuffer<uint>            g_blocks_count : register(u2);
RWStructuredBuffer<CompressedBlock> g_compressed_blocks : register(u3);
RWStructuredBuffer<uint>            g_triangle_pointers : register(u4);

groupshared uint lds_blocks_counter;
groupshared uint lds_group_write_offset;
// For scan and compact.
groupshared uint lds_keys[GROUP_SIZE];

//=====================================================================================================================
// Structures
struct FaceData
{
    uint prim_id;  // PrimitiveID of the triangle in the Ngon
    uint v_off;    // Used to specify the vertex rotation of the triangle (used for barycentrics)
};

struct Ngon
{
    uint     inds[5];
    FaceData f[4];
};

struct Triangle
{
    float3 v0;
    float3 v1;
    float3 v2;
};

struct Aabb
{
    float3 pmin;
    uint   padding0;
    float3 pmax;
    uint   padding1;
};

//=====================================================================================================================
// Move to parallel prims
uint BlockScan(uint key, uint lidx)
{
#ifndef USE_WAVE_INTRINSICS
    // Load the key into LDS
    lds_keys[lidx] = key;

    GroupMemoryBarrierWithGroupSync();

    // Calculate reduction
    int stride = 0;
    for (stride = 1; stride < GROUP_SIZE; stride <<= 1)
    {
        if (lidx < GROUP_SIZE / (2 * stride))
        {
            lds_keys[2 * (lidx + 1) * stride - 1] =
                lds_keys[2 * (lidx + 1) * stride - 1] + lds_keys[(2 * lidx + 1) * stride - 1];
        }

        GroupMemoryBarrierWithGroupSync();
    }

    // Then put 0 into the root for downsweep
    if (lidx == 0)
        lds_keys[GROUP_SIZE - 1] = 0;

    GroupMemoryBarrierWithGroupSync();

    // Perform downsweep
    for (stride = GROUP_SIZE >> 1; stride > 0; stride >>= 1)
    {
        if (lidx < GROUP_SIZE / (2 * stride))
        {
            int temp                              = lds_keys[(2 * lidx + 1) * stride - 1];
            lds_keys[(2 * lidx + 1) * stride - 1] = lds_keys[2 * (lidx + 1) * stride - 1];
            lds_keys[2 * (lidx + 1) * stride - 1] = lds_keys[2 * (lidx + 1) * stride - 1] + temp;
        }

        GroupMemoryBarrierWithGroupSync();
    }

    return lds_keys[lidx];
#else
    lds_keys[lidx] = 0;

    GroupMemoryBarrierWithGroupSync();

    // Perform scan within a subgroup
    int wave_scanned = WavePrefixSum(key);

    uint widx  = lidx / WaveGetLaneCount();
    uint wlidx = WaveGetLaneIndex();

    // Last element in each subgroup writes partial sum into LDS
    if (wlidx == WaveGetLaneCount() - 1)
    {
        lds_keys[widx] = wave_scanned + key;
    }

    GroupMemoryBarrierWithGroupSync();

    // Then first subgroup scannes partial sums
    if (widx == 0)
    {
        lds_keys[lidx] = WavePrefixSum(lds_keys[lidx]);
    }

    GroupMemoryBarrierWithGroupSync();

    // And we add partial sums back to each subgroup-scanned element
    wave_scanned += lds_keys[widx];

    return wave_scanned;
#endif
}

uint GlobalCompactedOffset(uint lidx, uint local_count)
{
    GroupMemoryBarrierWithGroupSync();

    uint local_offset = BlockScan(local_count, lidx);

    if (lidx == GROUP_SIZE - 1)
    {
        uint global_offset;
        InterlockedAdd(g_blocks_count[0], local_offset + local_count, global_offset);
        lds_group_write_offset = global_offset;
    }

    GroupMemoryBarrierWithGroupSync();

    return lds_group_write_offset + local_offset;
}

//=====================================================================================================================
// Helper functions
uint3 GetFaceIndices(uint tri_idx)
{
    return uint3(g_indices[3 * tri_idx], g_indices[3 * tri_idx + 1], g_indices[3 * tri_idx + 2]);
}

float3 FetchVertex(uint idx)
{
    uint stride_in_floats = (g_constants.vertex_stride >> 2);
    uint index_in_floats  = idx * stride_in_floats;
    return float3(g_vertices[index_in_floats], g_vertices[index_in_floats + 1], g_vertices[index_in_floats + 2]);
}

Triangle FetchTriangle(uint3 idx)
{
    Triangle tri;
    tri.v0 = FetchVertex(idx.x);
    tri.v1 = FetchVertex(idx.y);
    tri.v2 = FetchVertex(idx.z);
    return tri;
}

bool CompareVertices(uint idx0, uint idx1)
{
    // Vertices match if they have the same vertex index.
    if (idx0 == idx1)
    {
        return true;
    }

    if ((idx0 == INVALID_IDX) || (idx1 == INVALID_IDX))
    {
        return true;
    }

    return false;
}

//=====================================================================================================================
// Kernels
[numthreads(32, 1, 1)] void InitBlockCount(in uint gidx
                                           : SV_DispatchThreadID) {
    if (gidx == 0)
    {
        g_blocks_count[0] = 0;
    }
}

    [numthreads(GROUP_SIZE, 1, 1)] void CompressTriangles(in uint gidx
                                                          : SV_DispatchThreadID, in uint lidx
                                                          : SV_GroupThreadID, in uint bidx
                                                          : SV_GroupID)
{
    if (lidx == 0)
    {
        lds_blocks_counter = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint start_tri_idx = gidx * TRIANGLES_PER_THREAD;

    Ngon ngon_array[TRIANGLES_PER_THREAD];

    uint ngon_index = 0;

    // Loop over every triangle in the triangle group to compress into Ngons.
    for (uint tri_idx = start_tri_idx;
         (tri_idx < (start_tri_idx + TRIANGLES_PER_THREAD)) && (tri_idx < g_constants.triangle_count);
         tri_idx++)
    {
        // Fetch face indices from index buffer
        uint3 face_indices = GetFaceIndices(tri_idx);

        // Fetch triangle vertex data from vertex buffer
        Triangle tri = FetchTriangle(face_indices);

        // Write out the scratch node. The node pointer is not known yet; it will be updated after compression.
        uint node_ptr = INVALID_IDX;

        uint ind[3]             = {face_indices.x, face_indices.y, face_indices.z};
        uint best_ngon          = ngon_index;
        uint best_pattern       = 0;
        uint new_verts          = 4;
        uint best_input_pattern = 0;

// At a high level this triangle compression algorithm works by iterating through each triangle
// in the triangle group and triangles compressed so far and exhaustively searching for the best
// transformation of the triangle and triangle node that yields the best compression
// (lowest new vertices) to insert the triangle into.
// This algorithm is academically slow (O(n^2)) but it is heavily optimizable using bit manipulation
// tricks which can make it reasonably fast in practice.

// The pattern and input_pattern arrays are 2 and 3 dimensional arrays whose last level
// of components is packed into each hexadecimal digit of the uint number to avoid the memory
// and divergence cost of indexing these arrays fully unpacked.
// The CI macro is used to index into this last level array.
#define CI(swizzle, index) (((swizzle) >> ((index)*4u)) & 0xf)

        // So for example if we had an array that looked like {5,2,1,3,0};
        // We would pack this into a uint like this:
        // uint value = 0x03125;
        // Note: The order of the values is reversed because hex numbers are
        // entered in little endian instead of big endian like arrays are.

        // So, to index the 5th element in the array packed into 'value' we would do the following:
        // uint entry3 = CI(value,4)
        // And 'entry3' would be set to '0' afterwords.

        // The pattern lookup table is used to compute the mapping between vertices on the triangle
        // and vertices of the triangle node.
        // Each component of the array corresponds to one triangle on the triangle node.
        // The hex digits of each entry specify which vertex on the triangle node each vertex of
        // the triangle corresponds to.
        uint pattern[4] = {
            0x0210210,  // Triangle 0 Vertex Mapping
            0x0231231,  // Triangle 1 Vertex Mapping
            0x1243243,  // Triangle 2 Vertex Mapping
            0x1204204   // Triangle 3 Vertex Mapping
        };

        // This allows the mapping of a triangle to be converted to triangle node indices using the following function:
        // uint TriangleNodeIndex = CI(pattern[TriangleInTriangleNode]>>(NumTriangleRotations), VertexInTriangle);
        // So, for example if we wanted to compute the index in the triangle node for vertex 1 of the compressed
        // triangle after it the triangle was rotated 2 times and is going to be inserted into triangle slot 3 you would
        // do this: uint TriangleNodeIndex = CI((pattern[3]>>2), 1); The 7th number on this list is used to denote how
        // many additional rotations must be applied to the triangle so that the barycentrics are correct after
        // inserting into the triangle node.

        // The input_pattern lookup table is used to transform a triangle node into another equivalent triangle node.
        // It is broken into 3 sections:
        // VertexSwizzle: The mapping from the vertices in the old triangle node to the new triangle node.
        // ValidFaceMask: Specifies which faces in the original triangle node must be invalid for the transform to be
        // valid. face_swizzle: The mapping from the triangle slots in the old triangle node to the new triangle node.
        uint input_pattern[3][3] = {
            // VertexSwizzle, ValidFaceMask, face_swizzle
            {0x43210, 0x0000, 0x3210},  // Identity Mapping F0->F0, F1->F1, F2->F2, F3->F3
            {0x40123, 0x1100, 0x3201},  // Flip triangle 0 and 1 F0->F1, F1->F0, F2->(invalid), F3->(invalid)
            {0x13042, 0x0110, 0x0213},  // Flip triangle 0 and 3 F0->F3, F1->(invalid), F2->(invalid), F3->F0
        };

        // This loop is used to find the Ngon in the ngon_array that the current triangle compresses the best into.
        for (uint n = 0; n < ngon_index; ++n)
        {
            // For each of the Ngons, the primitive mask of valid primitives is calculated
            uint prim_mask = (ngon_array[n].f[0].prim_id != INVALID_IDX) ? 0x1 : 0;
            prim_mask |= (ngon_array[n].f[1].prim_id != INVALID_IDX) ? 0x10 : 0;
            prim_mask |= (ngon_array[n].f[2].prim_id != INVALID_IDX) ? 0x100 : 0;
            prim_mask |= (ngon_array[n].f[3].prim_id != INVALID_IDX) ? 0x1000 : 0;

            // This loop is used to find the best transformation of the triangle node
            // that yields the lowest number of newly allocated vertices.
            for (uint ip = 0; ip < 3; ++ip)
            {
                // Skip testing of Ngon transforms that would invalidate
                // some of the valid triangles in the triangle node.
                if ((prim_mask & input_pattern[ip][1]) != 0)
                {
                    continue;
                }

                // A triangle must share a vertex with the center vertex of a triangle node (vertex 2)
                // to fit inside of it. By computing the rotation where that happens ahead of
                // time we can save on the inner loop computation by only testing 4 permuations
                // of the triangle instead of 12 (the four triangle slots * 3 rotations)
                uint input_ind_swizzle = input_pattern[ip][0];
                uint ind2              = ngon_array[n].inds[CI(input_ind_swizzle, 2)];

                // Find out how much we have to rotate the triangle to line up with vertex 2 of the
                // the triangle node and save that in p_off. If we can't line up with vertex 2,
                // then we skip to the next triangle node transformation since the triangle can't
                // fit in the current one.
                int p_off = 0;

                if (ind2 == -1)
                {
                    p_off = 0;
                } else if (CompareVertices(ind2, ind[0]))
                {
                    p_off = 2;
                } else if (CompareVertices(ind2, ind[1]))
                {
                    p_off = 1;
                } else if (CompareVertices(ind2, ind[2]))
                {
                    p_off = 0;
                } else
                {
                    continue;
                }

                // Lookup the face_swizzle mapping for this transformation
                uint face_swizzle = input_pattern[ip][2];

                // This loop checks all the triangle slots in the triangle node for a good candidate.
                for (uint t = 0; t < 4; t += 1)
                {
                    // Check that there isn't already a triangle compressed in this slot
                    if ((CI(prim_mask, CI(face_swizzle, t))) == 0)
                    {
                        int  ind4[3];
                        uint pattern_row = pattern[t] >> (p_off * 4);

                        // Get the three indices where we are trying to insert the triangle into.
                        ind4[0] = ngon_array[n].inds[CI(input_ind_swizzle, CI(pattern_row, 0))];
                        ind4[1] = ngon_array[n].inds[CI(input_ind_swizzle, CI(pattern_row, 1))];
                        ind4[2] = ngon_array[n].inds[CI(input_ind_swizzle, CI(pattern_row, 2))];

                        // Count how many new vertices we are allocating by inserting into this node.
                        int newVertCnt = (ind4[0] == INVALID_IDX ? 1 : 0) + (ind4[1] == INVALID_IDX ? 1 : 0) +
                                         (ind4[2] == INVALID_IDX ? 1 : 0);

                        // If we have already found a better triangle node to compress into don't bother with
                        // checking if the triangle fits into the slot.
                        if (newVertCnt < new_verts)
                        {
                            bool fits = true;

                            // Check if each of the three vertices fit. If a vertex was already found to fit
                            // in the p_off test or if the vertex was not previously allocated it is not tested,
                            // and instead is treated as a guaranteed fit.
                            if ((p_off != 2) && (ind4[0] != INVALID_IDX))
                            {
                                fits &= CompareVertices(ind4[0], ind[0]);
                            }
                            if ((p_off != 1) && (ind4[1] != INVALID_IDX))
                            {
                                fits &= CompareVertices(ind4[1], ind[1]);
                            }
                            if ((p_off != 0) && (ind4[2] != INVALID_IDX))
                            {
                                fits &= CompareVertices(ind4[2], ind[2]);
                            }

                            if (fits)
                            {
                                // The triangle fits and this Ngon and transform is the best
                                // that has been found so far, so latch the transform and Ngon.
                                new_verts          = newVertCnt;
                                best_ngon          = n;
                                best_pattern       = t * 4 + p_off;
                                best_input_pattern = ip;
                            }
                        }
                    }
                }
            }
        }

        uint face        = best_pattern / 4;
        uint p_off       = best_pattern % 4;
        uint pattern_row = pattern[face];
        // Check if there was any good candidate Ngon found to compress the triangle into.
        if (ngon_index <= best_ngon)
        {
            // There wasn't a good Ngon, so allocate a new one and default initialize it.
            ngon_array[best_ngon].inds[0]      = INVALID_IDX;
            ngon_array[best_ngon].inds[1]      = INVALID_IDX;
            ngon_array[best_ngon].inds[2]      = INVALID_IDX;
            ngon_array[best_ngon].inds[3]      = INVALID_IDX;
            ngon_array[best_ngon].inds[4]      = INVALID_IDX;
            ngon_array[best_ngon].f[0].prim_id = INVALID_IDX;
            ngon_array[best_ngon].f[1].prim_id = INVALID_IDX;
            ngon_array[best_ngon].f[2].prim_id = INVALID_IDX;
            ngon_array[best_ngon].f[3].prim_id = INVALID_IDX;
            ngon_array[best_ngon].f[0].v_off   = 0;
            ngon_array[best_ngon].f[1].v_off   = 0;
            ngon_array[best_ngon].f[2].v_off   = 0;
            ngon_array[best_ngon].f[3].v_off   = 0;

            ngon_index++;
        } else
        {
            // Apply the needed transform on the existing Ngon that the triangle will be inserted in.
            uint index_swizzle = input_pattern[best_input_pattern][0];
            uint face_swizzle  = input_pattern[best_input_pattern][2];
            Ngon ng            = ngon_array[best_ngon];

            uint i = 0;
            for (i = 0; i < 5; ++i)
            {
                ngon_array[best_ngon].inds[CI(index_swizzle, i)] = ng.inds[i];
            }
            for (i = 0; i < 4; ++i)
            {
                ngon_array[best_ngon].f[(face_swizzle >> (i * 4)) & 0xf] = ng.f[i];
            }

            // The topology changes if input_pattern 1 ends up rotating the triangles, so we
            // must update v_off to compensate.
            if (best_input_pattern == 1)
            {
                ngon_array[best_ngon].f[0].v_off += 2;
                ngon_array[best_ngon].f[1].v_off += 1;
            }
        }

        // Insert the new triangle into the Ngon where it compresses the best.
        ngon_array[best_ngon].inds[CI(pattern_row, 0 + p_off)] = ind[0];
        ngon_array[best_ngon].inds[CI(pattern_row, 1 + p_off)] = ind[1];
        ngon_array[best_ngon].inds[CI(pattern_row, 2 + p_off)] = ind[2];
        ngon_array[best_ngon].f[face].prim_id                  = tri_idx;
        ngon_array[best_ngon].f[face].v_off                    = (CI(pattern_row, 6) + p_off) % 3;
    }

    uint thread_write_offset = GlobalCompactedOffset(lidx, ngon_index);

    uint block_byte_offset = thread_write_offset * sizeof(CompressedBlock);

    // This step iterates through all of the Ngons after compression and writes out the triangle nodes.
    for (int x = 0; x < ngon_index; ++x)
    {
        Ngon n = ngon_array[x];

        // If this geometry has a valid transformation matrix. Transform each vertex using this matrix.
        // float4x4 transform;
        // if (ShaderConstants.HasValidTransform)
        // {
        //     transform[0] = TransformBuffer[0];
        //     transform[1] = TransformBuffer[1];
        //     transform[2] = TransformBuffer[2];
        //     transform[3] = float4(0, 0, 0, 1);
        // }

        // Load, transform, and store all of the 5 vertices.
        CompressedBlock compressed_block;
        if (n.inds[0] != INVALID_IDX)
            compressed_block.v0 = FetchVertex(n.inds[0]);
        if (n.inds[1] != INVALID_IDX)
            compressed_block.v1 = FetchVertex(n.inds[1]);
        if (n.inds[2] != INVALID_IDX)
            compressed_block.v2 = FetchVertex(n.inds[2]);
        if (n.inds[3] != INVALID_IDX)
            compressed_block.v3 = FetchVertex(n.inds[3]);
        if (n.inds[4] != INVALID_IDX)
            compressed_block.v4 = FetchVertex(n.inds[4]);

        compressed_block.triangle_id = 0;
        for (uint i = 0; i < 4; ++i)
        {
            if (n.f[i].prim_id != INVALID_IDX)
            {
                compressed_block.triangle_id |= ((n.f[i].v_off + 1) % 3) << (i * 8);
                compressed_block.triangle_id |= ((n.f[i].v_off + 2) % 3) << (i * 8 + 2);

                const uint triangle_pointer         = i | (block_byte_offset >> 3);
                g_triangle_pointers[n.f[i].prim_id] = triangle_pointer;
            }
        }

        g_compressed_blocks[thread_write_offset] = compressed_block;
        block_byte_offset += sizeof(CompressedBlock);
    }
}