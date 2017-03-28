#include "RprLoadStore.h"

#include "Rpr/RadeonProRender.h"

#include <fstream>
#include <cstring>

#include "frs6.h"
#include "frs7.h"
#include "rps8.h"


std::map<std::string, int> g_extraCustomParam_int;
std::map<std::string, float> g_extraCustomParam_float;
std::vector<RPS_OBJECT_DECLARED> g_listLoadedObjects;



int rprsExportCustomList(
	char const * frsFileName, 
	int materialNode_number, rpr_material_node* materialNode_list,
	int camera_number, rpr_camera* camera_list,
	int light_number, rpr_light* light_list,
	int shape_number, rpr_shape* shape_list,
	int image_number, rpr_image* image_list
)
{
	rpr_int status = RPR_SUCCESS;

	std::fstream ofileStore(frsFileName, std::fstream::binary | std::fstream::out | std::fstream::trunc);
	if (ofileStore.is_open() && !ofileStore.fail() ) 
	{
		RPS8 frsFile(&ofileStore, true);

		std::vector<rpr_material_node> materialNodeList;
		std::vector<rpr_camera> cameraList;
		std::vector<rpr_light> lightList;
		std::vector<rpr_shape> shapeList;
		std::vector<rpr_image> imageList;
		
		for(int i=0; i<materialNode_number; i++)
			materialNodeList.push_back(materialNode_list[i]);

		for(int i=0; i<camera_number; i++)
			cameraList.push_back(camera_list[i]);

		for(int i=0; i<light_number; i++)
			lightList.push_back(light_list[i]);

		for(int i=0; i<shape_number; i++)
			shapeList.push_back(shape_list[i]);

		for(int i=0; i<image_number; i++)
			imageList.push_back(image_list[i]);

		status = frsFile.StoreCustomList(
			materialNodeList,
			cameraList,
			lightList,
			shapeList,
			imageList);
		
		ofileStore.close();

	}
	else
	{
		status = RPR_ERROR_INTERNAL_ERROR;
	}

	return status;
}



int rprsImportCustomList(
	char const * rprsFileName, 
	rpr_context context, 
	rpr_material_system materialSystem ,
	int* materialNode_number, rpr_material_node* materialNode_list,
	int* camera_number, rpr_camera* camera_list,
	int* light_number, rpr_light* light_list,
	int* shape_number, rpr_shape* shape_list,
	int* image_number, rpr_image* image_list)
{
	rpr_int status = RPR_SUCCESS;

	std::fstream ifileStore(rprsFileName, std::ifstream::binary | std::ifstream::in );
	if (ifileStore.is_open() && !ifileStore.fail() ) 
	{
		//first, we check the version of the opened FRS, so we can choose the good importer.
		
		int32_t checkCode = 0;
		ifileStore.read((char*)&checkCode, sizeof(checkCode));
		int32_t version = 0;
		ifileStore.read((char*)&version, sizeof(version));
		ifileStore.seekg(0, ifileStore.beg); // go to start of file

		if ( version != 8  )
		{
			// only version 8 supports  rprsImportCustomList
			status = RPR_ERROR_INTERNAL_ERROR;
		}
		else // latest version : 8
		{
			std::vector<rpr_image> imageList;
			std::vector<rpr_shape> shapeList;
			std::vector<rpr_light> lightList;
			std::vector<rpr_camera> cameraList;
			std::vector<rpr_material_node> materialNodeList;

			RPS8 frsFile(&ifileStore, false);
			status = frsFile.LoadCustomList(
				context,
				materialSystem,
				imageList,
				shapeList,
				lightList,
				cameraList,
				materialNodeList
				);
			g_extraCustomParam_int.clear();
			g_extraCustomParam_float.clear();
			g_listLoadedObjects.clear();

			if ( status == RPR_SUCCESS )
			{
				g_extraCustomParam_int = frsFile.GetExtraCustomParam_int();
				g_extraCustomParam_float = frsFile.GetExtraCustomParam_float();
				g_listLoadedObjects = frsFile.GetListObjectDeclared();

				if ( image_number != NULL ) { *image_number = imageList.size(); }
				if ( image_list != NULL ) { for(int i=0; i<imageList.size(); i++) { image_list[i] = imageList[i]; } }
			}



		}


		ifileStore.close();
	}
	else
	{
		status = RPR_ERROR_INTERNAL_ERROR;
	}


	return status;
}





