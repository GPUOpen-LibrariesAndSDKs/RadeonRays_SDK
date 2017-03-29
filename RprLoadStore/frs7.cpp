#include <vector>
#include <cstring>
#include <algorithm>
#include "frs7.h"

#include <sstream> 

//with linux, it seems  cstddef and string.h are needed for compile
#ifdef __linux__
#include <cstddef>
#include <string.h>
#endif

// history of changes :
// version 02 : first
// version 03 : add shadow flag
// version 04 : add LOADER_DATA_FROM_3DS
// version 05 : (07 jan 2016) new material system 
// version 06 : (25 feb 2016) add more data inside LOADER_DATA_FROM_3DS + possibility to save if no camera in scene.  note that version 6 can import version 5
// version 07 : lots of changes. not retrocompatible with previous versions. But really more flexible. the flexibility of this version should make it the very last version of FRS. 
int32_t const FRS7::m_FILE_VERSION = 0x00000007;

char const FRS7::m_HEADER_CHECKCODE[4] = {'F','R','S','F'}; // "FRSF" = FireRenderSceneFile
char const FRS7::m_HEADER_BADCHECKCODE[4] = {'B','A','D','0'};
char const STR__SHAPE_INSTANCE_REFERENCE_ID[] = "shape_instance_reference_id";
char const STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID[] = "shape_for_sky_light_portal_id";
char const STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID[] = "shape_for_environment_light_portal_id";

#define CHECK_STATUS_RETURNERROR { if (status != FR_SUCCESS) { FRS_MACRO_ERROR(); return status; } }
#define CHECK_STATUS_RETURNNULL { if (status != FR_SUCCESS) { FRS_MACRO_ERROR(); return 0; } }


#define FRS_MACRO_ERROR() ErrorDetected (__FUNCTION__,__LINE__,"");


FRS7::FRS7(std::fstream* myfile, bool allowWrite) : 
	m_allowWrite(allowWrite)
{
	m_frsFile = myfile;
	m_listObjectDeclared.clear();
	m_level = 0;

	m_idCounter = 1000; // just in order to avoid confusion with other numbers, I start this counter from 1000

	m_fileVersionOfLoadFile = 0; // only used when read FRS file
}

FRS7::~FRS7()
{

}

void FRS7::WarningDetected()
{
	//add breakpoint here for debugging frLoader
	int a=0;
}
void FRS7::ErrorDetected (const char* function, int32_t line, const char* message)
{
	//add breakpoint here for debugging frLoader
	int a=0;

	//if an error is detected, the file is corrupted anyway, so we can write error debug info at the end of the file.
	//Note that a bad file file is directly detected at the begining of the file with the string : m_HEADER_BADCHECKCODE
	if ( m_allowWrite && m_frsFile->is_open() && !m_frsFile->fail() )
	{
		std::ostringstream stringStream;
		stringStream << "\r\nERROR -- FUNC=" << std::string(function) << " LINE=" << line << " EXTRA_MESSAGE=" << message << " ENDERROR\r\n";
		std::string errorInfoStr = stringStream.str();

		m_frsFile->write(errorInfoStr.c_str(), errorInfoStr.length());
		m_frsFile->flush();
	}
}




