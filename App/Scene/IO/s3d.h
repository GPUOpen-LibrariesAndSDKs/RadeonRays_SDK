/************************************************************
* LM_LIGHTMAP_COMMON.H
*
* Lightmap scene data transferring
************************************************************/

#ifndef _LM_LIGHTMAP_COMMON_H_
#define _LM_LIGHTMAP_COMMON_H_

#pragma once

#include <iostream>
#include <vector>
#include <fstream>

#define S3D_VERSION 1

enum lmS3D_LIGHT_TYPE {
   lmS3D_LIGHT_POINT,
   lmS3D_LIGHT_SPOT,
   lmS3D_LIGHT_DIR
};

enum lmS3D_OBJ_BAKE_TYPE {
   lmS3D_OBJ_BAKE_VC,
   lmS3D_OBJ_BAKE_TEX,
   lmS3D_OBJ_BAKE_NONE,
};

struct S3D_FLOAT3 { 
   float x, y, z;

   S3D_FLOAT3 (void) {}
#ifdef __S3D__
   S3D_FLOAT3(const m3dV & v) : x(v.x), y(v.y), z(v.z) {}
   S3D_FLOAT3(const m3dVTX & v) : x(v.s), y(v.t) {}
   S3D_FLOAT3(const m3dCOLOR & v) : x(v.r), y(v.g), z(v.b) {}
#endif // __S3D__
};

struct lmS3D_LIGHT {
      lmS3D_LIGHT_TYPE type;
      S3D_FLOAT3 pos;
      S3D_FLOAT3 dir;
      S3D_FLOAT3 color;
      float hotSpotAngleFactor;
      float fallOffAngle;
      float radius;
      float param;
};

struct lmS3D_OBJ_SPLIT {
   std::vector<int> dataTriangle;
};

struct lmS3D_OBJ {
      std::vector<lmS3D_OBJ_SPLIT> splits;
      std::vector<S3D_FLOAT3> points;
      std::vector<S3D_FLOAT3> normals;
      std::vector<S3D_FLOAT3> tangents;
      std::vector<S3D_FLOAT3> bitangents;
      std::vector<S3D_FLOAT3> lmUV;
      std::vector<S3D_FLOAT3> diffUV;
      lmS3D_OBJ_BAKE_TYPE bakeType;
};

struct lmS3D_SCENE_DATA {
      std::vector<lmS3D_OBJ> objects;
      std::vector<lmS3D_LIGHT> lights;
};

template <class T>
void lmSave(const T & data, std::ofstream & fs);
template <class T>
void lmLoad(T & data, std::ifstream & fs);

#define LM_LOAD_SAVE_PRIM(TYPE)                \
void lmSave (const TYPE & data, std::ofstream & fs) \
{                                              \
    fs.write((char *)&data, sizeof(TYPE));     \
}                                              \
void lmLoad (TYPE & data, std::ifstream & fs)  \
{                                              \
    fs.read((char *)&data, sizeof(TYPE));      \
}

LM_LOAD_SAVE_PRIM(float)
LM_LOAD_SAVE_PRIM(int)
LM_LOAD_SAVE_PRIM(S3D_FLOAT3)
LM_LOAD_SAVE_PRIM(lmS3D_LIGHT)
LM_LOAD_SAVE_PRIM(lmS3D_OBJ_BAKE_TYPE)

template<class T> void lmSave(const std::vector<T> & data, std::ofstream & fs)
{
   int size = data.size();
   fs.write((char *)&size, sizeof(int));
   for (const T & elem : data) {
      lmSave(elem, fs);
   }
}

template<class T> void lmLoad(std::vector<T> & data, std::ifstream & fs)
{
   int size;
   fs.read((char *)&size, sizeof(int));
   data.resize(size);
   for (T & elem : data) {
      lmLoad(elem, fs);
   }
}

void lmSave(const lmS3D_OBJ_SPLIT & data, std::ofstream & fs)
{
   lmSave(data.dataTriangle, fs);
}

void lmLoad(lmS3D_OBJ_SPLIT & data, std::ifstream & fs)
{
   lmLoad(data.dataTriangle, fs);
}

void lmSave(const lmS3D_OBJ & data, std::ofstream & fs)
{
   lmSave(data.splits, fs);
   lmSave(data.points, fs);
   lmSave(data.normals, fs);
   lmSave(data.tangents, fs);
   lmSave(data.bitangents, fs);
   lmSave(data.lmUV, fs);
   lmSave(data.diffUV, fs);
   lmSave(data.bakeType, fs);
}

void lmLoad(lmS3D_OBJ & data, std::ifstream & fs)
{
   lmLoad(data.splits, fs);
   lmLoad(data.points, fs);
   lmLoad(data.normals, fs);
   lmLoad(data.tangents, fs);
   lmLoad(data.bitangents, fs);
   lmLoad(data.lmUV, fs);
   lmLoad(data.diffUV, fs);
   lmLoad(data.bakeType, fs);
}

void lmRadeonRaysSaberSave(const lmS3D_SCENE_DATA & data, char * fileName)
{
   std::ofstream fs;

   fs.open(fileName, std::fstream::binary | std::fstream::out);
   int version = S3D_VERSION;
   fs.write((char *)&version, sizeof(int));
   lmSave(data.lights, fs);
   lmSave(data.objects, fs);

   fs.close();
}
bool lmRadeonRaysSaberLoad(lmS3D_SCENE_DATA & data, const char * fileName)
{
   std::ifstream fs;

   fs.open(fileName, std::fstream::binary | std::fstream::in);
   int version;
   fs.read((char *)&version, sizeof(int));
   if (version == S3D_VERSION) {
      lmLoad(data.lights, fs);
      lmLoad(data.objects, fs);
   }
   fs.close();
   return version == S3D_VERSION;
}

#endif // _LM_LIGHTMAP_COMMON_H_

//
// End-of-file _LM_LIGHTMAP_COMMON.H
//