int rprsExport(
	const char* frsFileName, 
	rpr_context context, 
	rpr_scene scene,
	
	int extraCustomParam_int_number,
	const char** extraCustomParam_int_names,
	const int* extraCustomParam_int_values,

	int extraCustomParam_float_number,
	const char** extraCustomParam_float_names,
	const float* extraCustomParam_float_values
	)
{
	rpr_int status = RPR_SUCCESS;

	std::fstream ofileStore(frsFileName, std::fstream::binary | std::fstream::out | std::fstream::trunc);
	if (ofileStore.is_open() && !ofileStore.fail() ) 
	{
		RPS8 frsFile(&ofileStore, true);

		for(int i=0; i<extraCustomParam_float_number; i++)
		{
			frsFile.AddExtraCustomParam_float(extraCustomParam_float_names[i],extraCustomParam_float_values[i]);
		}
		for(int i=0; i<extraCustomParam_int_number; i++)
		{
			frsFile.AddExtraCustomParam_int(extraCustomParam_int_names[i],extraCustomParam_int_values[i]);
		}

		status = frsFile.StoreEverything(context,scene);
		ofileStore.close();
	}
	else
	{
		status = RPR_ERROR_INTERNAL_ERROR;
	}

	return status;
}

int rprsImport(	
	const char* frsFileName,
	rpr_context context,  
	rpr_material_system materialSystem, 
	rpr_scene* scene,  
	bool useAlreadyExistingScene)
{
	rpr_int status = RPR_SUCCESS;

	std::fstream ifileStore(frsFileName, std::ifstream::binary | std::ifstream::in );
	if (ifileStore.is_open() && !ifileStore.fail() ) 
	{
		//first, we check the version of the opened FRS, so we can choose the good importer.
		
		int32_t checkCode = 0;
		ifileStore.read((char*)&checkCode, sizeof(checkCode));
		int32_t version = 0;
		ifileStore.read((char*)&version, sizeof(version));
		ifileStore.seekg(0, ifileStore.beg); // go to start of file

		if ( version == 5 ||  version == 6 ) // legacy version 5 & 6
		{
			FRS6::LOADER_DATA_FROM_3DS maxData;
			status = FRS6::LoadEverything(context,materialSystem,&ifileStore,*scene,&maxData,useAlreadyExistingScene);
			g_extraCustomParam_int.clear();
			g_extraCustomParam_float.clear();
			g_listLoadedObjects.clear();
		}
		else if ( version == 7 )
		{
			FRS7 frsFile(&ifileStore, false);
			status = frsFile.LoadEverything(context,materialSystem,*scene,useAlreadyExistingScene);
			g_extraCustomParam_int.clear();
			g_extraCustomParam_float.clear();
			g_listLoadedObjects.clear();

			if ( status == RPR_SUCCESS ) // legacy version 7
			{
				g_extraCustomParam_int = frsFile.GetExtraCustomParam_int();
				g_extraCustomParam_float = frsFile.GetExtraCustomParam_float();
				g_listLoadedObjects = frsFile.GetListObjectDeclared();
			}
		}
		else if ( version == 8 ) // latest version : 8
		{
			RPS8 frsFile(&ifileStore, false);
			status = frsFile.LoadEverything(context,materialSystem,*scene,useAlreadyExistingScene);
			g_extraCustomParam_int.clear();
			g_extraCustomParam_float.clear();
			g_listLoadedObjects.clear();

			if ( status == RPR_SUCCESS )
			{
				g_extraCustomParam_int = frsFile.GetExtraCustomParam_int();
				g_extraCustomParam_float = frsFile.GetExtraCustomParam_float();
				g_listLoadedObjects = frsFile.GetListObjectDeclared();
			}
		}
		else
		{
			status = RPR_ERROR_INTERNAL_ERROR;
		}


		ifileStore.close();
	}
	else
	{
		status = RPR_ERROR_INTERNAL_ERROR;
	}


	return status;
}