fr_int FRS7::StoreEverything(
	fr_context context, 
	fr_scene scene
	)
{
	fr_int status = FR_SUCCESS;

	// write a wrong check code: the good check code will be written at the end, when we are sure that the generation didn't fail.
	// so if we have a crash or anything bad during StoreEverything, the FRS will be nicely unvalid.
	m_frsFile->write((const char*)&m_HEADER_BADCHECKCODE, sizeof(m_HEADER_BADCHECKCODE)); 
	m_frsFile->write((const char*)&m_FILE_VERSION, sizeof(m_FILE_VERSION));


	status = Store_Context(context);
	if (status != FR_SUCCESS) { FRS_MACRO_ERROR(); return status; }
	status = Store_Scene(scene);
	if (status != FR_SUCCESS) { FRS_MACRO_ERROR(); return status; }


	//store extra custom params
	for (auto it=m_extraCustomParam_int.begin(); it!=m_extraCustomParam_int.end(); ++it)
	{
		if ( !Store_ObjectParameter((*it).first.c_str(),FRS7::FRSPT_INT32_1,sizeof(float),&((*it).second)) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	for (auto it=m_extraCustomParam_float.begin(); it!=m_extraCustomParam_float.end(); ++it)
	{
		if ( !Store_ObjectParameter((*it).first.c_str(),FRS7::FRSPT_FLOAT1,sizeof(float),&((*it).second)) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}

	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}

	//rewind and write the good check code at the begining of the file.
	m_frsFile->seekp(0);
	m_frsFile->write((const char*)&m_HEADER_CHECKCODE, sizeof(m_HEADER_CHECKCODE));

	return status;
}


fr_int FRS7::Store_Camera(fr_camera camera)
{

	fr_int status = FR_SUCCESS;

	if ( !Store_StartObject("sceneCamera","fr_camera",camera) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	float param_fstop;
	status = frCameraGetInfo(camera, FR_CAMERA_FSTOP, sizeof(param_fstop), &param_fstop, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_FSTOP",FRSPT_FLOAT1,sizeof(param_fstop), &param_fstop) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	unsigned int param_apblade;
	status = frCameraGetInfo(camera, FR_CAMERA_APERTURE_BLADES, sizeof(unsigned int), &param_apblade, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_APERTURE_BLADES",FRSPT_UINT32_1,sizeof(param_apblade), &param_apblade) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	float param_camexpo;
	status = frCameraGetInfo(camera, FR_CAMERA_EXPOSURE, sizeof(param_camexpo), &param_camexpo, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_EXPOSURE",FRSPT_FLOAT1,sizeof(param_camexpo), &param_camexpo) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	float param_focalLen;
	status = frCameraGetInfo(camera, FR_CAMERA_FOCAL_LENGTH, sizeof(param_focalLen), &param_focalLen, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_FOCAL_LENGTH",FRSPT_FLOAT1,sizeof(param_focalLen), &param_focalLen) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	float param_sensorsize[2];
	status = frCameraGetInfo(camera, FR_CAMERA_SENSOR_SIZE, sizeof(param_sensorsize), &param_sensorsize, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_SENSOR_SIZE",FRSPT_FLOAT2,sizeof(param_sensorsize), &param_sensorsize) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	fr_camera_mode param_mode;
	status = frCameraGetInfo(camera, FR_CAMERA_MODE, sizeof(param_mode), &param_mode, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_MODE",FRSPT_UINT32_1,sizeof(param_mode), &param_mode) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	float param_orthowidth;
	status = frCameraGetInfo(camera, FR_CAMERA_ORTHO_WIDTH, sizeof(param_orthowidth), &param_orthowidth, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_ORTHO_WIDTH",FRSPT_FLOAT1,sizeof(param_orthowidth), &param_orthowidth) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

    float param_orthoheight;
    status = frCameraGetInfo(camera, FR_CAMERA_ORTHO_HEIGHT, sizeof(param_orthoheight), &param_orthoheight, NULL);
    CHECK_STATUS_RETURNERROR;
    if (!Store_ObjectParameter("FR_CAMERA_ORTHO_HEIGHT", FRSPT_FLOAT1, sizeof(param_orthoheight), &param_orthoheight)) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	float param_focusdist;
	status = frCameraGetInfo(camera, FR_CAMERA_FOCUS_DISTANCE, sizeof(param_focusdist), &param_focusdist, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_FOCUS_DISTANCE",FRSPT_FLOAT1,sizeof(param_focusdist), &param_focusdist) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	fr_float param_pos[4];
	status = frCameraGetInfo(camera, FR_CAMERA_POSITION, sizeof(param_pos), &param_pos, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_POSITION",FRSPT_FLOAT3,3*sizeof(float), param_pos) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	fr_float param_at[4];
	status = frCameraGetInfo(camera, FR_CAMERA_LOOKAT, sizeof(param_at), &param_at, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_LOOKAT",FRSPT_FLOAT3,3*sizeof(float), param_at) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	fr_float param_up[4];
	status = frCameraGetInfo(camera, FR_CAMERA_UP, sizeof(param_up), &param_up, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_CAMERA_UP",FRSPT_FLOAT3,3*sizeof(float), param_up) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }


	//save FR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = frCameraGetInfo(camera, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = frCameraGetInfo(camera, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	return FR_SUCCESS;
}


fr_int FRS7::Store_MaterialNode(fr_material_node shader, const std::string& name)
{

	fr_int status = FR_SUCCESS;


	int indexFound = -1;
	//search if this node is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "fr_material_node" && m_listObjectDeclared[iObj].obj == shader)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if node already saved, just save reference
		if ( !Store_ReferenceToObject(name,"fr_material_node",m_listObjectDeclared[indexFound].id) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if node not already saved



		if ( !Store_StartObject(name,"fr_material_node",shader) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_material_node_type nodeType = 0;
		status = frMaterialNodeGetInfo(shader,FR_MATERIAL_NODE_TYPE, sizeof(nodeType),&nodeType,NULL);
		CHECK_STATUS_RETURNNULL;
		if ( !Store_ObjectParameter("FR_MATERIAL_NODE_TYPE",FRSPT_UINT32_1,sizeof(nodeType), &nodeType) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		uint64_t nbInput = 0;
		status = frMaterialNodeGetInfo(shader,FR_MATERIAL_NODE_INPUT_COUNT, sizeof(nbInput),&nbInput,NULL);
		CHECK_STATUS_RETURNNULL;
		if ( !Store_ObjectParameter("FR_MATERIAL_NODE_INPUT_COUNT",FRSPT_UINT64_1,sizeof(nbInput), &nbInput) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		

		for (int i = 0; i < nbInput; i++)
		{
			size_t shaderParameterName_lenght_size_t = 0;
			status = frMaterialNodeGetInputInfo(shader, i, FR_MATERIAL_NODE_INPUT_NAME_STRING, 0, NULL, &shaderParameterName_lenght_size_t);
			uint64_t shaderParameterName_lenght = shaderParameterName_lenght_size_t;
			CHECK_STATUS_RETURNNULL;
			char* shaderParameterName = new char[shaderParameterName_lenght];
			status = frMaterialNodeGetInputInfo(shader, i, FR_MATERIAL_NODE_INPUT_NAME_STRING, shaderParameterName_lenght, shaderParameterName, NULL);
			CHECK_STATUS_RETURNNULL;
			//myfile->write((char*)&shaderParameterName_lenght, sizeof(shaderParameterName_lenght));
			//myfile->write((char*)shaderParameterName, shaderParameterName_lenght);

			fr_uint nodeInputType = 0;
			status = frMaterialNodeGetInputInfo(shader, i, FR_MATERIAL_NODE_INPUT_TYPE, sizeof(nodeInputType), &nodeInputType, NULL);
			CHECK_STATUS_RETURNNULL;
			//myfile->write((char*)&nodeInputType, sizeof(nodeInputType));

			size_t shaderParameterValue_lenght_size_t = 0;
			status = frMaterialNodeGetInputInfo(shader, i, FR_MATERIAL_NODE_INPUT_VALUE, 0, NULL, &shaderParameterValue_lenght_size_t);
			uint64_t shaderParameterValue_lenght = shaderParameterValue_lenght_size_t;
			CHECK_STATUS_RETURNNULL;
			char* shaderParameterValue = new char[shaderParameterValue_lenght];
			status = frMaterialNodeGetInputInfo(shader, i, FR_MATERIAL_NODE_INPUT_VALUE, shaderParameterValue_lenght, shaderParameterValue, NULL);
			CHECK_STATUS_RETURNNULL;


			if ( nodeInputType == FR_MATERIAL_NODE_INPUT_TYPE_NODE )
			{
				if (shaderParameterValue_lenght != sizeof(fr_material_node))
				{
					FRS_MACRO_ERROR();
					return FR_ERROR_INTERNAL_ERROR;
				}
				else
				{
				

					fr_material_node* shader = (fr_material_node*)shaderParameterValue;
					if (*shader != NULL)
					{
						//int32_t shaderExists = 1;
						//myfile->write((char*)&shaderExists, sizeof(shaderExists));
						status = Store_MaterialNode(*shader,shaderParameterName);
					}
					else
					{
						//int32_t shaderExists = 0;
						//myfile->write((char*)&shaderExists, sizeof(shaderExists));
					}
					CHECK_STATUS_RETURNERROR;

				}
			}
			else if (nodeInputType == FR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
			{
				if (shaderParameterValue_lenght != sizeof(fr_image))
				{
					FRS_MACRO_ERROR();
					return FR_ERROR_INTERNAL_ERROR;
				}
				else
				{
					fr_image* image = (fr_material_node*)shaderParameterValue;
					if (*image != NULL)
					{
						//int32_t imageExists = 1;
						//myfile->write((char*)&imageExists, sizeof(imageExists));
						status = Store_Image(*image,shaderParameterName);
					}
					else
					{
						//int32_t imageExists = 0;
						//myfile->write((char*)&imageExists, sizeof(imageExists));
					}
					CHECK_STATUS_RETURNERROR;
				}
			}
			else if ( nodeInputType == FR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 )
			{
				if ( !Store_ObjectParameter(shaderParameterName,FRSPT_FLOAT4,shaderParameterValue_lenght, shaderParameterValue) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
			}
			else if ( nodeInputType == FR_MATERIAL_NODE_INPUT_TYPE_UINT )
			{
				if ( !Store_ObjectParameter(shaderParameterName,FRSPT_UINT32_1,shaderParameterValue_lenght, shaderParameterValue) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
			}
			else
			{
				if ( !Store_ObjectParameter(shaderParameterName,FRSPT_UNDEF,shaderParameterValue_lenght, shaderParameterValue) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
			}

			delete[] shaderParameterName; shaderParameterName = NULL;
			delete[] shaderParameterValue; shaderParameterValue = NULL;
		}


		//save FR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = frMaterialNodeGetInfo(shader, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = frMaterialNodeGetInfo(shader, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	}

	return status;
}

fr_int FRS7::Store_Light(fr_light light, fr_scene scene)
{

	fr_int status = FR_SUCCESS;

	if ( !Store_StartObject("sceneLight","fr_light",light) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	fr_light_type type = 0;
	status = frLightGetInfo(light, FR_LIGHT_TYPE, sizeof(fr_light_type), &type, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_LIGHT_TYPE",FRSPT_UNDEF,sizeof(fr_light_type), &type) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	if (type == FR_LIGHT_TYPE_POINT)
	{
		float radian4f[4];
		status = frLightGetInfo(light, FR_POINT_LIGHT_RADIANT_POWER, sizeof(radian4f), &radian4f, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_POINT_LIGHT_RADIANT_POWER",FRSPT_FLOAT3,3*sizeof(float), radian4f) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	else if (type == FR_LIGHT_TYPE_DIRECTIONAL)
	{
		float radian4f[4];
		status = frLightGetInfo(light, FR_DIRECTIONAL_LIGHT_RADIANT_POWER, sizeof(radian4f), &radian4f, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_DIRECTIONAL_LIGHT_RADIANT_POWER",FRSPT_FLOAT3,3*sizeof(float), radian4f) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	else if (type == FR_LIGHT_TYPE_SPOT)
	{
		float radian4f[4];
		status = frLightGetInfo(light, FR_SPOT_LIGHT_RADIANT_POWER, sizeof(radian4f), &radian4f, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SPOT_LIGHT_RADIANT_POWER",FRSPT_FLOAT3,3*sizeof(float), radian4f) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		float coneShape[2];
		status = frLightGetInfo(light, FR_SPOT_LIGHT_CONE_SHAPE, sizeof(coneShape), &coneShape, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SPOT_LIGHT_CONE_SHAPE",FRSPT_FLOAT2,sizeof(coneShape), coneShape) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	else if (type == FR_LIGHT_TYPE_ENVIRONMENT)
	{
		fr_image image = NULL;
		status = frLightGetInfo(light, FR_ENVIRONMENT_LIGHT_IMAGE, sizeof(fr_image), &image, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( image )
		{
			status = Store_Image((fr_image)image,"FR_ENVIRONMENT_LIGHT_IMAGE");
			CHECK_STATUS_RETURNERROR;
		}

		float intensityScale;
		status = frLightGetInfo(light, FR_ENVIRONMENT_LIGHT_INTENSITY_SCALE, sizeof(intensityScale), &intensityScale, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_ENVIRONMENT_LIGHT_INTENSITY_SCALE",FRSPT_FLOAT1,sizeof(intensityScale), &intensityScale) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		
		size_t nbOfPortals = 0;
		status = frLightGetInfo(light,FR_ENVIRONMENT_LIGHT_PORTAL_COUNT, sizeof(nbOfPortals), &nbOfPortals, NULL );
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_ENVIRONMENT_LIGHT_PORTAL_COUNT",FRSPT_INT64_1,sizeof(nbOfPortals), &nbOfPortals) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		if ( nbOfPortals > 0 )
		{
			fr_shape* shapes = new fr_shape[nbOfPortals];
			status = frLightGetInfo(light, FR_ENVIRONMENT_LIGHT_PORTAL_LIST,   nbOfPortals * sizeof(fr_shape), shapes, NULL);
			CHECK_STATUS_RETURNERROR;

			for (uint64_t i = 0; i < nbOfPortals; i++)
			{
				fr_int err = Store_Shape(shapes[i],STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID);
				if (err != FR_SUCCESS)
				{
					FRS_MACRO_ERROR();
					return err;
				}
			}

			delete[] shapes; shapes=NULL;
		}

	}
	else if (type == FR_LIGHT_TYPE_SKY)
	{
		float scale = 0.0f;
		status = frLightGetInfo(light, FR_SKY_LIGHT_SCALE, sizeof(scale), &scale, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SKY_LIGHT_SCALE",FRSPT_FLOAT1,sizeof(scale), &scale) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		float albedo = 0.0f;
		status = frLightGetInfo(light, FR_SKY_LIGHT_ALBEDO, sizeof(albedo), &albedo, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SKY_LIGHT_ALBEDO",FRSPT_FLOAT1,sizeof(albedo), &albedo) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		float turbidity = 0.0f;
		status = frLightGetInfo(light, FR_SKY_LIGHT_TURBIDITY, sizeof(turbidity), &turbidity, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SKY_LIGHT_TURBIDITY",FRSPT_FLOAT1,sizeof(turbidity), &turbidity) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		size_t nbOfPortals = 0;
		status = frLightGetInfo(light,FR_SKY_LIGHT_PORTAL_COUNT, sizeof(nbOfPortals), &nbOfPortals, NULL );
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SKY_LIGHT_PORTAL_COUNT",FRSPT_INT64_1,sizeof(nbOfPortals), &nbOfPortals) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		if ( nbOfPortals > 0 )
		{
			fr_shape* shapes = new fr_shape[nbOfPortals];
			status = frLightGetInfo(light, FR_SKY_LIGHT_PORTAL_LIST,   nbOfPortals * sizeof(fr_shape), shapes, NULL);
			CHECK_STATUS_RETURNERROR;

			for (uint64_t i = 0; i < nbOfPortals; i++)
			{
				fr_int err = Store_Shape(shapes[i],STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID);
				if (err != FR_SUCCESS)
				{
					FRS_MACRO_ERROR();
					return err;
				}
			}

			delete[] shapes; shapes=NULL;
		}

	}
	else if (type == FR_LIGHT_TYPE_IES)
	{
		float radian4f[4];
		status = frLightGetInfo(light, FR_IES_LIGHT_RADIANT_POWER, sizeof(radian4f), radian4f, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_IES_LIGHT_RADIANT_POWER",FRSPT_FLOAT3,3*sizeof(float), radian4f) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_ies_image_desc iesdesc;
		status = frLightGetInfo(light, FR_IES_LIGHT_IMAGE_DESC, sizeof(iesdesc), &iesdesc, NULL);
		CHECK_STATUS_RETURNERROR;

		if ( !Store_ObjectParameter("FR_IES_LIGHT_IMAGE_DESC_W",FRSPT_INT32_1,sizeof(iesdesc.w), &iesdesc.w) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		if ( !Store_ObjectParameter("FR_IES_LIGHT_IMAGE_DESC_H",FRSPT_INT32_1,sizeof(iesdesc.h), &iesdesc.h) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		if ( iesdesc.data )
		{
			uint64_t dataSize =  strlen(iesdesc.data)+1; // include null-terminated character
			if ( !Store_ObjectParameter("FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE",FRSPT_UINT64_1,sizeof(dataSize), &dataSize) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
			if ( !Store_ObjectParameter("FR_IES_LIGHT_IMAGE_DESC_DATA",FRSPT_UNDEF,dataSize, iesdesc.data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		}
		else
		{
			WarningDetected();
		}

	}
	else
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INVALID_PARAMETER;
	}

	float lightTransform[16];
	status = frLightGetInfo(light, FR_LIGHT_TRANSFORM, sizeof(lightTransform), &lightTransform, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("FR_LIGHT_TRANSFORM",FRSPT_FLOAT16,sizeof(lightTransform), lightTransform) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	//save FR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = frLightGetInfo(light, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = frLightGetInfo(light, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	return FR_SUCCESS;

}

fr_int FRS7::Store_Shape(fr_shape shape, const std::string& name)
{
	fr_int status = FR_SUCCESS;

	int indexFound = -1;
	//search if this shape is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "fr_shape" && m_listObjectDeclared[iObj].obj == shape)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if shape already saved, just save reference
		if ( !Store_ReferenceToObject(name,"fr_shape",m_listObjectDeclared[indexFound].id) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if shape not already saved

		if ( !Store_StartObject(name,"fr_shape",shape) ) { return FR_ERROR_INTERNAL_ERROR; }

		fr_shape_type type;
		status = frShapeGetInfo(shape, FR_SHAPE_TYPE, sizeof(fr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_TYPE",FRSPT_UNDEF,sizeof(type), &type) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		if (
			type == FR_SHAPE_TYPE_MESH
			)
		{

			//m_shapeList.push_back(shape);

			uint64_t poly_count = 0;
			status = frMeshGetInfo(shape, FR_MESH_POLYGON_COUNT, sizeof(poly_count), &poly_count, NULL); //number of primitives
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("FR_MESH_POLYGON_COUNT",FRSPT_UINT64_1,sizeof(poly_count), &poly_count) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

			uint64_t  vertex_count = 0;
			status = frMeshGetInfo(shape, FR_MESH_VERTEX_COUNT, sizeof(vertex_count), &vertex_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("FR_MESH_VERTEX_COUNT",FRSPT_UINT64_1,sizeof(vertex_count), &vertex_count) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

			uint64_t normal_count = 0;
			status = frMeshGetInfo(shape, FR_MESH_NORMAL_COUNT, sizeof(normal_count), &normal_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("FR_MESH_NORMAL_COUNT",FRSPT_UINT64_1,sizeof(normal_count), &normal_count) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

			uint64_t uv_count = 0;
			status = frMeshGetInfo(shape, FR_MESH_UV_COUNT, sizeof(uv_count), &uv_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("FR_MESH_UV_COUNT",FRSPT_UINT64_1,sizeof(uv_count), &uv_count) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		

			if (vertex_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_VERTEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = frMeshGetInfo(shape, FR_MESH_VERTEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_VERTEX_ARRAY",FRSPT_UINT64_1,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			}
			if (normal_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_NORMAL_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = frMeshGetInfo(shape, FR_MESH_NORMAL_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_NORMAL_ARRAY",FRSPT_UINT64_1,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			}
			if (uv_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_UV_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = frMeshGetInfo(shape, FR_MESH_UV_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_UV_ARRAY",FRSPT_UINT64_1,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; 
				data = NULL;
			
			}
			if (poly_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_VERTEX_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = frMeshGetInfo(shape, FR_MESH_VERTEX_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_VERTEX_INDEX_ARRAY",FRSPT_UNDEF,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (normal_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_NORMAL_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = frMeshGetInfo(shape, FR_MESH_NORMAL_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_NORMAL_INDEX_ARRAY",FRSPT_UNDEF,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (uv_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_UV_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = frMeshGetInfo(shape, FR_MESH_UV_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_UV_INDEX_ARRAY",FRSPT_UNDEF,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (poly_count)
			{
				size_t required_size_size_t = 0;
				status = frMeshGetInfo(shape, FR_MESH_NUM_FACE_VERTICES_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = frMeshGetInfo(shape, FR_MESH_NUM_FACE_VERTICES_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("FR_MESH_NUM_VERTICES_ARRAY",FRSPT_UNDEF,required_size, data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}

		}
		else if (type == FR_SHAPE_TYPE_INSTANCE)
		{
			//search address of original shape

			fr_shape shapeOriginal = NULL;
			status = frInstanceGetBaseShape(shape, &shapeOriginal);
			CHECK_STATUS_RETURNERROR;

			if (shapeOriginal == 0)
			{
				FRS_MACRO_ERROR();
				return FR_ERROR_INTERNAL_ERROR;
			}

			bool found = false;
			for (int iShape = 0; iShape < m_listObjectDeclared.size(); iShape++)
			{
				if (m_listObjectDeclared[iShape].type == "fr_shape" && m_listObjectDeclared[iShape].obj == shapeOriginal)
				{
					int32_t IDShapeBase = m_listObjectDeclared[iShape].id;
					if ( !Store_ObjectParameter(STR__SHAPE_INSTANCE_REFERENCE_ID,FRSPT_INT32_1,sizeof(IDShapeBase), &IDShapeBase) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
					found = true;
					break;
				}
			}

			if (!found)
			{
				FRS_MACRO_ERROR();
				return FR_ERROR_INTERNAL_ERROR;
			}

		}


		fr_material_node data_shader = 0;
		status = frShapeGetInfo(shape, FR_SHAPE_MATERIAL, sizeof(data_shader), &data_shader, NULL);
		CHECK_STATUS_RETURNERROR;
		if (data_shader != NULL)
		{
			fr_int error = Store_MaterialNode(data_shader,"shaderOfShape");
			if (error != FR_SUCCESS)
			{
				return error;
			}
		}
	
	
		float data_transform[16];
		status = frShapeGetInfo(shape, FR_SHAPE_TRANSFORM, sizeof(data_transform), data_transform, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_TRANSFORM",FRSPT_FLOAT16,sizeof(data_transform), data_transform) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	
		float data_linMotion[4];
		status = frShapeGetInfo(shape, FR_SHAPE_LINEAR_MOTION, sizeof(data_linMotion) , data_linMotion, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_LINEAR_MOTION",FRSPT_FLOAT3,3*sizeof(float), data_linMotion) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		float data_angMotion[4];
		status = frShapeGetInfo(shape, FR_SHAPE_ANGULAR_MOTION, sizeof(data_angMotion) , data_angMotion, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_ANGULAR_MOTION",FRSPT_FLOAT4,sizeof(data_angMotion), data_angMotion) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_bool data_visFlag;
		status = frShapeGetInfo(shape, FR_SHAPE_VISIBILITY_FLAG, sizeof(data_visFlag), &data_visFlag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_VISIBILITY",FRSPT_INT32_1,sizeof(data_visFlag), &data_visFlag) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	
		fr_bool data_shadoflag;
		status = frShapeGetInfo(shape, FR_SHAPE_SHADOW_FLAG, sizeof(data_shadoflag), &data_shadoflag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_SHADOW",FRSPT_INT32_1,sizeof(data_shadoflag), &data_shadoflag) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_bool data_shadocatcherflag;
		status = frShapeGetInfo(shape, FR_SHAPE_SHADOW_CATCHER_FLAG, sizeof(data_shadocatcherflag), &data_shadocatcherflag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_SHADOW_CATCHER",FRSPT_INT32_1,sizeof(data_shadocatcherflag), &data_shadocatcherflag) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }


		fr_uint data_subdiv;
		status = frShapeGetInfo(shape, FR_SHAPE_SUBDIVISION_FACTOR, sizeof(data_subdiv), &data_subdiv, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_SUBDIVISION_FACTOR",FRSPT_UINT32_1,sizeof(data_subdiv), &data_subdiv) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_float data_displacementscale[2];
		status = frShapeGetInfo(shape, FR_SHAPE_DISPLACEMENT_SCALE, sizeof(data_displacementscale), &data_displacementscale, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("FR_SHAPE_DISPLACEMENT_SCALE",FRSPT_FLOAT2,sizeof(data_displacementscale), &data_displacementscale) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_image displacementimage = NULL;
		status = frShapeGetInfo(shape, FR_SHAPE_DISPLACEMENT_IMAGE, sizeof(fr_image), &displacementimage, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( displacementimage )
		{
			status = Store_Image(displacementimage,"FR_SHAPE_DISPLACEMENT_IMAGE");
			CHECK_STATUS_RETURNERROR;
		}


		//save FR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = frShapeGetInfo(shape, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = frShapeGetInfo(shape, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	}
	return FR_SUCCESS;
}


fr_int FRS7::Store_Image(fr_image image, const std::string& name)
{
	
	fr_int status = FR_SUCCESS;


	int indexFound = -1;
	//search if this image is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "fr_image" && m_listObjectDeclared[iObj].obj == image)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if image already saved, just save reference
		if ( !Store_ReferenceToObject(name,"fr_image",m_listObjectDeclared[indexFound].id) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if image not already saved


		if ( !Store_StartObject(name,"fr_image",image) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	
		//auto pos = std::find(m_imageList.begin(), m_imageList.end(), image);
		//if (pos == m_imageList.end())
		//{
		//	//if image not already saved
		//	m_imageList.push_back(image);
		//	int32_t imageIndex = -1; // index = -1 --> means that the image was not already saved prviously
		//	myfile->write((const char*)&imageIndex, sizeof(imageIndex));
		//}
		//else
		//{
		//	//if image already saved
		//	int32_t imageIndex = (int32_t)(pos - m_imageList.begin());
		//	myfile->write((const char*)&imageIndex, sizeof(imageIndex));
		//	myfile->write((const char*)&m_IMAGE_END, sizeof(m_IMAGE_END));
		//	return FR_SUCCESS;
		//}

	
		fr_image_format format;
		status = frImageGetInfo(image, FR_IMAGE_FORMAT, sizeof(format), &format, NULL);
		CHECK_STATUS_RETURNERROR;
		//myfile->write((char*)&format, sizeof(fr_image_format));
		if ( !Store_ObjectParameter("FR_IMAGE_FORMAT",FRSPT_UNDEF,sizeof(fr_image_format), &format) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		fr_image_desc desc;
		status = frImageGetInfo(image, FR_IMAGE_DESC, sizeof(desc), &desc, NULL);
		CHECK_STATUS_RETURNERROR
		//myfile->write((char*)&desc, sizeof(fr_image_desc));
		if ( !Store_ObjectParameter("FR_IMAGE_DESC",FRSPT_UNDEF,sizeof(fr_image_desc), &desc) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		size_t size_size_t = 0;
		status = frImageGetInfo(image, FR_IMAGE_DATA, 0, NULL, &size_size_t);
		uint64_t size = size_size_t;
		CHECK_STATUS_RETURNERROR
		char *idata = new char[size];
		//myfile->write((char*)&size, sizeof(uint64_t));
		status = frImageGetInfo(image, FR_IMAGE_DATA, size, idata, NULL);
		CHECK_STATUS_RETURNERROR
		//myfile->write(idata, size);
		if ( !Store_ObjectParameter("FR_IMAGE_DATA",FRSPT_UNDEF,size, idata) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

		delete[] idata; idata = NULL;
	

		//save FR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = frImageGetInfo(image, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = frImageGetInfo(image, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	}
	return status;
}



fr_int FRS7::Store_Scene(fr_scene scene)
{
	fr_int status = FR_SUCCESS;

	if ( !Store_StartObject("sceneDeclare","fr_scene",scene) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	uint64_t nbShape = 0;
	status = frSceneGetInfo(scene, FR_SCENE_SHAPE_COUNT, sizeof(nbShape), &nbShape, NULL);
	CHECK_STATUS_RETURNERROR;
	uint64_t nbLight = 0;
	status = frSceneGetInfo(scene, FR_SCENE_LIGHT_COUNT, sizeof(nbLight), &nbLight, NULL);
	CHECK_STATUS_RETURNERROR;
	//uint64_t nbTexture = 0;
	//status = frSceneGetInfo(scene, FR_SCENE_TEXTURE_COUNT, sizeof(nbTexture), &nbTexture, NULL);
	//CHECK_STATUS_RETURNERROR;


	//myfile->write((char*)&nbShape, sizeof(nbShape));
	//myfile->write((char*)&nbLight, sizeof(nbLight));
	//myfile->write((char*)&nbTexture, sizeof(nbTexture));

	fr_shape* shapes = NULL;
	fr_light* light = NULL;

	if ( nbShape >= 1 )
	{
		shapes = new fr_shape[nbShape];
		status = frSceneGetInfo(scene, FR_SCENE_SHAPE_LIST,   nbShape * sizeof(fr_shape), shapes, NULL);
		CHECK_STATUS_RETURNERROR;
	}

	if ( nbLight >= 1 )
	{
		light = new fr_light[nbLight];
		status = frSceneGetInfo(scene, FR_SCENE_LIGHT_LIST, nbLight * sizeof(fr_light), light, NULL);
		CHECK_STATUS_RETURNERROR;
	}


	for (uint64_t i = 0; i < nbShape; i++) // first: store non-instanciate shapes
	{
		fr_shape_type type;
		status = frShapeGetInfo(shapes[i], FR_SHAPE_TYPE, sizeof(fr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if (type == FR_SHAPE_TYPE_MESH)
		{
			fr_int err = Store_Shape(shapes[i],"nonInstancedSceneShape");
			if (err != FR_SUCCESS)
			{
				FRS_MACRO_ERROR();
				return err; // easier for debug if stop everything
			}
		}
	}

	for (uint64_t i = 0; i < nbShape; i++)// second: store instanciate shapes only
	{
		fr_shape_type type;
		status = frShapeGetInfo(shapes[i], FR_SHAPE_TYPE, sizeof(fr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if (type != FR_SHAPE_TYPE_MESH)
		{
			fr_int err = Store_Shape(shapes[i],"InstancedSceneShape");
			if (err != FR_SUCCESS)
			{
				FRS_MACRO_ERROR();
				return err; // easier for debug if stop everything
			}
		}
	}

	for (uint64_t i = 0; i < nbLight; i++)
	{
		fr_int err = Store_Light(light[i],scene);
		if (err != FR_SUCCESS)
		{
			FRS_MACRO_ERROR();
			return err; // easier for debug if stop everything
		}
	}

	if ( shapes ) { delete[] shapes; shapes = NULL; }
	if ( light )  { delete[] light;  light = NULL;}

	fr_camera camera = NULL;
	status = frSceneGetCamera(scene, &camera);
	CHECK_STATUS_RETURNERROR;
	
	if (camera)
	{
		status = Store_Camera(camera);
		CHECK_STATUS_RETURNERROR;
	}

	//save FR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = frSceneGetInfo(scene, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = frSceneGetInfo(scene, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	return FR_SUCCESS;

}


fr_int FRS7::Store_Context(fr_context context)
{

	fr_int status = FR_SUCCESS;

	if ( !Store_StartObject("contextDeclare","fr_context",context) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	uint64_t param_count = 0;
	status = frContextGetInfo(context, FR_CONTEXT_PARAMETER_COUNT, sizeof(uint64_t), &param_count, NULL);
	CHECK_STATUS_RETURNERROR;

	unsigned int imagefilter_type = -1;
	unsigned int tonemapping_type = -1;

	for (int iPass = 0; iPass < 2; iPass++)
	{
		for (uint64_t i = 0; i < param_count; i++)
		{
			size_t name_length_size_t = 0;
			status = frContextGetParameterInfo(context, int(i), FR_PARAMETER_NAME_STRING, 0, NULL, &name_length_size_t);
			uint64_t name_length = name_length_size_t;
			CHECK_STATUS_RETURNERROR;
			char* paramName = new char[name_length];
			status = frContextGetParameterInfo(context, int(i), FR_PARAMETER_NAME_STRING, name_length, paramName, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( paramName[name_length-1] != 0 ) // supposed to be null-terminated
			{
				 FRS_MACRO_ERROR(); 
				 return FR_ERROR_INTERNAL_ERROR;
			}

			fr_context_info paramID = 0;
			status = frContextGetParameterInfo(context, int(i), FR_PARAMETER_NAME, sizeof(paramID), &paramID, NULL);
			CHECK_STATUS_RETURNERROR;

			//need to store in 2 passes.
			//first, we store the parameters that will set several default parameters.
			//so we are sure that all custom parameters will be set during the load of context
			if (
				(iPass == 0 && ( paramID == FR_CONTEXT_IMAGE_FILTER_TYPE || paramID == FR_CONTEXT_TONE_MAPPING_TYPE ))
				||
				(iPass == 1 && paramID != FR_CONTEXT_IMAGE_FILTER_TYPE && paramID != FR_CONTEXT_TONE_MAPPING_TYPE)
				)
			{

				//don't store values that don't fitt with filter/tonemap types
				bool storeValue = true;
				if ( paramID == FR_CONTEXT_IMAGE_FILTER_BOX_RADIUS && imagefilter_type != FR_FILTER_BOX ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_IMAGE_FILTER_GAUSSIAN_RADIUS && imagefilter_type != FR_FILTER_GAUSSIAN ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_IMAGE_FILTER_TRIANGLE_RADIUS && imagefilter_type != FR_FILTER_TRIANGLE ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_IMAGE_FILTER_MITCHELL_RADIUS && imagefilter_type != FR_FILTER_MITCHELL ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_IMAGE_FILTER_LANCZOS_RADIUS && imagefilter_type != FR_FILTER_LANCZOS ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_IMAGE_FILTER_BLACKMANHARRIS_RADIUS && imagefilter_type != FR_FILTER_BLACKMANHARRIS ) { storeValue = false; }

				if ( paramID == FR_CONTEXT_TONE_MAPPING_LINEAR_SCALE && tonemapping_type != FR_TONEMAPPING_OPERATOR_LINEAR ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_SENSITIVITY && tonemapping_type != FR_TONEMAPPING_OPERATOR_PHOTOLINEAR ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_EXPOSURE && tonemapping_type != FR_TONEMAPPING_OPERATOR_PHOTOLINEAR ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_FSTOP && tonemapping_type != FR_TONEMAPPING_OPERATOR_PHOTOLINEAR ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_TONE_MAPPING_REINHARD02_PRE_SCALE && tonemapping_type != FR_TONEMAPPING_OPERATOR_REINHARD02 ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_TONE_MAPPING_REINHARD02_POST_SCALE && tonemapping_type != FR_TONEMAPPING_OPERATOR_REINHARD02 ) { storeValue = false; }
				if ( paramID == FR_CONTEXT_TONE_MAPPING_REINHARD02_BURN && tonemapping_type != FR_TONEMAPPING_OPERATOR_REINHARD02 ) { storeValue = false; }



				if ( storeValue )
				{
					fr_parameter_type type;
					status = frContextGetParameterInfo(context, int(i), FR_PARAMETER_TYPE, sizeof(type), &type, NULL);
					CHECK_STATUS_RETURNERROR;

					size_t value_length_size_t = 0;
					status = frContextGetParameterInfo(context, int(i), FR_PARAMETER_VALUE, 0, NULL, &value_length_size_t);
					uint64_t value_length = value_length_size_t;
					CHECK_STATUS_RETURNERROR;

					char* paramValue = NULL;
					if (value_length > 0)
					{
						paramValue = new char[value_length];
						status = frContextGetParameterInfo(context, int(i), FR_PARAMETER_VALUE, value_length, paramValue, NULL);
						CHECK_STATUS_RETURNERROR;

						if (  paramID == FR_CONTEXT_IMAGE_FILTER_TYPE && type == FR_PARAMETER_TYPE_UINT) 
						{ 
							imagefilter_type = ((unsigned int*)(paramValue))[0]; 
						}
						if (  paramID == FR_CONTEXT_TONE_MAPPING_TYPE && type == FR_PARAMETER_TYPE_UINT) 
						{ 
							tonemapping_type = ((unsigned int*)(paramValue))[0]; 
						}

					}

					if ( !Store_ObjectParameter(paramName,FRPARAMETERTYPE_to_FRSPT(type),value_length,paramValue) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if ( paramValue ) { delete[] paramValue; paramValue = NULL; }
				}
			}

			delete[] paramName; paramName = NULL;
		}
	}


	//save FR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = frContextGetInfo(context, FR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = frContextGetInfo(context, FR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("FR_OBJECT_NAME",FRSPT_UNDEF,frobjectName_size, frobjectName_data) ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;



	if ( !Store_EndObject() ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	return FR_SUCCESS;
}


bool FRS7::Store_String( const std::string& str)
{
	uint32_t strSize = (uint32_t)str.length();
	m_frsFile->write((const char*)&strSize, sizeof(strSize)); 
	if ( strSize > 0 )
	{
		m_frsFile->write((const char*)str.c_str(), strSize); 
	}
	return true;
}

bool FRS7::Store_ReferenceToObject(const std::string& objName, const std::string& type, int32_t id)
{
	//just for debug purpose, we check that the referenced ID actually exists
	bool found = false;
	for(int i=0; i<m_listObjectDeclared.size(); i++)
	{
		if ( m_listObjectDeclared[i].id == id )
		{
			found = true;
			break;
		}
	}
	if ( !found )
	{
		FRS_MACRO_ERROR();
		return false;
	}

	FRS_ELEMENTS_TYPE typeWrite = FRSRT_REFERENCE;
	m_frsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	if ( !Store_String(objName)) { FRS_MACRO_ERROR(); return false; }
	if ( !Store_String(type.c_str())) { FRS_MACRO_ERROR(); return false; }
	m_frsFile->write((const char*)&id, sizeof(id)); 

	return true;
}

bool FRS7::Store_StartObject( const std::string& objName,const std::string& type, void* obj)
{
	FRS_ELEMENTS_TYPE typeWrite = FRSRT_OBJECT_BEG;
	m_frsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	if ( !Store_String(objName)) { FRS_MACRO_ERROR(); return false; }
	if ( !Store_String(type.c_str())) { FRS_MACRO_ERROR(); return false; }
	m_frsFile->write((const char*)&m_idCounter, sizeof(m_idCounter)); 
	
	RPS_OBJECT_DECLARED newObj;
	newObj.id = m_idCounter;
	newObj.type = type;
	newObj.obj = obj;
	m_listObjectDeclared.push_back(newObj);

	m_level++;
	m_idCounter++;

	return true;
}

bool FRS7::Store_EndObject()
{
	m_level--;
	if ( m_level < 0 )
	{
		FRS_MACRO_ERROR();
		return false;
	}

	FRS_ELEMENTS_TYPE typeWrite = FRSRT_OBJECT_END;
	m_frsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	Store_String(""); // empty string
	return true;
}

bool FRS7::Store_ObjectParameter( const std::string& parameterName,FRS_PARAMETER_TYPE type,uint64_t dataSize, const void* data)
{
	if ( data == NULL && dataSize > 0 )
	{
		FRS_MACRO_ERROR();
		return false;
	}
	if ( sizeof(FRS_PARAMETER_TYPE) != sizeof(uint32_t) )
	{
		FRS_MACRO_ERROR();
		return false;
	}

	int32_t sizeOfFRSPT = FRSPT_to_size(type);
	if ( sizeOfFRSPT != -1 && sizeOfFRSPT != dataSize )
	{
		FRS_MACRO_ERROR();
		return false;
	}

	FRS_ELEMENTS_TYPE typeWrite = FRSRT_PARAMETER;
	m_frsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	if ( !Store_String(parameterName)){ FRS_MACRO_ERROR(); return false;  }
	m_frsFile->write((const char*)&type, sizeof(type)); 
	m_frsFile->write((const char*)&dataSize, sizeof(dataSize)); 
	m_frsFile->write((const char*)data, dataSize); 
	return true;
}

int32_t FRS7::FRSPT_to_size(FRS_PARAMETER_TYPE in)
{
		 if ( in == FRSPT_FLOAT1 ) { return sizeof(float)*1; }
	else if ( in == FRSPT_FLOAT2 ) { return sizeof(float)*2; }
	else if ( in == FRSPT_FLOAT3 ) { return sizeof(float)*3; }
	else if ( in == FRSPT_FLOAT4 ) { return sizeof(float)*4; }
	else if ( in == FRSPT_FLOAT16 ) { return sizeof(float)*16; }

	else if ( in == FRSPT_UINT32_1 ) { return sizeof(uint32_t)*1; }
	return -1;
}

fr_parameter_type FRS7::FRSPT_to_FRPARAMETERTYPE(FRS_PARAMETER_TYPE in)
{	
		 if ( in == FRSPT_FLOAT1 ) { return FR_PARAMETER_TYPE_FLOAT; }
	else if ( in == FRSPT_FLOAT2 ) { return FR_PARAMETER_TYPE_FLOAT2; }
	else if ( in == FRSPT_FLOAT3 ) { return FR_PARAMETER_TYPE_FLOAT3; }
	else if ( in == FRSPT_FLOAT4 ) { return FR_PARAMETER_TYPE_FLOAT4; }
	else if ( in == FRSPT_UINT32_1 ) { return FR_PARAMETER_TYPE_UINT; }
	FRS_MACRO_ERROR();
	return 0;
}

FRS7::FRS_PARAMETER_TYPE FRS7::FRPARAMETERTYPE_to_FRSPT(fr_parameter_type in)
{
		 if ( in == FR_PARAMETER_TYPE_FLOAT ) { return FRSPT_FLOAT1; }
	else if ( in == FR_PARAMETER_TYPE_FLOAT2 ) { return FRSPT_FLOAT2; }
	else if ( in == FR_PARAMETER_TYPE_FLOAT3 ) { return FRSPT_FLOAT3; }
	else if ( in == FR_PARAMETER_TYPE_FLOAT4 ) { return FRSPT_FLOAT4; }
	else if ( in == FR_PARAMETER_TYPE_IMAGE ) { return FRSPT_UNDEF; }
	else if ( in == FR_PARAMETER_TYPE_STRING ) { return FRSPT_UNDEF; }
	else if ( in == FR_PARAMETER_TYPE_SHADER ) { return FRSPT_UNDEF; }
	else if ( in == FR_PARAMETER_TYPE_UINT ) { return FRSPT_UINT32_1; }
	return FRSPT_UNDEF;
}


fr_int FRS7::LoadEverything(
	fr_context context, 
	fr_material_system materialSystem,
	fr_scene& scene,
	bool useAlreadyExistingScene
	)
{

	fr_int status = FR_SUCCESS;

	//check arguments:
	if (context == NULL) { FRS_MACRO_ERROR(); return FR_ERROR_INVALID_PARAMETER; }
	if (scene != NULL && !useAlreadyExistingScene ) { FRS_MACRO_ERROR(); return FR_ERROR_INVALID_PARAMETER; }
	if (scene == NULL && useAlreadyExistingScene )  { FRS_MACRO_ERROR(); return FR_ERROR_INVALID_PARAMETER; }

	char headCheckCode[4] = {0,0,0,0};
	m_frsFile->read(headCheckCode, sizeof(headCheckCode));
	if (headCheckCode[0] != m_HEADER_CHECKCODE[0] || 
		headCheckCode[1] != m_HEADER_CHECKCODE[1] || 
		headCheckCode[2] != m_HEADER_CHECKCODE[2] || 
		headCheckCode[3] != m_HEADER_CHECKCODE[3]  )
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}

	m_frsFile->read((char*)&m_fileVersionOfLoadFile, sizeof(m_fileVersionOfLoadFile));
	if (m_fileVersionOfLoadFile != m_FILE_VERSION)
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}

	fr_int succes = Read_Context(context);
	if (succes != FR_SUCCESS)
	{
		FRS_MACRO_ERROR();
		return succes;
	}

	
	fr_scene newScene = Read_Scene(context, materialSystem, useAlreadyExistingScene ? scene : NULL);
	if (newScene == 0)
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}
	


	//load extra custom param
	while(true)
	{
		std::string elementName;
		std::string objBegType;
		FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == FRSRT_PARAMETER )
		{
			std::string paramName;
			FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

			if ( paramDataSize > 0 )
			{
				//char* data_data = new char[paramDataSize];
				

				if (paramType == FRSPT_INT32_1)
				{
					fr_int data_int;
					if ( Read_Element_ParameterData(&data_int,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == sizeof(fr_int))
					{
						m_extraCustomParam_int[paramName] = data_int;
					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				
				}
				else if (paramType == FRSPT_FLOAT1)
				{
					fr_float data_float;
					if ( Read_Element_ParameterData(&data_float,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == sizeof(fr_float))
					{
						m_extraCustomParam_float[paramName] = data_float;
					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				}
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					WarningDetected();

					delete[] data_data; data_data = NULL;
				}

				//delete[] data_data; data_data = NULL;
			}

		}
		else if ( nextElem == FRSRT_UNDEF ) // if reached end of file
		{
			break;
		}
		else
		{
			FRS_MACRO_ERROR();
			return FR_ERROR_INTERNAL_ERROR;
		}


	}




	if ( !useAlreadyExistingScene )
	{
		scene = newScene;
	}
	
	//overwrite the gamma with the 3dsmax gamma
	auto dsmax_gammaenabled_found = m_extraCustomParam_int.find("3dsmax.gammaenabled");
	auto dsmax_displaygamma_found = m_extraCustomParam_float.find("3dsmax.displaygamma");
	if ( dsmax_gammaenabled_found != m_extraCustomParam_int.end() && dsmax_displaygamma_found != m_extraCustomParam_float.end() )
	{
		if ( m_extraCustomParam_int["3dsmax.gammaenabled"] != 0 )
		{
			status = frContextSetParameter1f(context,"displaygamma" , m_extraCustomParam_float["3dsmax.displaygamma"]);
			if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
		}
	}

	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}


	return FR_SUCCESS;
}


fr_int FRS7::Read_Context(fr_context context)
{
	fr_int status = FR_SUCCESS;


	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	if ( objType != "fr_context" )
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}


	while(true)
	{
		std::string elementName;
		std::string objBegType;
		FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == FRSRT_PARAMETER )
		{
			std::string paramName;
			FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

			if ( paramDataSize > 0 )
			{


				if (paramType == FRSPT_UINT32_1)
				{
					fr_uint data_uint;
					if ( Read_Element_ParameterData(&data_uint,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
					if (paramDataSize == sizeof(fr_uint))
					{
						status = frContextSetParameter1u(context, paramName.c_str(), (fr_uint)data_uint);
						if (status != FR_SUCCESS) 
						{ 
							if ( paramName != "stacksize" ) // "stacksize" is read only. so it's expected to have an error
							{
								WarningDetected();   // this is minor error, we wont exit loader for that
							}
						}
					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				
				}
				else if (paramType == FRSPT_FLOAT1)
				{
					fr_float data_float;
					if ( Read_Element_ParameterData(&data_float,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == sizeof(fr_float))
					{

							status = frContextSetParameter1f(context, paramName.c_str(), data_float);
							if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that

					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (paramType == FRSPT_FLOAT2)
				{
					fr_float data_float[2];
					if ( Read_Element_ParameterData(data_float,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == 2*sizeof(fr_float))
					{
						status = frContextSetParameter3f(context, paramName.c_str(), data_float[0], data_float[1], 0.0);
						if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (paramType == FRSPT_FLOAT3)
				{
					fr_float data_float[3];
					if ( Read_Element_ParameterData(data_float,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == 3*sizeof(fr_float))
					{
						status = frContextSetParameter3f(context, paramName.c_str(), data_float[0], data_float[1], data_float[2]);
						if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (paramType == FRSPT_FLOAT4)
				{
					fr_float data_float[4];
					if ( Read_Element_ParameterData(data_float,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == 4*sizeof(fr_float))
					{
						status = frContextSetParameter4f(context, paramName.c_str(), data_float[0], data_float[1], data_float[2], data_float[3]);
						if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
					}
					else
					{
						FRS_MACRO_ERROR();
						return FR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if ( paramType == FRSPT_UNDEF && paramName == "FR_OBJECT_NAME" )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
					status = frObjectSetName(context,data_data);
					CHECK_STATUS_RETURNERROR;
					delete[] data_data; data_data = NULL;
				}


				// read only parameter. don't set it.
				else if (  (paramType == FRSPT_UNDEF && paramName == "gpu0name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu1name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu2name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu3name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu4name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu5name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu6name")
						|| (paramType == FRSPT_UNDEF && paramName == "gpu7name")
						|| (paramType == FRSPT_UNDEF && paramName == "cpuname")
					)
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
					delete[] data_data; data_data = NULL;
				}


				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

					WarningDetected();

					delete[] data_data; data_data = NULL;
				}

				
			}
			else
			{
				WarningDetected();
			}

		}
		else if ( nextElem == FRSRT_OBJECT_END )
		{
			break;
		}
		else
		{
			FRS_MACRO_ERROR();
			return FR_ERROR_INTERNAL_ERROR;
		}


	}


	if ( Read_Element_EndObject("fr_context",context,objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }

	return FR_SUCCESS;
}

fr_camera FRS7::Read_Camera(fr_context context)
{
	fr_int status = FR_SUCCESS;

	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
	if ( objType != "fr_camera" )
	{
		FRS_MACRO_ERROR();
		return NULL;
	}

	fr_camera camera = NULL;
	status = frContextCreateCamera(context,&camera);
	CHECK_STATUS_RETURNNULL;

	bool param_pos_declared = false;
	fr_float param_pos[3];
	bool param_at_declared = false;
	fr_float param_at[3];
	bool param_up_declared = false;
	fr_float param_up[3];

	while(true)
	{

		std::string elementName;
		std::string objBegType;
		FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == FRSRT_PARAMETER )
		{
			std::string paramName;
			FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

			if ( paramDataSize > 0 )
			{
		
				if ( paramName == "FR_CAMERA_FSTOP" && paramType == FRSPT_FLOAT1  )
				{
					float param_fstop;
					if ( Read_Element_ParameterData(&param_fstop,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetFStop(camera, param_fstop);
					CHECK_STATUS_RETURNNULL;
				}

				else if ( paramName == "FR_CAMERA_APERTURE_BLADES" && paramType == FRSPT_UINT32_1  )
				{
					unsigned int param_apblade;
					if ( Read_Element_ParameterData(&param_apblade,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetApertureBlades(camera, param_apblade);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "FR_CAMERA_EXPOSURE" && paramType == FRSPT_FLOAT1  )
				{
					float param_camexpo;
					if ( Read_Element_ParameterData(&param_camexpo,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetExposure(camera, param_camexpo);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "FR_CAMERA_FOCAL_LENGTH" && paramType == FRSPT_FLOAT1  )
				{
					float param_focalLen;
					if ( Read_Element_ParameterData(&param_focalLen,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetFocalLength(camera, param_focalLen);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "FR_CAMERA_SENSOR_SIZE" && paramType == FRSPT_FLOAT2  )
				{
					float param_sensorsize[2];
					if ( Read_Element_ParameterData(param_sensorsize,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetSensorSize(camera, param_sensorsize[0], param_sensorsize[1]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "FR_CAMERA_MODE" && paramType == FRSPT_UINT32_1  )
				{
					fr_camera_mode param_mode;
					if ( Read_Element_ParameterData(&param_mode,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetMode(camera, param_mode);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "FR_CAMERA_ORTHO_WIDTH" && paramType == FRSPT_FLOAT1  )
				{
					float param_orthowidth;
					if ( Read_Element_ParameterData(&param_orthowidth,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetOrthoWidth(camera, param_orthowidth);
					CHECK_STATUS_RETURNNULL;
				}
                else if (paramName == "FR_CAMERA_ORTHO_HEIGHT" && paramType == FRSPT_FLOAT1)
                {
                    float param_orthoheight;
                    if (Read_Element_ParameterData(&param_orthoheight, paramDataSize) != FR_SUCCESS) { FRS_MACRO_ERROR(); return NULL; }
                    status = frCameraSetOrthoHeight(camera, param_orthoheight);
                    CHECK_STATUS_RETURNNULL;
                }
				else if ( paramName == "FR_CAMERA_FOCUS_DISTANCE" && paramType == FRSPT_FLOAT1  )
				{
					float param_focusdist;
					if ( Read_Element_ParameterData(&param_focusdist,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frCameraSetFocusDistance(camera, param_focusdist);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "FR_CAMERA_POSITION" && paramType == FRSPT_FLOAT3  )
				{
					param_pos_declared = true;
					if ( Read_Element_ParameterData(param_pos,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}
				else if ( paramName == "FR_CAMERA_LOOKAT" && paramType == FRSPT_FLOAT3  )
				{
					param_at_declared = true;
					if ( Read_Element_ParameterData(param_at,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}
				else if ( paramName == "FR_CAMERA_UP" && paramType == FRSPT_FLOAT3  )
				{
					param_up_declared = true;
					if ( Read_Element_ParameterData(param_up,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}
				else if ( paramName == "FR_OBJECT_NAME" && paramType == FRSPT_UNDEF  )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frObjectSetName(camera,data_data);
					CHECK_STATUS_RETURNNULL;
					delete[] data_data; data_data = NULL;
				}
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					WarningDetected();
					delete[] data_data; data_data = NULL;
				}
				
			}
			else
			{
				WarningDetected();
			}

		}
		else if ( nextElem == FRSRT_OBJECT_END )
		{
			break;
		}
		else
		{
			FRS_MACRO_ERROR();
			return NULL;
		}

	}

	if ( param_pos_declared && param_at_declared && param_up_declared )
	{
		status = frCameraLookAt(camera, 
			param_pos[0],param_pos[1],param_pos[2],
			param_at[0],param_at[1],param_at[2],
			param_up[0],param_up[1],param_up[2]
			);
		CHECK_STATUS_RETURNNULL;
	}
	else
	{
		WarningDetected();
	}

	if ( Read_Element_EndObject("fr_camera",camera,objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

	return camera;
}

fr_light FRS7::Read_Light(fr_context context, fr_scene scene, fr_material_system materialSystem)
{
	fr_int status = FR_SUCCESS;


	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
	if ( objType != "fr_light" )
	{
		FRS_MACRO_ERROR();
		return NULL;
	}

	fr_light light = NULL;

	fr_light_type lightType = 0;
	bool param__FR_IES_LIGHT_IMAGE_DESC_W__defined = false;
	int32_t param__FR_IES_LIGHT_IMAGE_DESC_W__data = 0;
	bool param__FR_IES_LIGHT_IMAGE_DESC_H__defined = false;
	int32_t param__FR_IES_LIGHT_IMAGE_DESC_H__data = 0;
	bool param__FR_IES_LIGHT_IMAGE_DESC_DATA__defined = false;
	char* param__FR_IES_LIGHT_IMAGE_DESC_DATA__data = NULL;
	bool param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined = false;
	uint64_t param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data = 0;
	int64_t param__EnvlightPortalCount = 0;
	int64_t param__SkylightPortalCount = 0;

	while(true)
	{

		std::string elementName;
		std::string objBegType;
		FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == FRSRT_PARAMETER )
		{
			std::string paramName;
			FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

			if ( paramDataSize > 0 )
			{
			

				if (light == NULL && paramType == FRSPT_UNDEF && paramDataSize == sizeof(fr_light_type) && paramName == "FR_LIGHT_TYPE")
				{
					if ( Read_Element_ParameterData(&lightType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

					if ( lightType == FR_LIGHT_TYPE_POINT )
					{
						status = frContextCreatePointLight(context, &light);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( lightType == FR_LIGHT_TYPE_DIRECTIONAL )
					{
						status = frContextCreateDirectionalLight(context, &light);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( lightType == FR_LIGHT_TYPE_SPOT )
					{
						status = frContextCreateSpotLight(context, &light);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( lightType == FR_LIGHT_TYPE_ENVIRONMENT )
					{
						status = frContextCreateEnvironmentLight(context, &light);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( lightType == FR_LIGHT_TYPE_SKY )
					{
						status = frContextCreateSkyLight(context, &light);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( lightType == FR_LIGHT_TYPE_IES )
					{
						status = frContextCreateIESLight(context, &light);
						CHECK_STATUS_RETURNNULL;
					}
					else
					{
						FRS_MACRO_ERROR();
						return NULL;
					}
				}
				else if ( light != NULL && paramName == "FR_POINT_LIGHT_RADIANT_POWER" && paramType == FRSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
				{
					float radian3f[3];
					if ( Read_Element_ParameterData(radian3f,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frPointLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_DIRECTIONAL_LIGHT_RADIANT_POWER" && paramType == FRSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
				{
					float radian3f[3];
					if ( Read_Element_ParameterData(radian3f,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frDirectionalLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_SPOT_LIGHT_RADIANT_POWER" && paramType == FRSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
				{
					float radian3f[3];
					if ( Read_Element_ParameterData(radian3f,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frSpotLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_IES_LIGHT_RADIANT_POWER" && paramType == FRSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
				{
					float radian3f[3];
					if ( Read_Element_ParameterData(radian3f,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frIESLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_SPOT_LIGHT_CONE_SHAPE" && paramType == FRSPT_FLOAT2 && paramDataSize == sizeof(float)*2 )
				{
					float coneShape2f[2];
					if ( Read_Element_ParameterData(coneShape2f,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frSpotLightSetConeShape(light, coneShape2f[0], coneShape2f[1]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_ENVIRONMENT_LIGHT_INTENSITY_SCALE" && paramType == FRSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
				{
					float intensityScale;
					if ( Read_Element_ParameterData(&intensityScale,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frEnvironmentLightSetIntensityScale(light, intensityScale);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "lightIsTheEnvironmentLight" && paramType == FRSPT_INT32_1 && paramDataSize == sizeof(int32_t) )
				{
					//parameter not used anymore because  frSceneSetEnvironmentLight replaced by frSceneAttachLight

					int32_t isEnvLight;
					if ( Read_Element_ParameterData(&isEnvLight,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					if ( isEnvLight == 1 )
					{
						//status = frSceneSetEnvironmentLight(scene, light);
						CHECK_STATUS_RETURNNULL;
					}	
				}
				else if ( light != NULL && paramName == "FR_LIGHT_TRANSFORM" && paramType == FRSPT_FLOAT16 && paramDataSize == sizeof(float)*16 )
				{
					float lightTransform[16];
					if ( Read_Element_ParameterData(lightTransform,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frLightSetTransform(light, 0, (float*)lightTransform);
					CHECK_STATUS_RETURNNULL;
				}

				else if ( light != NULL && paramName == "FR_SKY_LIGHT_SCALE" && paramType == FRSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
				{
					float scale;
					if ( Read_Element_ParameterData(&scale,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frSkyLightSetScale(light, scale);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_SKY_LIGHT_ALBEDO" && paramType == FRSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
				{
					float albedo;
					if ( Read_Element_ParameterData(&albedo,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frSkyLightSetAlbedo(light, albedo);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( light != NULL && paramName == "FR_SKY_LIGHT_TURBIDITY" && paramType == FRSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
				{
					float turbidity;
					if ( Read_Element_ParameterData(&turbidity,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frSkyLightSetTurbidity(light, turbidity);
					CHECK_STATUS_RETURNNULL;
				}

				else if ( light != NULL && paramName == "FR_IES_LIGHT_IMAGE_DESC_W" && paramType == FRSPT_INT32_1 && paramDataSize == sizeof(int32_t) )
				{
					param__FR_IES_LIGHT_IMAGE_DESC_W__defined = true;
					if ( Read_Element_ParameterData(&param__FR_IES_LIGHT_IMAGE_DESC_W__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}
				else if ( light != NULL && paramName == "FR_IES_LIGHT_IMAGE_DESC_H" && paramType == FRSPT_INT32_1 && paramDataSize == sizeof(int32_t) )
				{
					param__FR_IES_LIGHT_IMAGE_DESC_H__defined = true;
					if ( Read_Element_ParameterData(&param__FR_IES_LIGHT_IMAGE_DESC_H__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}
				else if ( light != NULL && paramName == "FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE" && paramType == FRSPT_UINT64_1 && paramDataSize == sizeof(uint64_t) )
				{
					param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined = true;
					if ( Read_Element_ParameterData(&param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}
				else if ( light != NULL && paramName == "FR_IES_LIGHT_IMAGE_DESC_DATA" && paramType == FRSPT_UNDEF && paramDataSize == param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data && param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined )
				{
					param__FR_IES_LIGHT_IMAGE_DESC_DATA__defined = true;
					param__FR_IES_LIGHT_IMAGE_DESC_DATA__data = new char[param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data];
					if ( Read_Element_ParameterData(param__FR_IES_LIGHT_IMAGE_DESC_DATA__data,param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}

				else if ( light != NULL && paramName == "FR_OBJECT_NAME" && paramType == FRSPT_UNDEF  )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					status = frObjectSetName(light,data_data);
					CHECK_STATUS_RETURNNULL;
					delete[] data_data; data_data = NULL;
				}

				else if ( light != NULL && paramName == "FR_ENVIRONMENT_LIGHT_PORTAL_COUNT" && paramType == FRSPT_INT64_1  && paramDataSize == sizeof(int64_t) )
				{
					if ( Read_Element_ParameterData(&param__EnvlightPortalCount,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}

				else if ( light != NULL && paramName == "FR_SKY_LIGHT_PORTAL_COUNT" && paramType == FRSPT_INT64_1  && paramDataSize == sizeof(int64_t) )
				{
					if ( Read_Element_ParameterData(&param__SkylightPortalCount,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
				}

				else if ( light == NULL )
				{
					// "FR_LIGHT_TYPE" must be declared as the first parameter in the parameters list of the light
					FRS_MACRO_ERROR();
					return NULL;
				}
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

					WarningDetected();

					delete[] data_data; data_data = NULL;
				}
				



			}
			else
			{
				WarningDetected();
			}

		}
		else if ( (nextElem == FRSRT_OBJECT_BEG && elementName == "FR_ENVIRONMENT_LIGHT_IMAGE" && objBegType == "fr_image")
			||    (nextElem == FRSRT_REFERENCE && elementName == "FR_ENVIRONMENT_LIGHT_IMAGE")
			)
		{
			fr_image img = Read_Image(context);
			status = frEnvironmentLightSetImage(light, img);
			CHECK_STATUS_RETURNNULL;
		}
		else if ( (nextElem == FRSRT_OBJECT_BEG && elementName == STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID && objBegType == "fr_shape")
			||    (nextElem == FRSRT_REFERENCE && elementName == STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID)
			)
		{
			fr_shape shape = Read_Shape(context,materialSystem);
			if ( shape == NULL ) { FRS_MACRO_ERROR(); return NULL; }
			status = frSkyLightAttachPortal(light, shape);
			CHECK_STATUS_RETURNNULL;
		}
		else if ( (nextElem == FRSRT_OBJECT_BEG && elementName == STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID && objBegType == "fr_shape")
			||    (nextElem == FRSRT_REFERENCE && elementName == STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID)
			)
		{
			fr_shape shape = Read_Shape(context,materialSystem);
			if ( shape == NULL ) { FRS_MACRO_ERROR(); return NULL; }
			status = frEnvironmentLightAttachPortal(light, shape);
			CHECK_STATUS_RETURNNULL;
		}
		else if ( nextElem == FRSRT_OBJECT_END )
		{
			break;
		}
		else
		{
			FRS_MACRO_ERROR();
			return NULL;
		}

	}

	if ( Read_Element_EndObject("fr_light",light,objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }


	//set parameters for IES
	if ( lightType == FR_LIGHT_TYPE_IES 
		&& param__FR_IES_LIGHT_IMAGE_DESC_W__defined
		&& param__FR_IES_LIGHT_IMAGE_DESC_H__defined
		&& param__FR_IES_LIGHT_IMAGE_DESC_DATA__defined
		&& param__FR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined )
	{
		status = frIESLightSetImageFromIESdata(light, param__FR_IES_LIGHT_IMAGE_DESC_DATA__data, param__FR_IES_LIGHT_IMAGE_DESC_W__data, param__FR_IES_LIGHT_IMAGE_DESC_H__data);
		if (status != FR_SUCCESS) { WarningDetected(); }
	}



	return light;
}



fr_image FRS7::Read_Image(fr_context context)
{
	fr_image image = NULL;
	fr_int status = FR_SUCCESS;

	std::string elementName;
	std::string objBegType;
	FRS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "fr_image" )
	{
		FRS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == FRSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
		image = objReferenced.obj;
	}
	else if ( elementType == FRSRT_OBJECT_BEG )
	{
		int32_t objID = 0;
		std::string objType;
		Read_Element_StartObject(elementName,objType,objID);
		
		fr_image_format imgFormat;
		memset(&imgFormat,0,sizeof(imgFormat));
		fr_image_desc imgDesc;
		memset(&imgDesc,0,sizeof(imgDesc));
		void* imgData = NULL;
		char* objectName = NULL;

		while(true)
		{
			std::string elementName;
			std::string objBegType;
			FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == FRSRT_PARAMETER )
			{
				std::string paramName;
				FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
					//char* data_data = new char[paramDataSize];
					
					if (paramName == "FR_IMAGE_FORMAT" && paramType == FRSPT_UNDEF && paramDataSize == sizeof(fr_image_format))
					{
						//imgFormat = ((fr_image_format*)data_data)[0];
						if ( Read_Element_ParameterData(&imgFormat,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_IMAGE_DESC" && paramType == FRSPT_UNDEF && paramDataSize == sizeof(fr_image_desc))
					{
						//imgDesc = ((fr_image_desc*)data_data)[0];
						if ( Read_Element_ParameterData(&imgDesc,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_IMAGE_DATA" && paramType == FRSPT_UNDEF )
					{
						imgData = new char[paramDataSize];
						if ( Read_Element_ParameterData(imgData,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_OBJECT_NAME" && paramType == FRSPT_UNDEF )
					{
						objectName = new char[paramDataSize];
						if ( Read_Element_ParameterData(objectName,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else
					{
						FRS_MACRO_ERROR(); return NULL;
					}
					
					//delete[] data_data; data_data = NULL;
				}

			}
			else if ( nextElem == FRSRT_OBJECT_END ) // if reached end of file
			{
				break;
			}
			else
			{
				FRS_MACRO_ERROR();
				return NULL;
			}
		}

		if ( imgData == NULL )
		{
			FRS_MACRO_ERROR();
			return NULL;
		}
		status = frContextCreateImage(context, imgFormat, &imgDesc, imgData, &image);
		if ( imgData ) { delete[] static_cast<char*>(imgData); imgData=NULL; }
		CHECK_STATUS_RETURNNULL;

		if ( objectName )
		{
			status = frObjectSetName(image,objectName);
			CHECK_STATUS_RETURNNULL;
			delete[] objectName; objectName=NULL;
		}

		Read_Element_EndObject("fr_image",image,objID);
	}
	else
	{
		FRS_MACRO_ERROR();
		return NULL;
	}

	return image;
}


fr_material_node FRS7::Read_MaterialNode(fr_material_system materialSystem, fr_context context)
{
	fr_material_node material = NULL;
	fr_int status = FR_SUCCESS;
	
	std::string elementName;

	std::string objBegType;
	FRS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "fr_material_node" )
	{
		FRS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == FRSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
		material = objReferenced.obj;
	}
	else if ( elementType == FRSRT_OBJECT_BEG )
	{


		std::string objType;
		int32_t materialID = 0;
		if ( Read_Element_StartObject(elementName,objType,materialID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
		if ( objType != "fr_material_node" )
		{
			FRS_MACRO_ERROR();
			return NULL;
		}


		while(true)
		{

			std::string elementName;
			std::string objBegType;
			FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == FRSRT_PARAMETER )
			{
				std::string paramName;
				FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
				
				
					if (paramName == "FR_MATERIAL_NODE_TYPE" && paramType == FRSPT_UINT32_1 && paramDataSize == sizeof(fr_material_node_type) )
					{
						fr_material_node_type type = 0;
						if ( Read_Element_ParameterData(&type,sizeof(fr_material_node_type))  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
						status = frMaterialSystemCreateNode(materialSystem, type, &material);
						CHECK_STATUS_RETURNNULL;
					}
					else if (paramName == "FR_MATERIAL_NODE_INPUT_COUNT" && paramType == FRSPT_UINT64_1 && paramDataSize == sizeof(uint64_t) )
					{
						uint64_t nbInput = 0;
						if ( Read_Element_ParameterData(&nbInput,sizeof(nbInput))  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else
					{
						if ( paramType == FRSPT_FLOAT4 )
						{
							float paramValue[4];
							if ( Read_Element_ParameterData(paramValue,sizeof(paramValue))  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
							status = frMaterialNodeSetInputF(material,paramName.c_str(),paramValue[0],paramValue[1],paramValue[2],paramValue[3]);
							if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
						}
						else if ( paramType == FRSPT_UINT32_1 )
						{
							uint32_t paramValue;
							if ( Read_Element_ParameterData(&paramValue,sizeof(paramValue))  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
							status = frMaterialNodeSetInputU(material,paramName.c_str(),paramValue);
							if (status != FR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
						}
						else if ( paramType == FRSPT_UNDEF && paramName == "FR_OBJECT_NAME" )
						{
							char* data_data = new char[paramDataSize];
							if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
							status = frObjectSetName(material,data_data);
							CHECK_STATUS_RETURNNULL;
							delete[] data_data; data_data = NULL;
						}
						else
						{
							//unmanaged parameter
							char* data_data = new char[paramDataSize];
							if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

							WarningDetected();

							delete[] data_data; data_data = NULL;
						}
					}
					
				
				}

			}
			else if ( (nextElem == FRSRT_OBJECT_BEG && objBegType == "fr_material_node") 
				||    (nextElem == FRSRT_REFERENCE && objBegType == "fr_material_node")
				)
			{
			
				fr_material_node matNode = Read_MaterialNode(materialSystem,context);
				if ( matNode == NULL )
				{
					FRS_MACRO_ERROR();
					return NULL;
				}
				status = frMaterialNodeSetInputN(material,elementName.c_str(),matNode);
				CHECK_STATUS_RETURNNULL;

			}
			else if ( (nextElem == FRSRT_OBJECT_BEG && objBegType == "fr_image")
				||    (nextElem == FRSRT_REFERENCE && objBegType == "fr_image")
				)
			{
			
				fr_image image = Read_Image(context);
				if ( image == NULL )
				{
					FRS_MACRO_ERROR();
					return NULL;
				}
				status = frMaterialNodeSetInputImageData(material,elementName.c_str(),image);
				CHECK_STATUS_RETURNNULL;

			}
			else if ( nextElem == FRSRT_OBJECT_END )
			{
				break;
			}
			else
			{
				FRS_MACRO_ERROR();
				return NULL;
			}

		}



		if ( Read_Element_EndObject("fr_material_node",material,materialID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }





	}
	else
	{
		FRS_MACRO_ERROR();
		return NULL;
	}

	return material;
}

fr_shape FRS7::Read_Shape(fr_context context, fr_material_system materialSystem )
{
	fr_int status = FR_SUCCESS;
	fr_shape shape = NULL;

	std::string elementName;
	std::string objBegType;
	FRS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "fr_shape" )
	{
		FRS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == FRSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
		shape = objReferenced.obj;
	}
	else if ( elementType == FRSRT_OBJECT_BEG )
	{
		std::string elementName;
		std::string objType;
		int32_t objID = 0;
		Read_Element_StartObject(elementName,objType,objID);
		if ( objType != "fr_shape" )
		{
			FRS_MACRO_ERROR(); 
			return NULL;
		}


		bool          param__FR_SHAPE_TYPE__defined = false;
		fr_shape_type param__FR_SHAPE_TYPE__data = NULL;
		bool          param__FR_MESH_POLYGON_COUNT__defined = false;
		uint64_t param__FR_MESH_POLYGON_COUNT__data = NULL;
		bool          param__FR_MESH_VERTEX_COUNT__defined = false;
		uint64_t param__FR_MESH_VERTEX_COUNT__data = NULL;
		bool          param__FR_MESH_NORMAL_COUNT__defined = false;
		uint64_t param__FR_MESH_NORMAL_COUNT__data = NULL;
		bool          param__FR_MESH_UV_COUNT__defined = false;
		uint64_t param__FR_MESH_UV_COUNT__data = NULL;

		bool          param__FR_MESH_VERTEX_ARRAY__defined = false;
		float* param__FR_MESH_VERTEX_ARRAY__data = NULL;
		int32_t param__FR_MESH_VERTEX_ARRAY__dataSize = 0;
		bool          param__FR_MESH_NORMAL_ARRAY__defined = false;
		float* param__FR_MESH_NORMAL_ARRAY__data = NULL;
		int32_t param__FR_MESH_NORMAL_ARRAY__dataSize = 0;
		bool          param__FR_MESH_UV_ARRAY__defined = false;
		float* param__FR_MESH_UV_ARRAY__data = NULL;
		int32_t param__FR_MESH_UV_ARRAY__dataSize = 0;

		bool          param__FR_MESH_VERTEX_INDEX_ARRAY__defined = false;
		int32_t* param__FR_MESH_VERTEX_INDEX_ARRAY__data = NULL;
		bool          param__FR_MESH_NORMAL_INDEX_ARRAY__defined = false;
		int32_t* param__FR_MESH_NORMAL_INDEX_ARRAY__data = NULL;
		bool          param__FR_MESH_UV_INDEX_ARRAY__defined = false;
		int32_t* param__FR_MESH_UV_INDEX_ARRAY__data = NULL;
		bool          param__FR_MESH_NUM_VERTICES_ARRAY__defined = false;
		int32_t* param__FR_MESH_NUM_VERTICES_ARRAY__data = NULL;
		bool          param__SHAPE_INSTANCE_REFERENCE_ID__defined = false;
		int32_t param__SHAPE_INSTANCE_REFERENCE_ID__data = NULL;
		bool          param__FR_SHAPE_TRANSFORM__defined = false;
		float param__FR_SHAPE_TRANSFORM__data[16];
		bool          param__FR_SHAPE_LINEAR_MOTION__defined = false;
		float param__FR_SHAPE_LINEAR_MOTION__data[3];
		bool          param__FR_SHAPE_ANGULAR_MOTION__defined = false;
		float param__FR_SHAPE_ANGULAR_MOTION__data[4];
		bool          param__FR_SHAPE_VISIBILITY__defined = false;
		fr_bool param__FR_SHAPE_VISIBILITY__data = NULL;
		bool          param__FR_SHAPE_SHADOW__defined = false;
		fr_bool param__FR_SHAPE_SHADOW__data = NULL;
		bool          param__FR_SHAPE_SHADOWCATCHER__defined = false;
		fr_bool param__FR_SHAPE_SHADOWCATCHER__data = NULL;

		bool          param__FR_SHAPE_SUBDIVISION_FACTOR__defined = false;
		fr_uint param__FR_SHAPE_SUBDIVISION_FACTOR__data = 0;
		bool          param__FR_SHAPE_DISPLACEMENT_IMAGE__defined = false;
		fr_image param__FR_SHAPE_DISPLACEMENT_IMAGE__data = NULL;
		bool          param__FR_SHAPE_DISPLACEMENT_SCALE__defined = false;
		fr_float param__FR_SHAPE_DISPLACEMENT_SCALE__data[2] = {0.0f,0.0f};

		fr_material_node shapeMaterial = NULL;

		char* objectName = NULL;

		while(true)
		{
			std::string elementName;
			std::string objBegType;
			FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == FRSRT_PARAMETER )
			{
				std::string paramName;
				FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
				
					if (paramName == "FR_SHAPE_TYPE" && paramType == FRSPT_UNDEF )
					{
						param__FR_SHAPE_TYPE__defined = true;
						if ( Read_Element_ParameterData(&param__FR_SHAPE_TYPE__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_POLYGON_COUNT" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_POLYGON_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__FR_MESH_POLYGON_COUNT__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_VERTEX_COUNT" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_VERTEX_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__FR_MESH_VERTEX_COUNT__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_NORMAL_COUNT" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_NORMAL_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__FR_MESH_NORMAL_COUNT__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_UV_COUNT" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_UV_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__FR_MESH_UV_COUNT__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_VERTEX_ARRAY" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_VERTEX_ARRAY__data = (float*)malloc(paramDataSize);
						param__FR_MESH_VERTEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_VERTEX_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
						param__FR_MESH_VERTEX_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "FR_MESH_NORMAL_ARRAY" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_NORMAL_ARRAY__data = (float*)malloc(paramDataSize);
						param__FR_MESH_NORMAL_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_NORMAL_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
						param__FR_MESH_NORMAL_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "FR_MESH_UV_ARRAY" && paramType == FRSPT_UINT64_1 )
					{
						param__FR_MESH_UV_ARRAY__data = (float*)malloc(paramDataSize);
						param__FR_MESH_UV_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_UV_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
						param__FR_MESH_UV_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "FR_MESH_VERTEX_INDEX_ARRAY" && paramType == FRSPT_UNDEF )
					{
						param__FR_MESH_VERTEX_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__FR_MESH_VERTEX_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_VERTEX_INDEX_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_NORMAL_INDEX_ARRAY" && paramType == FRSPT_UNDEF )
					{
						param__FR_MESH_NORMAL_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__FR_MESH_NORMAL_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_NORMAL_INDEX_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_UV_INDEX_ARRAY" && paramType == FRSPT_UNDEF )
					{
						param__FR_MESH_UV_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__FR_MESH_UV_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_UV_INDEX_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_MESH_NUM_VERTICES_ARRAY" && paramType == FRSPT_UNDEF )
					{
						param__FR_MESH_NUM_VERTICES_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__FR_MESH_NUM_VERTICES_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__FR_MESH_NUM_VERTICES_ARRAY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == STR__SHAPE_INSTANCE_REFERENCE_ID && paramType == FRSPT_INT32_1 )
					{
						param__SHAPE_INSTANCE_REFERENCE_ID__defined = true;
						if ( Read_Element_ParameterData(&param__SHAPE_INSTANCE_REFERENCE_ID__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_TRANSFORM" && paramType == FRSPT_FLOAT16 )
					{
						param__FR_SHAPE_TRANSFORM__defined = true;
						if ( Read_Element_ParameterData(param__FR_SHAPE_TRANSFORM__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_LINEAR_MOTION" && paramType == FRSPT_FLOAT3 )
					{
						param__FR_SHAPE_LINEAR_MOTION__defined = true;
						if ( Read_Element_ParameterData(param__FR_SHAPE_LINEAR_MOTION__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_ANGULAR_MOTION" && paramType == FRSPT_FLOAT4 )
					{
						param__FR_SHAPE_ANGULAR_MOTION__defined = true;
						if ( Read_Element_ParameterData(param__FR_SHAPE_ANGULAR_MOTION__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_VISIBILITY" && paramType == FRSPT_INT32_1 )
					{
						param__FR_SHAPE_VISIBILITY__defined = true;
						if ( Read_Element_ParameterData(&param__FR_SHAPE_VISIBILITY__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_SHADOW" && paramType == FRSPT_INT32_1 )
					{
						param__FR_SHAPE_SHADOW__defined = true;
						if ( Read_Element_ParameterData(&param__FR_SHAPE_SHADOW__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_SHADOW_CATCHER" && paramType == FRSPT_INT32_1 )
					{
						param__FR_SHAPE_SHADOWCATCHER__defined = true;
						if ( Read_Element_ParameterData(&param__FR_SHAPE_SHADOWCATCHER__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_DISPLACEMENT_SCALE" && paramType == FRSPT_FLOAT2 )
					{
						param__FR_SHAPE_DISPLACEMENT_SCALE__defined = true;
						if ( Read_Element_ParameterData(param__FR_SHAPE_DISPLACEMENT_SCALE__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_SHAPE_SUBDIVISION_FACTOR" && paramType == FRSPT_UINT32_1 )
					{
						param__FR_SHAPE_SUBDIVISION_FACTOR__defined = true;
						if ( Read_Element_ParameterData(&param__FR_SHAPE_SUBDIVISION_FACTOR__data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "FR_OBJECT_NAME" && paramType == FRSPT_UNDEF )
					{
						objectName = new char[paramDataSize];
						if ( Read_Element_ParameterData(objectName,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
					}
					else
					{
						char* data_data = new char[paramDataSize];
						if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

						WarningDetected();

						delete[] data_data; data_data = NULL;
					}
					
				
				}

			}
			else if ( (nextElem == FRSRT_OBJECT_BEG && elementName == "FR_SHAPE_DISPLACEMENT_IMAGE" && objBegType == "fr_image")
				||    (nextElem == FRSRT_REFERENCE && elementName == "FR_SHAPE_DISPLACEMENT_IMAGE") 
				)
			{
				param__FR_SHAPE_DISPLACEMENT_IMAGE__data = Read_Image(context);
				param__FR_SHAPE_DISPLACEMENT_IMAGE__defined = true;
			}
			else if ( (nextElem == FRSRT_OBJECT_BEG && objBegType == "fr_material_node")
				||    (nextElem == FRSRT_REFERENCE  && objBegType == "fr_material_node")
				)
			{
				shapeMaterial = Read_MaterialNode(materialSystem,context);
				if ( shapeMaterial == NULL )
				{
					FRS_MACRO_ERROR();
					return NULL;
				}
			}
			else if ( nextElem == FRSRT_OBJECT_END )
			{
				break;
			}
			else
			{
				FRS_MACRO_ERROR();
				return NULL;
			}
		}


		if ( !param__FR_SHAPE_TYPE__defined ) { FRS_MACRO_ERROR(); return NULL; }
		if ( param__FR_SHAPE_TYPE__data == FR_SHAPE_TYPE_MESH )
		{
			//special case : no UV data
			if (    param__FR_MESH_UV_COUNT__defined && param__FR_MESH_UV_COUNT__data == 0
				&&  !param__FR_MESH_UV_ARRAY__defined
				&&  !param__FR_MESH_UV_INDEX_ARRAY__defined
				)
			{
				param__FR_MESH_UV_ARRAY__defined = true;
				param__FR_MESH_UV_ARRAY__data = nullptr;
				param__FR_MESH_UV_ARRAY__dataSize = 0;
				param__FR_MESH_UV_INDEX_ARRAY__defined = true;
				param__FR_MESH_UV_INDEX_ARRAY__data = nullptr;
			}


			if (    !param__FR_MESH_POLYGON_COUNT__defined
				||  !param__FR_MESH_VERTEX_COUNT__defined
				||  !param__FR_MESH_NORMAL_COUNT__defined
				||  !param__FR_MESH_UV_COUNT__defined
				||  !param__FR_MESH_VERTEX_ARRAY__defined
				||  !param__FR_MESH_NORMAL_ARRAY__defined
				||  !param__FR_MESH_UV_ARRAY__defined
				||  !param__FR_MESH_VERTEX_INDEX_ARRAY__defined
				||  !param__FR_MESH_NORMAL_INDEX_ARRAY__defined
				||  !param__FR_MESH_UV_INDEX_ARRAY__defined
				||  !param__FR_MESH_NUM_VERTICES_ARRAY__defined
				) 
			{ FRS_MACRO_ERROR(); return NULL; }

			shape = NULL;
			status = frContextCreateMesh(context,
				param__FR_MESH_VERTEX_ARRAY__data, param__FR_MESH_VERTEX_COUNT__data, int(param__FR_MESH_VERTEX_ARRAY__dataSize / param__FR_MESH_VERTEX_COUNT__data),
				param__FR_MESH_NORMAL_ARRAY__data, param__FR_MESH_NORMAL_COUNT__data, int(param__FR_MESH_NORMAL_ARRAY__dataSize / param__FR_MESH_NORMAL_COUNT__data),
				param__FR_MESH_UV_ARRAY__data	 , param__FR_MESH_UV_COUNT__data, param__FR_MESH_UV_ARRAY__dataSize == 0 ? 0 : int(param__FR_MESH_UV_ARRAY__dataSize / param__FR_MESH_UV_COUNT__data),
				param__FR_MESH_VERTEX_INDEX_ARRAY__data, sizeof(fr_int),
				param__FR_MESH_NORMAL_INDEX_ARRAY__data, sizeof(fr_int),
				param__FR_MESH_UV_INDEX_ARRAY__data, sizeof(fr_int),
				param__FR_MESH_NUM_VERTICES_ARRAY__data, param__FR_MESH_POLYGON_COUNT__data, &shape);
			CHECK_STATUS_RETURNNULL;
		}
		else if ( param__FR_SHAPE_TYPE__data == FR_SHAPE_TYPE_INSTANCE )
		{
			if ( !param__SHAPE_INSTANCE_REFERENCE_ID__defined ) { FRS_MACRO_ERROR(); return NULL; }

			//search the id in the previous loaded
			fr_shape shapeReferenced = NULL;
			for(int iObj=0; iObj<m_listObjectDeclared.size(); iObj++)
			{
				if ( m_listObjectDeclared[iObj].id == param__SHAPE_INSTANCE_REFERENCE_ID__data )
				{
					shapeReferenced = m_listObjectDeclared[iObj].obj;
					break;
				}
			}

			if ( shapeReferenced == NULL ) { FRS_MACRO_ERROR(); return NULL; }

			shape = NULL;
			status = frContextCreateInstance(context,
				shapeReferenced,
				&shape);
			CHECK_STATUS_RETURNNULL;
		}
		else
		{
			FRS_MACRO_ERROR();
			return NULL;
		}


		if ( param__FR_SHAPE_TRANSFORM__defined )
		{
			status = frShapeSetTransform(shape,false,param__FR_SHAPE_TRANSFORM__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_LINEAR_MOTION__defined )
		{
			status = frShapeSetLinearMotion(shape,param__FR_SHAPE_LINEAR_MOTION__data[0],param__FR_SHAPE_LINEAR_MOTION__data[1],param__FR_SHAPE_LINEAR_MOTION__data[2]);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_ANGULAR_MOTION__defined )
		{
			status = frShapeSetAngularMotion(shape,param__FR_SHAPE_ANGULAR_MOTION__data[0],param__FR_SHAPE_ANGULAR_MOTION__data[1],param__FR_SHAPE_ANGULAR_MOTION__data[2],param__FR_SHAPE_ANGULAR_MOTION__data[3]);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_VISIBILITY__defined )
		{
			status = frShapeSetVisibility(shape,param__FR_SHAPE_VISIBILITY__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_SHADOW__defined )
		{
			status = frShapeSetShadow(shape,param__FR_SHAPE_SHADOW__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_SHADOWCATCHER__defined )
		{
			status = frShapeSetShadowCatcher(shape,param__FR_SHAPE_SHADOWCATCHER__data);
			CHECK_STATUS_RETURNNULL;
		}

		if ( param__FR_SHAPE_SUBDIVISION_FACTOR__defined )
		{
			status = frShapeSetSubdivisionFactor(shape,param__FR_SHAPE_SUBDIVISION_FACTOR__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_DISPLACEMENT_IMAGE__defined )
		{
			status = frShapeSetDisplacementImage(shape,param__FR_SHAPE_DISPLACEMENT_IMAGE__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__FR_SHAPE_DISPLACEMENT_SCALE__defined )
		{
			status = frShapeSetDisplacementScale(shape,param__FR_SHAPE_DISPLACEMENT_SCALE__data[0],param__FR_SHAPE_DISPLACEMENT_SCALE__data[1]);
			CHECK_STATUS_RETURNNULL;
		}



		if ( param__FR_MESH_VERTEX_ARRAY__data) { free(param__FR_MESH_VERTEX_ARRAY__data); param__FR_MESH_VERTEX_ARRAY__data=NULL; }
		if ( param__FR_MESH_NORMAL_ARRAY__data) { free(param__FR_MESH_NORMAL_ARRAY__data); param__FR_MESH_NORMAL_ARRAY__data=NULL; }
		if ( param__FR_MESH_UV_ARRAY__data) { free(param__FR_MESH_UV_ARRAY__data); param__FR_MESH_UV_ARRAY__data=NULL; }
		if ( param__FR_MESH_VERTEX_INDEX_ARRAY__data) { free(param__FR_MESH_VERTEX_INDEX_ARRAY__data); param__FR_MESH_VERTEX_INDEX_ARRAY__data=NULL; }
		if ( param__FR_MESH_NORMAL_INDEX_ARRAY__data) { free(param__FR_MESH_NORMAL_INDEX_ARRAY__data); param__FR_MESH_NORMAL_INDEX_ARRAY__data=NULL; }
		if ( param__FR_MESH_UV_INDEX_ARRAY__data) { free(param__FR_MESH_UV_INDEX_ARRAY__data); param__FR_MESH_UV_INDEX_ARRAY__data=NULL; }
		if ( param__FR_MESH_NUM_VERTICES_ARRAY__data) { free(param__FR_MESH_NUM_VERTICES_ARRAY__data); param__FR_MESH_NUM_VERTICES_ARRAY__data=NULL; }


		if ( shapeMaterial )
		{
			status = frShapeSetMaterial(shape,shapeMaterial);
			CHECK_STATUS_RETURNNULL
		}

		if ( objectName )
		{
			frObjectSetName(shape,objectName);
			delete[] objectName; objectName=NULL;
		}

		Read_Element_EndObject("fr_shape",shape,objID);

	}
	else
	{
		FRS_MACRO_ERROR();
		return NULL;
	}

	return shape;
}

fr_scene FRS7::Read_Scene(fr_context context, fr_material_system materialSystem, fr_scene sceneExisting)
{
	fr_int status = FR_SUCCESS;

	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName,objType,objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
	if ( objType != "fr_scene" )
	{
		FRS_MACRO_ERROR(); 
		return NULL;
	}


	fr_scene scene = NULL;
	if ( sceneExisting == NULL )
	{
		scene = NULL;
		status = frContextCreateScene(context, &scene);
	}
	else
	{
		scene = sceneExisting;
	}
	CHECK_STATUS_RETURNNULL;

	
	while(true)
	{
		std::string elementName;
		std::string objBegType;
		FRS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == FRSRT_OBJECT_BEG )
		{
			if ( objBegType == "fr_light" )
			{
				fr_light light = Read_Light(context,scene,materialSystem);
				if ( light == NULL ) { FRS_MACRO_ERROR(); return NULL; }
				status = frSceneAttachLight(scene, light);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( objBegType == "fr_shape" )
			{
				fr_shape shape = Read_Shape(context, materialSystem );
				if ( shape == NULL ) { FRS_MACRO_ERROR(); return NULL; }
				status = frSceneAttachShape(scene, shape);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( objBegType == "fr_camera" )
			{
				fr_camera camera = Read_Camera(context ) ;
				if ( camera == NULL ) { FRS_MACRO_ERROR(); return NULL; }
				status = frSceneSetCamera(scene, camera);
				CHECK_STATUS_RETURNNULL;
			}
			else
			{
				//unkonwn object name
				FRS_MACRO_ERROR(); return NULL;
			}

		}
		else if ( nextElem == FRSRT_PARAMETER && elementName == "FR_OBJECT_NAME" )
		{
			std::string paramName;
			FRS_PARAMETER_TYPE paramType = FRSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

			char* data_data = new char[paramDataSize];
			if ( Read_Element_ParameterData(data_data,paramDataSize)  != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }
			status = frObjectSetName(scene,data_data);
			CHECK_STATUS_RETURNNULL;
			delete[] data_data; data_data = NULL;
		}
		else
		{
			break;
		}


	}

	status = frContextSetScene(context, scene);
	CHECK_STATUS_RETURNNULL;

	if ( Read_Element_EndObject(objType,scene,objID) != FR_SUCCESS ) { FRS_MACRO_ERROR(); return NULL; }

	return scene;
}


FRS7::FRS_ELEMENTS_TYPE FRS7::Read_whatsNext(std::string& name, std::string& objBegType)
{
	if ( sizeof(FRS_ELEMENTS_TYPE) != sizeof(int32_t) )
	{
		FRS_MACRO_ERROR();
		return FRSRT_UNDEF;
	}

	FRS_ELEMENTS_TYPE elementType = FRSRT_UNDEF;
	m_frsFile->read((char*)&elementType, sizeof(int32_t));
	if ( m_frsFile->eof() )
	{
		return FRSRT_UNDEF;
	}

	Read_String(name);

	int64_t sizeOfElementHead = sizeof(int32_t) + sizeof(int32_t) + name.length();

	if ( elementType == FRSRT_OBJECT_BEG ||  elementType == FRSRT_REFERENCE)
	{
		Read_String(objBegType);
		sizeOfElementHead += sizeof(int32_t) + objBegType.length();
	}
	else
	{
		objBegType = "";
	}

	//rewind to the beginning of element 
	m_frsFile->seekg(-sizeOfElementHead, m_frsFile->cur);

	if ( 
		   elementType == FRSRT_OBJECT_BEG 
		|| elementType == FRSRT_OBJECT_END 
		|| elementType == FRSRT_PARAMETER 
		|| elementType == FRSRT_REFERENCE 
		)
	{
		return elementType;
	}
	else
	{
		FRS_MACRO_ERROR();
		return FRSRT_UNDEF;
	}
}

fr_int FRS7::Read_Element_StartObject(std::string& name, std::string& type,  int32_t& id)
{
	FRS_ELEMENTS_TYPE elementType = FRSRT_UNDEF;
	m_frsFile->read((char*)&elementType, sizeof(int32_t));
	if ( elementType != FRSRT_OBJECT_BEG ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	Read_String(name);
	Read_String(type);
	m_frsFile->read((char*)&id, sizeof(int32_t));

	m_level++;
	return FR_SUCCESS;
}

fr_int FRS7::Read_Element_EndObject(const std::string& type, void* obj, int32_t id)
{
	FRS_ELEMENTS_TYPE elementType = FRSRT_UNDEF;
	m_frsFile->read((char*)&elementType, sizeof(int32_t));
	std::string endObjName; //this string is not used
	Read_String(endObjName);
	if ( elementType != FRSRT_OBJECT_END ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	m_level--;

	RPS_OBJECT_DECLARED newObj;
	newObj.id = id;
	newObj.type = type;
	newObj.obj = obj;
	m_listObjectDeclared.push_back(newObj);


	return FR_SUCCESS;
}

fr_int FRS7::Read_Element_ParameterData(void* data, uint64_t size)
{
	if ( data == NULL && size != 0 )
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}

	m_frsFile->read((char*)data, size);
	return FR_SUCCESS;
}

fr_int FRS7::Read_Element_Parameter(std::string& name, FRS_PARAMETER_TYPE& type, uint64_t& dataSize)
{
	FRS_ELEMENTS_TYPE elementType = FRSRT_UNDEF;
	m_frsFile->read((char*)&elementType, sizeof(int32_t));
	if ( elementType != FRSRT_PARAMETER ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	Read_String(name);
	m_frsFile->read((char*)&type, sizeof(type));
	m_frsFile->read((char*)&dataSize, sizeof(dataSize));
	return FR_SUCCESS;
}

fr_int FRS7::Read_Element_Reference(std::string& name, std::string& type, RPS_OBJECT_DECLARED& objReferenced)
{
	FRS_ELEMENTS_TYPE elementType = FRSRT_UNDEF;
	m_frsFile->read((char*)&elementType, sizeof(int32_t));
	if ( elementType != FRSRT_REFERENCE ) { FRS_MACRO_ERROR(); return FR_ERROR_INTERNAL_ERROR; }
	Read_String(name);
	Read_String(type);

	int32_t id;
	m_frsFile->read((char*)&id, sizeof(int32_t));

	//search object in list
	bool found = false;
	for(int iObj=0; iObj<m_listObjectDeclared.size(); iObj++)
	{
		if ( m_listObjectDeclared[iObj].id == id )
		{
			objReferenced = m_listObjectDeclared[iObj];
			found = true;
			break;
		}
	}

	if ( !found )
	{
		FRS_MACRO_ERROR();
		return FR_ERROR_INTERNAL_ERROR;
	}

	return FR_SUCCESS;
}

fr_int FRS7::Read_String(std::string& str)
{
	uint32_t strSize = 0;
	m_frsFile->read((char*)&strSize, sizeof(strSize));

	if ( strSize > 0 )
	{
		char* strRead = new char[strSize+1];
		m_frsFile->read(strRead, strSize);
		strRead[strSize] = '\0';
		str = std::string(strRead);
		delete[] strRead; 
		strRead = NULL;
	}
	else
	{
		str = "";
	}

	return FR_SUCCESS;
}
