#include "RprLoadStore.h"

int frsExport(char const * rprsFileName, void * context, void * scene, int extraCustomParam_int_number, char const * * extraCustomParam_int_names, int const * extraCustomParam_int_values, int extraCustomParam_float_number, char const * * extraCustomParam_float_names, float const * extraCustomParam_float_values)
{
    return rprsExport(rprsFileName, context, scene, extraCustomParam_int_number, extraCustomParam_int_names, extraCustomParam_int_values, extraCustomParam_float_number, extraCustomParam_float_names, extraCustomParam_float_values);
}

int frsImport(char const * rprsFileName, void * context, void * materialSystem, void * * scene, bool useAlreadyExistingScene)
{
    return rprsImport(rprsFileName, context, materialSystem, scene, useAlreadyExistingScene);
}

int frsGetExtraCustomParam_int(char const * name, int * value)
{
    return rprsGetExtraCustomParam_int(name, value);
}

int frsGetExtraCustomParam_float(char const * name, float * value)
{
    return rprsGetExtraCustomParam_float(name, value);
}

int frsGetExtraCustomParamIndex_int(int index, int * value)
{
    return rprsGetExtraCustomParamIndex_int(index, value);
}

int frsGetExtraCustomParamIndex_float(int index, float * value)
{
    return rprsGetExtraCustomParamIndex_float(index, value);
}

int frsGetNumberOfExtraCustomParam()
{
    return rprsGetNumberOfExtraCustomParam();
}

int frsGetExtraCustomParamNameSize(int index, int * nameSizeGet)
{
    return rprsGetExtraCustomParamNameSize(index, nameSizeGet);
}

int frsGetExtraCustomParamName(int index, char * nameGet, int nameGetSize)
{
    return rprsGetExtraCustomParamName(index, nameGet, nameGetSize);
}

int frsGetExtraCustomParamType(int index)
{
    return rprsGetExtraCustomParamType(index);
}

int frsListImportedCameras(void * * Cameras, int sizeCameraBytes, int * numberOfCameras)
{
    return rprsListImportedCameras(Cameras, sizeCameraBytes, numberOfCameras);
}

int frsListImportedMaterialNodes(void * * MaterialNodes, int sizeMaterialNodeBytes, int * numberOfMaterialNodes)
{
    return rprsListImportedMaterialNodes(MaterialNodes, sizeMaterialNodeBytes, numberOfMaterialNodes);
}

int frsListImportedLights(void * * Lights, int sizeLightBytes, int * numberOfLights)
{
    return rprsListImportedLights(Lights, sizeLightBytes, numberOfLights);
}

int frsListImportedShapes(void * * Shapes, int sizeShapeBytes, int * numberOfShapes)
{
    return rprsListImportedShapes(Shapes, sizeShapeBytes, numberOfShapes);
}

int frsListImportedImages(void * * Images, int sizeImageBytes, int * numberOfImages)
{
    return rprsListImportedImages(Images, sizeImageBytes, numberOfImages);
}

int frsExportCustomList(char const * rprsFileName, int materialNode_number, void** materialNode_list, int camera_number, void** camera_list, int light_number, void** light_list, int shape_number, void** shape_list, int image_number, void** image_list)
{
    return rprsExportCustomList(rprsFileName, materialNode_number, materialNode_list, camera_number, camera_list, light_number, light_list, shape_number, shape_list, image_number, image_list);
}

int frsImportCustomList(char const * rprsFileName, void * context, void * materialSystem, int*  materialNode_number, void** materialNode_list, int*  camera_number, void** camera_list, int*  light_number, void** light_list, int*  shape_number, void** shape_list, int*  image_number, void** image_list)
{
    return rprsImportCustomList(rprsFileName, context, materialSystem, materialNode_number, materialNode_list, camera_number, camera_list, light_number, light_list, shape_number, shape_list, image_number, image_list);
}