int rprsGetExtraCustomParam_int(const char* name, int* value)
{
	auto it = g_extraCustomParam_int.find(name);
	if ( it != g_extraCustomParam_int.end() )
	{
		*value = (*it).second;
		return RPR_SUCCESS;
	}
	else
	{
		return RPR_ERROR_INVALID_PARAMETER;
	}
}

int rprsGetExtraCustomParam_float(const char* name, float* value)
{
	auto it = g_extraCustomParam_float.find(name);
	if ( it != g_extraCustomParam_float.end() )
	{
		*value = (*it).second;
		return RPR_SUCCESS;
	}
	else
	{
		return RPR_ERROR_INVALID_PARAMETER;
	}
}


int rprsGetExtraCustomParamIndex_int(int index, int* value)
{
	int nbOfInt = g_extraCustomParam_int.size();

	if ( index >= 0 && index < nbOfInt ) 
	{
		int i=0;
		for(auto it=g_extraCustomParam_int.begin(); it!=g_extraCustomParam_int.end(); it++)
		{
			if ( i == index )
			{
				*value = (*it).second;
				return RPR_SUCCESS;
			}
			i++;
		}
	}

	return RPR_ERROR_INVALID_PARAMETER;
}

int rprsGetExtraCustomParamIndex_float(int index, float* value)
{
	int nbOfInt = g_extraCustomParam_int.size();
	int nbOfFloat = g_extraCustomParam_float.size();

	if ( index >= nbOfInt+0 && index < nbOfInt+nbOfFloat ) 
	{
		int i=nbOfInt;
		for(auto it=g_extraCustomParam_float.begin(); it!=g_extraCustomParam_float.end(); it++)
		{
			if ( i == index )
			{
				*value = (*it).second;
				return RPR_SUCCESS;
			}
			i++;
		}
	}

	return RPR_ERROR_INVALID_PARAMETER;
}


int rprsGetNumberOfExtraCustomParam()
{
	return g_extraCustomParam_int.size() + g_extraCustomParam_float.size();
}


int rprsGetExtraCustomParamNameSize(int index, int* nameSizeGet)
{
	int nbOfInt = g_extraCustomParam_int.size();
	int nbOfFloat = g_extraCustomParam_float.size();

	if ( index >= 0 && index < nbOfInt ) 
	{
		int i=0;
		for(auto it=g_extraCustomParam_int.begin(); it!=g_extraCustomParam_int.end(); it++)
		{
			if ( i == index )
			{
				*nameSizeGet = (*it).first.size()+1;
				return RPR_SUCCESS;
			}
			i++;
		}
	}
	else if ( index >= nbOfInt+0 && index < nbOfInt+nbOfFloat ) 
	{
		int i=nbOfInt;
		for(auto it=g_extraCustomParam_float.begin(); it!=g_extraCustomParam_float.end(); it++)
		{
			if ( i == index )
			{
				*nameSizeGet = (*it).first.size()+1;
				return RPR_SUCCESS;
			}
			i++;
		}
	}

	return RPR_ERROR_INVALID_PARAMETER;
}


int rprsGetExtraCustomParamType(int index)
{

	int nbOfInt = g_extraCustomParam_int.size();
	int nbOfFloat = g_extraCustomParam_float.size();

	if ( index >= 0 && index < nbOfInt ) 
	{
		return RPRLOADSTORE_PARAMETER_TYPE_INT;
	}
	else if ( index >= nbOfInt+0 && index < nbOfInt+nbOfFloat ) 
	{
		return RPRLOADSTORE_PARAMETER_TYPE_FLOAT;
	}

	return RPRLOADSTORE_PARAMETER_TYPE_UNDEF;
}

int rprsGetExtraCustomParamName(int index, char* nameGet, int nameGetSize)
{
	int nbOfInt = g_extraCustomParam_int.size();
	int nbOfFloat = g_extraCustomParam_float.size();

	if ( index >= 0 && index < nbOfInt ) 
	{
		int i=0;
		for(auto it=g_extraCustomParam_int.begin(); it!=g_extraCustomParam_int.end(); it++)
		{
			if ( i == index && nameGetSize >= (*it).first.size()+1 )
			{
				std::strcpy(nameGet,(*it).first.c_str());
				return RPR_SUCCESS;
			}
			i++;
		}
	}
	else if ( index >= nbOfInt+0 && index < nbOfInt+nbOfFloat ) 
	{
		int i=nbOfInt;
		for(auto it=g_extraCustomParam_float.begin(); it!=g_extraCustomParam_float.end(); it++)
		{
			if ( i == index && nameGetSize >= (*it).first.size()+1 )
			{
				std::strcpy(nameGet,(*it).first.c_str());
				return RPR_SUCCESS;
			}
			i++;
		}
	}

	return RPR_ERROR_INVALID_PARAMETER;
}



int rprsListImportedObjects(void** objOut, int sizeLightBytes ,int* numberOfObjectsOut, const char* frName, const char* rprName)
{
	int nbOfObjects = 0;

	for(int iObj=0; iObj<g_listLoadedObjects.size(); iObj++)
	{
		if ( g_listLoadedObjects[iObj].type == std::string(frName)
			||
			g_listLoadedObjects[iObj].type == std::string(rprName)
			)
		{
			if ( objOut )
			{
				if ( sizeLightBytes >= (nbOfObjects+1)*sizeof(rpr_light*) )
				{
					objOut[nbOfObjects] = g_listLoadedObjects[iObj].obj;
				}
				else
				{
					return RPR_ERROR_INVALID_PARAMETER;
				}
			}

			nbOfObjects++;

		}

	}

	if ( numberOfObjectsOut )
	{
		*numberOfObjectsOut = nbOfObjects;
	}

	return RPR_SUCCESS;
}


int rprsListImportedCameras(void** Cameras, int sizeCameraBytes ,int* numberOfCameras)
{
	int ret = rprsListImportedObjects(Cameras,sizeCameraBytes,numberOfCameras,"fr_camera","rpr_camera");
	return ret;
}
int rprsListImportedMaterialNodes(void** MaterialNodes, int sizeMaterialNodeBytes ,int* numberOfMaterialNodes)
{
	int ret = rprsListImportedObjects(MaterialNodes,sizeMaterialNodeBytes,numberOfMaterialNodes,"fr_material_node","rpr_material_node");
	return ret;
}
int rprsListImportedLights(void** Lights, int sizeLightBytes ,int* numberOfLights)
{
	int ret = rprsListImportedObjects(Lights,sizeLightBytes,numberOfLights,"fr_light","rpr_light");
	return ret;
}
int rprsListImportedShapes(void** Shapes, int sizeShapeBytes ,int* numberOfShapes)
{
	int ret = rprsListImportedObjects(Shapes,sizeShapeBytes,numberOfShapes,"fr_shape","rpr_shape");
	return ret;
}
int rprsListImportedImages(void** Images, int sizeImageBytes ,int* numberOfImages)
{
	int ret = rprsListImportedObjects(Images,sizeImageBytes,numberOfImages,"fr_image","rpr_image");
	return ret;
}
