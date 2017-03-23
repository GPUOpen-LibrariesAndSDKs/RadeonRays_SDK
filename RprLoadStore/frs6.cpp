#include <vector>
#include <cstring>
#include <algorithm>
#include "frs6.h"


#define CHECK_STATUS_RETURNERROR { if (status != RPR_SUCCESS) { ErrorDetected(); return status; } }
#define CHECK_STATUS_RETURNNULL { if (status != RPR_SUCCESS) { ErrorDetected(); return 0; } }

int32_t const FRS6::m_SHAPE_BEG      = 0x0AB000001;
int32_t const FRS6::m_SHAPE_END      = 0x0AB000002;
int32_t const FRS6::m_CAMERA_BEG     = 0x0AB000003;
int32_t const FRS6::m_CAMERA_END     = 0x0AB000004;
int32_t const FRS6::m_LIGHT_BEG      = 0x0AB000005;
int32_t const FRS6::m_LIGHT_END      = 0x0AB000006;
int32_t const FRS6::m_SHADER_BEG     = 0x0AB000007;
int32_t const FRS6::m_SHADER_END     = 0x0AB000008;
int32_t const FRS6::m_SCENE_BEG      = 0x0AB000009;
int32_t const FRS6::m_SCENE_END      = 0x0AB00000A;
int32_t const FRS6::m_CONTEXT_BEG    = 0x0AB00000B;
int32_t const FRS6::m_CONTEXT_END    = 0x0AB00000C;
int32_t const FRS6::m_EVERYTHING_BEG = 0x0AB00000D;
int32_t const FRS6::m_EVERYTHING_END = 0x0AB00000E;
int32_t const FRS6::m_IMAGE_BEG      = 0x0AB00000F;
int32_t const FRS6::m_IMAGE_END      = 0x0AB000010;
int32_t const FRS6::m_3DSDATA_BEG    = 0x0AB000011;
int32_t const FRS6::m_3DSDATA_END    = 0x0AB000012;

std::vector<rpr_shape> FRS6::m_shapeList;
std::vector<rpr_image> FRS6::m_imageList;

int32_t FRS6::m_fileVersionOfLoadFile = 0;

char const FRS6::m_HEADER_CHECKCODE[4] = {'F','R','S','F'}; // "FRSF" = FireRenderSceneFile

// history of changes :
// version 02 : first
// version 03 : add shadow flag
// version 04 : add LOADER_DATA_FROM_3DS
// version 05 : (07 jan 2016) new material system 
// version 06 : (25 feb 2016) add more data inside LOADER_DATA_FROM_3DS + possibility to save if no camera in scene.  note that version 6 can import version 5
int32_t const FRS6::m_FILE_VERSION = 0x00000006;

rpr_int FRS6::StoreCamera(rpr_camera camera, std::ofstream *myfile)
{
	
	myfile->write((const char*)&m_CAMERA_BEG, sizeof(m_CAMERA_BEG));


	rpr_int status = RPR_SUCCESS;

	float param_fstop;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FSTOP, sizeof(param_fstop), &param_fstop, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_fstop, sizeof(param_fstop));

	//status = rprCameraGetInfo(camera, RPR_CAMERA_TRANSFORM, sizeof(float)* 16, &param, NULL);
	//CHECK_STATUS_RETURNERROR;
	//myfile->write(param, sizeof(float) * 16);

	unsigned int param_apblade;
	status = rprCameraGetInfo(camera, RPR_CAMERA_APERTURE_BLADES, sizeof(unsigned int), &param_apblade, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_apblade, sizeof(unsigned int));

	float param_camexpo;
	status = rprCameraGetInfo(camera, RPR_CAMERA_EXPOSURE, sizeof(param_camexpo), &param_camexpo, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_camexpo, sizeof(param_camexpo));

	float param_focalLen;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FOCAL_LENGTH, sizeof(param_focalLen), &param_focalLen, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_focalLen, sizeof(param_focalLen));

	float param_sensorsize[2];
	status = rprCameraGetInfo(camera, RPR_CAMERA_SENSOR_SIZE, sizeof(param_sensorsize), &param_sensorsize, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_sensorsize, sizeof(param_sensorsize));

	rpr_camera_mode param_mode;
	status = rprCameraGetInfo(camera, RPR_CAMERA_MODE, sizeof(param_mode), &param_mode, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_mode, sizeof(param_mode));

	float param_orthowidth;
	status = rprCameraGetInfo(camera, RPR_CAMERA_ORTHO_WIDTH, sizeof(param_orthowidth), &param_orthowidth, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_orthowidth, sizeof(param_orthowidth));

    float param_orthoheight;
    status = rprCameraGetInfo(camera, RPR_CAMERA_ORTHO_HEIGHT, sizeof(param_orthoheight), &param_orthoheight, NULL);
    CHECK_STATUS_RETURNERROR;
    myfile->write((const char*)&param_orthoheight, sizeof(param_orthoheight));

	float param_focusdist;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FOCUS_DISTANCE, sizeof(param_focusdist), &param_focusdist, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&param_focusdist, sizeof(param_focusdist));

	rpr_float param_pos[3];
	status = rprCameraGetInfo(camera, RPR_CAMERA_POSITION, sizeof(param_pos), &param_pos, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)param_pos, sizeof(param_pos));

	rpr_float param_at[3];
	status = rprCameraGetInfo(camera, RPR_CAMERA_LOOKAT, sizeof(param_at), &param_at, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)param_at, sizeof(param_at));

	rpr_float param_up[3];
	status = rprCameraGetInfo(camera, RPR_CAMERA_UP, sizeof(param_up), &param_up, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)param_up, sizeof(param_up));

	myfile->write((const char*)&m_CAMERA_END, sizeof(m_CAMERA_END));


	return RPR_SUCCESS;
}

rpr_camera FRS6::LoadCamera(rpr_context context, std::istream *myfile)
{
	rpr_int status = RPR_SUCCESS;
	//uint64_t param_length = 0;

	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_CAMERA_BEG)
	{
		ErrorDetected();
		return 0;
	}


	rpr_camera  camera = NULL;
	status = rprContextCreateCamera(context, &camera);
	CHECK_STATUS_RETURNNULL;

	float param_fstop;
	myfile->read((char*)&param_fstop, sizeof(param_fstop));
	status = rprCameraSetFStop(camera, param_fstop);
	CHECK_STATUS_RETURNNULL;

	unsigned int param_apblade;
	myfile->read((char*)&param_apblade, sizeof(param_apblade));
	status = rprCameraSetApertureBlades(camera, param_apblade);
	CHECK_STATUS_RETURNNULL;

	float param_camexpo;
	myfile->read((char*)&param_camexpo, sizeof(param_camexpo));
	status = rprCameraSetExposure(camera, param_camexpo);
	CHECK_STATUS_RETURNNULL;

	float param_focalLen;
	myfile->read((char*)&param_focalLen, sizeof(param_focalLen));
	status = rprCameraSetFocalLength(camera, param_focalLen);
	CHECK_STATUS_RETURNNULL;

	float param_sensorsize[2];
	myfile->read((char*)&param_sensorsize, sizeof(param_sensorsize));
	status = rprCameraSetSensorSize(camera, (float)param_sensorsize[0], (float)param_sensorsize[1]);
	CHECK_STATUS_RETURNNULL;

	rpr_camera_mode param_mode;
	myfile->read((char*)&param_mode, sizeof(param_mode));
	status = rprCameraSetMode(camera, param_mode);
	CHECK_STATUS_RETURNNULL;

	float param_orthowidth;
	myfile->read((char*)&param_orthowidth, sizeof(param_orthowidth));
	status = rprCameraSetOrthoWidth(camera, param_orthowidth);
	CHECK_STATUS_RETURNNULL;

	float param_focusdist;
	myfile->read((char*)&param_focusdist, sizeof(param_focusdist));
	status = rprCameraSetFocusDistance(camera, param_focusdist);
	CHECK_STATUS_RETURNNULL;

	float param_position[3];
	myfile->read((char*)&param_position, sizeof(param_position));
	float param_lookat[3];
	myfile->read((char*)&param_lookat, sizeof(param_lookat));
	float param_up[3];
	myfile->read((char*)&param_up, sizeof(param_up));
	status = rprCameraLookAt(camera,
		param_position[0], param_position[1], param_position[2],
		param_lookat[0], param_lookat[1], param_lookat[2],
		param_up[0], param_up[1], param_up[2]);
	CHECK_STATUS_RETURNNULL;


	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_CAMERA_END)
	{
		ErrorDetected();
		return 0;
	}


	return camera;
}



/*
rpr_int FRS6::StoreShader(rpr_material_node shader, std::ofstream *myfile)
{

	
	rpr_int status = RPR_SUCCESS;
	//uint64_t param_length = 0;
	//char param[4096];

	myfile->write((const char*)&m_SHADER_BEG, sizeof(m_SHADER_BEG));

	fr_shader_type shaderType = 0;
	status = frShaderGetInfo(shader, FR_SHADER_TYPE, sizeof(shaderType), &shaderType, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((char*)&shaderType, sizeof(shaderType));

	
	//  name is not implemented
	//
	//uint64_t shaderName_lenght = 0;
	//status = frShaderGetInfo(shader, FR_SHADER_NAME, 0, NULL, &shaderName_lenght);
	//CHECK_STATUS_RETURNERROR;
	//char* shaderName = new char[shaderName_lenght];
	//status = frShaderGetInfo(shader, FR_SHADER_NAME, shaderName_lenght, shaderName, NULL);
	//CHECK_STATUS_RETURNERROR;
	//myfile->write((char*)&shaderName_lenght, sizeof(shaderName_lenght));
	//myfile->write((char*)shaderName, shaderName_lenght);
	//delete[] shaderName; shaderName = NULL;
	//

	uint64_t shaderParameterCount = 0;
	status = frShaderGetInfo(shader, FR_SHADER_PARAMETER_COUNT, sizeof(shaderParameterCount), &shaderParameterCount, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((char*)&shaderParameterCount, sizeof(shaderParameterCount));


	for (int i = 0; i < shaderParameterCount; i++)
	{
		
		uint64_t shaderParameterName_lenght = 0;
		status = frShaderGetParameterInfo(shader, i, RPR_PARAMETER_NAME, 0, NULL, &shaderParameterName_lenght);
		CHECK_STATUS_RETURNERROR;
		char* shaderParameterName = new char[shaderParameterName_lenght];
		status = frShaderGetParameterInfo(shader, i, RPR_PARAMETER_NAME, shaderParameterName_lenght, shaderParameterName, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&shaderParameterName_lenght, sizeof(shaderParameterName_lenght));
		myfile->write((char*)shaderParameterName, shaderParameterName_lenght);
		
		rpr_parameter_type paramType;
		status = frShaderGetParameterInfo(shader, i, RPR_PARAMETER_TYPE, sizeof(paramType), &paramType, NULL);
		myfile->write((char*)&paramType, sizeof(paramType));
		CHECK_STATUS_RETURNERROR;


		uint64_t shaderParameterValue_lenght = 0;
		status = frShaderGetParameterInfo(shader, i, RPR_PARAMETER_VALUE, 0, NULL, &shaderParameterValue_lenght);
		CHECK_STATUS_RETURNERROR;
		char* shaderParameterValue = new char[shaderParameterValue_lenght];
		status = frShaderGetParameterInfo(shader, i, RPR_PARAMETER_VALUE, shaderParameterValue_lenght, shaderParameterValue, NULL);
		CHECK_STATUS_RETURNERROR;

		if (paramType == RPR_PARAMETER_TYPE_IMAGE)
		{
			if (shaderParameterValue_lenght != sizeof(rpr_image))
			{
				ErrorDetected();
				return RPR_ERROR_INTERNAL_ERROR;
			}
			rpr_image* img = (rpr_image*)shaderParameterValue;
			if (*img != 0)
			{
				int32_t imageYes = 1;
				myfile->write((char*)&imageYes, sizeof(imageYes));
				status = StoreImage(*img, myfile);
				CHECK_STATUS_RETURNERROR;
			}
			else
			{
				int32_t imageNo = 0;
				myfile->write((char*)&imageNo, sizeof(imageNo));
			}
		}
		else if (RPR_PARAMETER_TYPE_SHADER == paramType)
		{
			if (shaderParameterValue_lenght != sizeof(rpr_material_node))
			{
				ErrorDetected();
				return RPR_ERROR_INTERNAL_ERROR;
			}
			else
			{
				rpr_material_node* shader = (rpr_material_node*)shaderParameterValue;
				status = StoreShader(*shader, myfile);
				CHECK_STATUS_RETURNERROR;
			}
		}
		else
		{
			myfile->write((char*)&shaderParameterValue_lenght, sizeof(shaderParameterValue_lenght));
			myfile->write((char*)shaderParameterValue, shaderParameterValue_lenght);
		}
		
		
		//don't need description in store file
		//
		//uint64_t shaderParameterDescription_lenght = 0;
		//status = frShaderGetParameterInfo(shader, i, FR_SHADER_PARAMETER_DESCRIPTION, 0, NULL, &shaderParameterDescription_lenght);
		//CHECK_STATUS_RETURNERROR;
		//char* shaderParameterDescription = new char[shaderParameterDescription_lenght];
		//status = frShaderGetParameterInfo(shader, i, FR_SHADER_PARAMETER_DESCRIPTION, shaderParameterDescription_lenght, shaderParameterDescription, NULL);
		//CHECK_STATUS_RETURNERROR;
		//myfile->write((char*)&shaderParameterDescription_lenght, sizeof(shaderParameterDescription_lenght));
		//myfile->write((char*)shaderParameterDescription, shaderParameterDescription_lenght);
		//delete[] shaderParameterDescription; shaderParameterDescription = NULL;


		delete[] shaderParameterName; shaderParameterName = NULL;
		delete[] shaderParameterValue; shaderParameterValue = NULL;

	}

	myfile->write((const char*)&m_SHADER_END, sizeof(m_SHADER_END));


	

	return RPR_SUCCESS;

	//	extern RPR_API_ENTRY rpr_int          frShaderGetParameterImagePath(rpr_material_node shader, const rpr_char* parameter, const rpr_char* image_path);
	
}
*/


/*
rpr_material_node FRS6::LoadShader(rpr_context context, std::istream *myfile)
{
	rpr_int status = RPR_SUCCESS;

	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SHADER_BEG)
	{
		return 0;
	}


	fr_shader_type shaderType;
	myfile->read((char*)&shaderType, sizeof(fr_shader_type)); //SHADER_TYPE
	rpr_material_node  shader = rprContextCreateShader(context, shaderType, &status);
	CHECK_STATUS_RETURNNULL;


	uint64_t nbParameters;
	myfile->read((char*)&nbParameters, sizeof(nbParameters)); //SHADER_PARAMETER_COUNT
	for (int i = 0; i < nbParameters; i++)
	{
		uint64_t paramName_length = 0;
		myfile->read((char*)&paramName_length, sizeof(paramName_length));
		char* paramName_data = new char[paramName_length];
		myfile->read((char*)paramName_data, paramName_length);

		rpr_parameter_type paramType;
		myfile->read((char*)&paramType, sizeof(paramType)); 

		

		if (paramType == RPR_PARAMETER_TYPE_IMAGE)
		{
			int32_t imageExist = 0;
			myfile->read((char*)&imageExist, sizeof(imageExist));
			if (imageExist == 1)
			{
				rpr_image newImage = LoadImage(context, myfile);
				if (newImage == 0)
				{
					return 0;
				}
				status = frShaderSetParameterImage(shader, paramName_data, newImage);
				CHECK_STATUS_RETURNNULL;
			}
		}
		else if (paramType == RPR_PARAMETER_TYPE_SHADER)
		{
			rpr_material_node newShader = LoadShader(context, myfile);
			if (newShader == 0)
			{
				return 0;
			}
			status = frShaderSetParameterShader(shader, paramName_data, newShader);
			CHECK_STATUS_RETURNNULL;
		}
		else
		{
			uint64_t value_length = 0;
			myfile->read((char*)&value_length, sizeof(value_length));
			char* value_data = new char[value_length];
			myfile->read(value_data, value_length);


	
			if (paramType == RPR_PARAMETER_TYPE_FLOAT)
			{
				float* value_data_float = (float*)value_data;

				if (value_length == sizeof(float) * 1)
				{
					status = frShaderSetParameter1f(shader, paramName_data, value_data_float[0]);
					CHECK_STATUS_RETURNNULL
				}
				else
				{
					return 0; 
				}
			}
			else if (paramType == RPR_PARAMETER_TYPE_FLOAT2)
			{
				float* value_data_float = (float*)value_data;

				if (value_length == sizeof(float) * 2)
				{
					status = frShaderSetParameter2f(shader, paramName_data, value_data_float[0], value_data_float[1]);
					CHECK_STATUS_RETURNNULL
				}
				else
				{
					return 0; 
				}
			}
			else if (paramType == RPR_PARAMETER_TYPE_FLOAT3)
			{
				float* value_data_float = (float*)value_data;

				if (value_length == sizeof(float) * 3)
				{
					status = frShaderSetParameter3f(shader, paramName_data, value_data_float[0], value_data_float[1], value_data_float[2]);
					CHECK_STATUS_RETURNNULL
				}
				else
				{
					return 0; 
				}
			}
			else if (paramType == RPR_PARAMETER_TYPE_FLOAT4)
			{
				float* value_data_float = (float*)value_data;

				if (value_length == sizeof(float) * 4)
				{
					status = frShaderSetParameter4f(shader, paramName_data, value_data_float[0], value_data_float[1], value_data_float[2], value_data_float[3]);
					CHECK_STATUS_RETURNNULL
				}
				else
				{
					return 0; 
				}

			}
			else if (paramType == RPR_PARAMETER_TYPE_STRING)
			{
				status = frShaderSetParameterImagePath(shader, paramName_data, value_data);
				CHECK_STATUS_RETURNNULL;
			}
			else
			{
				return 0;
			}



			delete[] paramName_data; paramName_data = NULL;
			delete[] value_data; value_data = NULL;
		}


		
	}

	
	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SHADER_END)
	{
		return 0;
	}
	
	return shader;



	
	//return 0;
}
*/






rpr_int FRS6::StoreShape(rpr_shape shape, std::ofstream *myfile)
{
	rpr_int status = RPR_SUCCESS;

	myfile->write((const char*)&m_SHAPE_BEG, sizeof(m_SHAPE_BEG));
	
	rpr_shape_type type;
	status = rprShapeGetInfo(shape, RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&type, sizeof(rpr_shape_type));

	if (
		type == RPR_SHAPE_TYPE_MESH
		)
	{

		m_shapeList.push_back(shape);

		
		uint64_t poly_count = 0;
		status = rprMeshGetInfo(shape, RPR_MESH_POLYGON_COUNT, sizeof(poly_count), &poly_count, NULL); //number of primitives
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&poly_count, sizeof(uint64_t));

		uint64_t  vertex_count = 0;
		status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_COUNT, sizeof(vertex_count), &vertex_count, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&vertex_count, sizeof(uint64_t));

		uint64_t normal_count = 0;
		status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_COUNT, sizeof(normal_count), &normal_count, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&normal_count, sizeof(uint64_t));

		uint64_t uv_count = 0;
		status = rprMeshGetInfo(shape, RPR_MESH_UV_COUNT, sizeof(uv_count), &uv_count, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&uv_count, sizeof(uint64_t));

		

		if (vertex_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; data = NULL;
		}
		if (normal_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; data = NULL;
		}
		if (uv_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_UV_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_UV_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; 
			data = NULL;
			
		}
		if (poly_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_INDEX_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_INDEX_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; data = NULL;
			
		}
		if (normal_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_INDEX_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_INDEX_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; data = NULL;
			
		}
		if (uv_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_UV_INDEX_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_UV_INDEX_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; data = NULL;
			
		}
		if (poly_count)
		{
			size_t required_size_size_t = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_NUM_FACE_VERTICES_ARRAY, 0, NULL, &required_size_size_t);
			uint64_t required_size = required_size_size_t;
			CHECK_STATUS_RETURNERROR;
			float* data = new float[required_size / sizeof(float)];
			status = rprMeshGetInfo(shape, RPR_MESH_NUM_FACE_VERTICES_ARRAY, required_size, data, NULL);
			CHECK_STATUS_RETURNERROR;
			myfile->write((char*)&required_size, sizeof(required_size));
			myfile->write((const char*)data, required_size);
			delete[] data; data = NULL;
			
		}

	}
	else if (type == RPR_SHAPE_TYPE_INSTANCE)
	{
		//search address of original shape

		rpr_shape shapeOriginal = NULL;
		status = rprInstanceGetBaseShape(shape, &shapeOriginal);
		CHECK_STATUS_RETURNERROR;

		if (shapeOriginal == 0)
		{
			ErrorDetected();
			return RPR_ERROR_INTERNAL_ERROR;
		}

		bool found = false;
		for (int iShape = 0; iShape < m_shapeList.size(); iShape++)
		{
			if (m_shapeList[iShape] == shapeOriginal)
			{
				int32_t indexShapeBase = iShape;
				myfile->write((const char*)&indexShapeBase, sizeof(indexShapeBase));
				found = true;
				break;
			}
		}

		if (!found)
		{
			ErrorDetected();
			return RPR_ERROR_INTERNAL_ERROR;
		}

	}


	rpr_material_node data_shader = 0;
	status = rprShapeGetInfo(shape, RPR_SHAPE_MATERIAL, sizeof(data_shader), &data_shader, NULL);
	CHECK_STATUS_RETURNERROR;
	if (data_shader)
	{
		int32_t _true = 1;
		myfile->write((const char*)&_true, sizeof(_true));
	}
	else
	{
		int32_t _false = 0;
		myfile->write((const char*)&_false, sizeof(_false));
	}
	if (data_shader != NULL)
	{
		rpr_int error = StoreShader(data_shader, myfile);
		if (error != RPR_SUCCESS)
		{
			return error;
		}
	}
	
	
	float data_transform[16];
	status = rprShapeGetInfo(shape, RPR_SHAPE_TRANSFORM, sizeof(data_transform), data_transform, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)data_transform, sizeof(data_transform));

	
	float data_linMotion[3];
	status = rprShapeGetInfo(shape, RPR_SHAPE_LINEAR_MOTION, sizeof(data_linMotion) , data_linMotion, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)data_linMotion, sizeof(data_linMotion));

	float data_angMotion[4];
	status = rprShapeGetInfo(shape, RPR_SHAPE_ANGULAR_MOTION, sizeof(data_angMotion) , data_angMotion, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)data_angMotion, sizeof(data_angMotion));

	rpr_bool data_visFlag;
	status = rprShapeGetInfo(shape, RPR_SHAPE_VISIBILITY_FLAG, sizeof(data_visFlag), &data_visFlag, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&data_visFlag, sizeof(data_visFlag));
	
	rpr_bool data_shadoflag;
	status = rprShapeGetInfo(shape, RPR_SHAPE_SHADOW_FLAG, sizeof(data_shadoflag), &data_shadoflag, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((const char*)&data_shadoflag, sizeof(data_shadoflag));

	myfile->write((const char*)&m_SHAPE_END, sizeof(m_SHAPE_END));

	return RPR_SUCCESS;
}

rpr_shape FRS6::LoadShape(rpr_context context, rpr_material_system materialSystem, std::istream *myfile )
{
	rpr_int status = RPR_SUCCESS;

	rpr_shape shape = NULL;


	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SHAPE_BEG)
	{
		ErrorDetected();
		return 0;
	}

	rpr_shape_type shapeType;
	myfile->read((char*)&shapeType, sizeof(shapeType));
	
	uint64_t sizePositionBufferByte = 0;
	uint64_t sizeNormalBufferByte = 0;
	uint64_t sizeUVBufferByte = 0;
	
	if (shapeType == RPR_SHAPE_TYPE_MESH)
	{
		uint64_t poly_count = NULL;
		uint64_t vertex_count = NULL;
		uint64_t normal_count = NULL;
		uint64_t uv_count = NULL;
		
		rpr_float* vertices = NULL;
		rpr_float* normals = NULL;
		rpr_float* texcoords = NULL;
		rpr_int*   vertex_indices  = NULL;
		rpr_int*   normal_indices = NULL;
		rpr_int*   texcoord_indices = NULL;
		rpr_int*   num_face_vertices = NULL;
		

		myfile->read((char*)&poly_count, sizeof(uint64_t));
		myfile->read((char*)&vertex_count, sizeof(uint64_t));
		myfile->read((char*)&normal_count, sizeof(uint64_t));
		myfile->read((char*)&uv_count, sizeof(uint64_t));

		if (vertex_count)
		{
			myfile->read((char*)&sizePositionBufferByte, sizeof(uint64_t));
			vertices = new float[sizePositionBufferByte / sizeof(float)];
			myfile->read((char*)vertices, sizePositionBufferByte);
		}
		if (normal_count)
		{
			myfile->read((char*)&sizeNormalBufferByte, sizeof(uint64_t));
			normals = new float[sizeNormalBufferByte / sizeof(float)];
			myfile->read((char*)normals, sizeNormalBufferByte);
		}
		if (uv_count)
		{
			myfile->read((char*)&sizeUVBufferByte, sizeof(uint64_t));
			texcoords = new float[sizeUVBufferByte / sizeof(float)];
			myfile->read((char*)texcoords, sizeUVBufferByte);
		}
		if (poly_count)
		{
			uint64_t required_size;
			myfile->read((char*)&required_size, sizeof(uint64_t));
			vertex_indices = new int[required_size / sizeof(int)];
			myfile->read((char*)vertex_indices, required_size);
		}
		if (normal_count)
		{
			uint64_t required_size;
			myfile->read((char*)&required_size, sizeof(uint64_t));
			normal_indices = new int[required_size / sizeof(int)];
			myfile->read((char*)normal_indices, required_size);
		}
		if (uv_count)
		{
			uint64_t required_size;
			myfile->read((char*)&required_size, sizeof(uint64_t));
			texcoord_indices = new int[required_size / sizeof(int)];
			myfile->read((char*)texcoord_indices, required_size);
		}
		if (poly_count)
		{
			uint64_t required_size;
			myfile->read((char*)&required_size, sizeof(uint64_t));
			num_face_vertices = new int[required_size / sizeof(int)];
			myfile->read((char*)num_face_vertices, required_size);
		}

		shape = NULL;
		status = rprContextCreateMesh(context,
			&vertices[0], vertex_count, int(sizePositionBufferByte / vertex_count),
			&normals[0], normal_count, int(sizeNormalBufferByte / normal_count),
			&texcoords[0], uv_count, int(sizeUVBufferByte / uv_count),
			vertex_indices, sizeof(rpr_int),
			normal_indices, sizeof(rpr_int),
			texcoord_indices, sizeof(rpr_int),
			num_face_vertices, poly_count, &shape);
		CHECK_STATUS_RETURNNULL;

		m_shapeList.push_back(shape);

		delete[] vertices; vertices = NULL;
		delete[] normals; normals = NULL;
		delete[] texcoords; texcoords = NULL;
		delete[] vertex_indices; vertex_indices = NULL;
		delete[] normal_indices; normal_indices = NULL;
		delete[] texcoord_indices; texcoord_indices = NULL;
		delete[] num_face_vertices; num_face_vertices = NULL;

	}
	else if ((rpr_shape_type)shapeType == RPR_SHAPE_TYPE_INSTANCE)
	{
		int32_t indexShapeOriginal = 0;
		myfile->read((char*)&indexShapeOriginal, sizeof(indexShapeOriginal));
		if (indexShapeOriginal >= 0 && indexShapeOriginal < m_shapeList.size())
		{
			status = rprContextCreateInstance(context, m_shapeList[indexShapeOriginal], &shape);
			CHECK_STATUS_RETURNNULL;
		}
		else
		{
			ErrorDetected();
			return 0;
		}

	}

	int32_t shaderExist = 0;
	myfile->read((char*)&shaderExist, sizeof(shaderExist));
	if (shaderExist != 0)
	{
		rpr_material_node shader = LoadShader(materialSystem , context, myfile);
		if (shader == 0)
		{
			ErrorDetected();
			return 0;
		}
		status = rprShapeSetMaterial(shape, shader);
		CHECK_STATUS_RETURNNULL
	}

	float float16[16];
	myfile->read((char*)float16, sizeof(float16));
	status = rprShapeSetTransform(shape, false, (rpr_float*)float16);
	CHECK_STATUS_RETURNNULL

	float linMotion[3];
	myfile->read((char*)linMotion, sizeof(linMotion));
	status = rprShapeSetLinearMotion(shape, linMotion[0], linMotion[1], linMotion[2] );
	CHECK_STATUS_RETURNNULL

	float angMotion[4];
	myfile->read((char*)angMotion, sizeof(angMotion));
	status = rprShapeSetAngularMotion(shape, angMotion[0], angMotion[1], angMotion[2], angMotion[3]);
	CHECK_STATUS_RETURNNULL

	rpr_bool visibilityFlag;
	myfile->read((char*)&visibilityFlag, sizeof(visibilityFlag));
	status = rprShapeSetVisibility(shape, visibilityFlag);
	CHECK_STATUS_RETURNNULL

	rpr_bool shadowFlag;
	myfile->read((char*)&shadowFlag, sizeof(shadowFlag));
	status = rprShapeSetShadow(shape, shadowFlag);
	CHECK_STATUS_RETURNNULL

	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SHAPE_END)
	{
		ErrorDetected();
		return 0;
	}

	return shape;
}

rpr_int FRS6::StoreLight(rpr_light light, rpr_scene scene, std::ofstream *myfile)
{
	myfile->write((const char*)&m_LIGHT_BEG, sizeof(m_LIGHT_BEG));

	rpr_int status = RPR_SUCCESS;
	//uint64_t param_length = 0;
	rpr_light_type type;

	status = rprLightGetInfo(light, RPR_LIGHT_TYPE, sizeof(rpr_light_type), &type, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((char*)&type, sizeof(rpr_light_type));

	

	if (type == RPR_LIGHT_TYPE_POINT)
	{
		float radian3f[3];
		status = rprLightGetInfo(light, RPR_POINT_LIGHT_RADIANT_POWER, sizeof(radian3f), &radian3f, NULL);
		CHECK_STATUS_RETURNERROR;
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&radian3f, sizeof(radian3f));
	}
	else if (type == RPR_LIGHT_TYPE_DIRECTIONAL)
	{
		float radian3f[3];
		status = rprLightGetInfo(light, RPR_DIRECTIONAL_LIGHT_RADIANT_POWER, sizeof(radian3f), &radian3f, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&radian3f, sizeof(radian3f));
	}
	else if (type == RPR_LIGHT_TYPE_SPOT)
	{
		float radian3f[3];
		status = rprLightGetInfo(light, RPR_SPOT_LIGHT_RADIANT_POWER, sizeof(radian3f), &radian3f, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&radian3f, sizeof(radian3f));

		float coneShape[2];
		status = rprLightGetInfo(light, RPR_SPOT_LIGHT_CONE_SHAPE, sizeof(coneShape), &coneShape, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&coneShape, sizeof(coneShape));
	}
	else if (type == RPR_LIGHT_TYPE_ENVIRONMENT)
	{
		rpr_image image = NULL;
		status = rprLightGetInfo(light, RPR_ENVIRONMENT_LIGHT_IMAGE, sizeof(rpr_image), &image, NULL);
		CHECK_STATUS_RETURNERROR;
		StoreImage((rpr_image)image, myfile);
		float intensityScale;
		status = rprLightGetInfo(light, RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE, sizeof(intensityScale), &intensityScale, NULL);
		CHECK_STATUS_RETURNERROR;
		myfile->write((char*)&intensityScale, sizeof(intensityScale));
		

		//not used anymore, because rprSceneSetEnvironmentLight replaced by rprSceneAttachLight
		int32_t isEnvLight = 0;
		myfile->write((const char*)&isEnvLight, sizeof(isEnvLight));

	}
	else
	{
		return RPR_ERROR_INVALID_PARAMETER;
	}

	float lightTransform[16];
	status = rprLightGetInfo(light, RPR_LIGHT_TRANSFORM, sizeof(lightTransform), &lightTransform, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((char*)lightTransform, sizeof(lightTransform));


	myfile->write((const char*)&m_LIGHT_END, sizeof(m_LIGHT_END));

	return RPR_SUCCESS;

}

rpr_light FRS6::LoadLight(rpr_context context, rpr_scene scene, std::istream *myfile)
{
	rpr_int status = RPR_SUCCESS;

	rpr_light light = NULL;

	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_LIGHT_BEG)
	{
		ErrorDetected();
		return 0;
	}


	rpr_light_type type = 0;
	myfile->read((char*)&type, sizeof(rpr_light_type));


	if (type == RPR_LIGHT_TYPE_POINT)
	{
		light = NULL;
		status = rprContextCreatePointLight(context, &light);
		CHECK_STATUS_RETURNNULL;
		float radian3f[3];
		myfile->read((char*)&radian3f, sizeof(radian3f));
		status = rprPointLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
		CHECK_STATUS_RETURNNULL;
	}
	else if (type == RPR_LIGHT_TYPE_DIRECTIONAL)
	{
		light = NULL;
		status = rprContextCreateDirectionalLight(context, &light);
		CHECK_STATUS_RETURNNULL;
		float radian3f[3];
		myfile->read((char*)&radian3f, sizeof(radian3f));
		status = rprDirectionalLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
		CHECK_STATUS_RETURNNULL;
	}
	else if (type == RPR_LIGHT_TYPE_SPOT)
	{
		light = NULL;
		status = rprContextCreateSpotLight(context, &light);
		CHECK_STATUS_RETURNNULL;
		
		float radian3f[3];
		myfile->read((char*)&radian3f, sizeof(radian3f));
		status = rprSpotLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
		CHECK_STATUS_RETURNNULL;

		float coneShape[2];
		myfile->read((char*)&coneShape, sizeof(coneShape));
		status = rprSpotLightSetConeShape(light, coneShape[0], coneShape[1]);
		CHECK_STATUS_RETURNNULL;
	}
	else if (type == RPR_LIGHT_TYPE_ENVIRONMENT)
	{
		light = NULL;
		status = rprContextCreateEnvironmentLight(context, &light);
		CHECK_STATUS_RETURNNULL;
		status = rprEnvironmentLightSetImage(light, LoadImage(context, myfile));
		CHECK_STATUS_RETURNNULL;
		float intensityScale;
		myfile->read((char*)&intensityScale, sizeof(intensityScale));
		status = rprEnvironmentLightSetIntensityScale(light, intensityScale);
		CHECK_STATUS_RETURNNULL;

		int32_t isEnvLight = 0;
		myfile->read((char*)&isEnvLight, sizeof(isEnvLight));
		if (isEnvLight == 1)
		{
			// not used anymore because rprSceneSetEnvironmentLight replaced by rprSceneAttachLight
			//rprSceneSetEnvironmentLight(scene, light); 
		}
	}
	else
	{
		ErrorDetected();
		return 0;
	}



	// rpr_light_info 
	float lightTransform[16];
	myfile->read((char*)&lightTransform, sizeof(lightTransform));
	status = rprLightSetTransform(light, 0, (float*)lightTransform);
	CHECK_STATUS_RETURNNULL;

	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_LIGHT_END)
	{
		ErrorDetected();
		return 0;
	}


	return light;
}


rpr_int FRS6::StoreImage(rpr_image image, std::ofstream *myfile)
{
	
	rpr_int status = RPR_SUCCESS;

	myfile->write((const char*)&m_IMAGE_BEG, sizeof(m_IMAGE_BEG));

	//search if this image is already saved.
	auto pos = std::find(m_imageList.begin(), m_imageList.end(), image);
	if (pos == m_imageList.end())
	{
		//if image not already saved
		m_imageList.push_back(image);
		int32_t imageIndex = -1; // index = -1 --> means that the image was not already saved prviously
		myfile->write((const char*)&imageIndex, sizeof(imageIndex));
	}
	else
	{
		//if image already saved
		int32_t imageIndex = (int32_t)(pos - m_imageList.begin());
		myfile->write((const char*)&imageIndex, sizeof(imageIndex));
		myfile->write((const char*)&m_IMAGE_END, sizeof(m_IMAGE_END));
		return RPR_SUCCESS;
	}

	
	rpr_image_format format;
	status = rprImageGetInfo(image, RPR_IMAGE_FORMAT, sizeof(format), &format, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((char*)&format, sizeof(rpr_image_format));

	rpr_image_desc desc;
	status = rprImageGetInfo(image, RPR_IMAGE_DESC, sizeof(desc), &desc, NULL);
	CHECK_STATUS_RETURNERROR
	myfile->write((char*)&desc, sizeof(rpr_image_desc));


	size_t size_size_t = 0;
	status = rprImageGetInfo(image, RPR_IMAGE_DATA, 0, NULL, &size_size_t);
	uint64_t size = size_size_t;
	CHECK_STATUS_RETURNERROR
	char *idata = new char[size];
	myfile->write((char*)&size, sizeof(uint64_t));
	status = rprImageGetInfo(image, RPR_IMAGE_DATA, size, idata, NULL);
	CHECK_STATUS_RETURNERROR
	myfile->write(idata, size);
	delete[] idata; idata = NULL;
	
	myfile->write((const char*)&m_IMAGE_END, sizeof(m_IMAGE_END));
	
	return status;
}


rpr_image FRS6::LoadImage(rpr_context context, std::istream *myfile)
{

	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_IMAGE_BEG)
	{
		ErrorDetected();
		return 0;
	}

	int32_t imageIndex = 0;
	myfile->read((char*)&imageIndex, sizeof(imageIndex));
	if (imageIndex == -1)
	{
		//if the this part of memory is a real image save

	}
	else
	{
		//if this is just an index to an already saved image

		checkCode = 0;
		myfile->read((char*)&checkCode, sizeof(checkCode));
		if (checkCode != m_IMAGE_END)
		{
			ErrorDetected();
			return 0;
		}

		return m_imageList[imageIndex];
	}

	
	rpr_int status = RPR_SUCCESS;
	rpr_image_desc image_desc;
	rpr_image_format format;
	rpr_image image = NULL;
	
	myfile->read((char*)&format, sizeof(format));
	myfile->read((char*)&image_desc, sizeof(image_desc));
	
	uint64_t size = 0;
	myfile->read((char*)&size, sizeof(size));
	char *idata = new char[size];
	myfile->read(idata, size);
	

	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_IMAGE_END)
	{
		ErrorDetected();
		return 0;
	}

	image = NULL;
	status = rprContextCreateImage(context, format, &image_desc, idata, &image);
	CHECK_STATUS_RETURNNULL;
	delete[] idata; idata = NULL;

	m_imageList.push_back(image);
	
	return image;
}


rpr_int FRS6::StoreEverything(
	rpr_context context, 
	rpr_scene scene, 
	const LOADER_DATA_FROM_3DS* dataFrom3DS,
	std::ofstream *myfile
	)
{
	rpr_int status = RPR_SUCCESS;

	// write a wrong check code: the good check code will be written at the end, when we are sure that the generation didn't fail.
	char const wrongCheckCode[4] = { '0', '0', '0', '0' };
	myfile->write((const char*)&wrongCheckCode, sizeof(wrongCheckCode)); 

	myfile->write((const char*)&m_FILE_VERSION, sizeof(m_FILE_VERSION));
	myfile->write((const char*)&m_EVERYTHING_BEG, sizeof(m_EVERYTHING_BEG));
	status = StoreContext(context, myfile);
	if (status != RPR_SUCCESS) { return status; }
	status = StoreScene(scene, myfile);
	if (status != RPR_SUCCESS) { return status; }

	myfile->write((const char*)&m_3DSDATA_BEG, sizeof(m_3DSDATA_BEG));
	int32_t data3dsStructSize = sizeof(LOADER_DATA_FROM_3DS);
	myfile->write((char*)&data3dsStructSize, sizeof(data3dsStructSize)); //since version 6, we enter the size of the struct
	if ( dataFrom3DS )
	{
		myfile->write((const char*)dataFrom3DS, sizeof(LOADER_DATA_FROM_3DS));
	}
	else
	{
		LOADER_DATA_FROM_3DS emptyData3DS;
		memset(&emptyData3DS, 0, sizeof(LOADER_DATA_FROM_3DS));
		emptyData3DS.used = false; // we don't use this data
		myfile->write((const char*)&emptyData3DS, sizeof(LOADER_DATA_FROM_3DS));
	}
	myfile->write((const char*)&m_3DSDATA_END, sizeof(m_3DSDATA_END));

	myfile->write((const char*)&m_EVERYTHING_END, sizeof(m_EVERYTHING_END));
	//rewind and write the good check code at the begining of the file.
	myfile->seekp(0);
	myfile->write((const char*)&m_HEADER_CHECKCODE, sizeof(m_HEADER_CHECKCODE));

	return RPR_SUCCESS;
}

rpr_int FRS6::LoadEverything(
	rpr_context context, 
	rpr_material_system materialSystem,
	std::istream *myfile, 
	rpr_scene& scene,
	LOADER_DATA_FROM_3DS* dataFrom3DS,
	bool useAlreadyExistingScene
	)
{

	m_fileVersionOfLoadFile = 0;

	rpr_int status = RPR_SUCCESS;

	//check arguments:
	if (context == NULL) { return RPR_ERROR_INVALID_PARAMETER; }
	if (scene != NULL && !useAlreadyExistingScene ) { return RPR_ERROR_INVALID_PARAMETER; }
	if (scene == NULL && useAlreadyExistingScene )  { return RPR_ERROR_INVALID_PARAMETER; }

	char headCheckCode[4] = {0,0,0,0};
	myfile->read(headCheckCode, sizeof(headCheckCode));
	if (headCheckCode[0] != m_HEADER_CHECKCODE[0] || 
		headCheckCode[1] != m_HEADER_CHECKCODE[1] || 
		headCheckCode[2] != m_HEADER_CHECKCODE[2] || 
		headCheckCode[3] != m_HEADER_CHECKCODE[3]  )
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	myfile->read((char*)&m_fileVersionOfLoadFile, sizeof(m_fileVersionOfLoadFile));
	if (m_fileVersionOfLoadFile < 5 || m_fileVersionOfLoadFile > m_FILE_VERSION)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_EVERYTHING_BEG)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	rpr_int succes = LoadContext(context, myfile);
	if (succes != RPR_SUCCESS)
	{
		return succes;
	}


	rpr_scene newScene = LoadScene(context, materialSystem, myfile, useAlreadyExistingScene ? scene : NULL);
	if (newScene == 0)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}



	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_3DSDATA_BEG)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}



	int32_t data3dsStructSize = 0;
	if ( m_fileVersionOfLoadFile >= 6 )
	{
		//since version 6, we enter the size of the struct
		myfile->read((char*)&data3dsStructSize, sizeof(data3dsStructSize));
	}
	else
	{
		struct LOADER_DATA_FROM_3DS__V4_V5 // old strucure version for V4 and V5
		{
			bool used;
			float exposure;
			bool isNormals;
			bool shouldToneMap;
		};

		data3dsStructSize = sizeof(LOADER_DATA_FROM_3DS__V4_V5);
	}

	if ( data3dsStructSize > sizeof(LOADER_DATA_FROM_3DS) )
	{
		//if the size of the read struct is > than the current size, it's an error
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	LOADER_DATA_FROM_3DS data3DSFromFile;
	memset(&data3DSFromFile,0,sizeof(LOADER_DATA_FROM_3DS));
	char* data3dsStructData = (char*)malloc(data3dsStructSize);
	if ( data3dsStructData == NULL )
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}
	myfile->read((char*)data3dsStructData, data3dsStructSize);
	//memset(dataFrom3DS,0,sizeof(LOADER_DATA_FROM_3DS));
	memcpy(&data3DSFromFile,data3dsStructData,data3dsStructSize);
	free(data3dsStructData); data3dsStructData=NULL;
	if (dataFrom3DS)
	{
		memcpy(dataFrom3DS,&data3DSFromFile,data3dsStructSize);
	}


	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_3DSDATA_END)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}


	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_EVERYTHING_END)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	if ( !useAlreadyExistingScene )
	{
		scene = newScene;
	}


	//overwrite the gamma with the 3dsmax gamma
	if ( m_fileVersionOfLoadFile >= 6 && data3DSFromFile.gamma_Enabled )
	{
		status = rprContextSetParameter1f(context, "displaygamma", data3DSFromFile.gamma_disp);
		CHECK_STATUS_RETURNERROR;
	}


	return RPR_SUCCESS;
}


rpr_int FRS6::StoreContext(rpr_context context, std::ofstream *myfile)
{

	rpr_int status = RPR_SUCCESS;
	myfile->write((const char*)&m_CONTEXT_BEG, sizeof(m_CONTEXT_BEG));

	uint64_t param_count = 0;
	status = rprContextGetInfo(context, RPR_CONTEXT_PARAMETER_COUNT, sizeof(uint64_t), &param_count, NULL);
	CHECK_STATUS_RETURNERROR;
	myfile->write((char*)&param_count, sizeof(param_count));


	for (int iPass = 0; iPass < 2; iPass++)
	{
		for (uint64_t i = 0; i < param_count; i++)
		{
			size_t name_length_size_t = 0;
			status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_NAME, 0, NULL, &name_length_size_t);
			uint64_t name_length = name_length_size_t;
			CHECK_STATUS_RETURNERROR;
			char* paramName = new char[name_length];
			status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_NAME, name_length, paramName, NULL);
			CHECK_STATUS_RETURNERROR;


			//need to store in 2 passes.
			//first, we store the parameters that will set several default parameters.
			//so we are sure that all custom parameters will be set during the load of context
			if (
				iPass == 0 && (strcmp("imagefilter.type", paramName) == 0 || strcmp("tonemapping.type", paramName) == 0)
				||
				iPass == 1 && strcmp("imagefilter.type", paramName) != 0 && strcmp("tonemapping.type", paramName) != 0
				)
			{
				myfile->write((char*)&name_length, sizeof(uint64_t));
				myfile->write(paramName, name_length);


				rpr_parameter_type type;
				status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_TYPE, sizeof(type), &type, NULL);
				CHECK_STATUS_RETURNERROR;
				myfile->write((char*)&type, sizeof(type));


				size_t value_length_size_t = 0;
				status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_VALUE, 0, NULL, &value_length_size_t);
				uint64_t value_length = value_length_size_t;
				CHECK_STATUS_RETURNERROR;
				myfile->write((char*)&value_length, sizeof(value_length));

				if (value_length > 0)
				{
					char* paramValue = new char[value_length];
					float* paramValue_float = (float*)paramValue;
					paramValue_float[0];
					status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_VALUE, value_length, paramValue, NULL);
					CHECK_STATUS_RETURNERROR;

					myfile->write(paramValue, value_length);

					delete[] paramValue; paramValue = NULL;
				}
			}
			delete[] paramName; paramName = NULL;

		}
	}


	myfile->write((const char*)&m_CONTEXT_END, sizeof(m_CONTEXT_END));
	
	return RPR_SUCCESS;
}



rpr_int FRS6::LoadContext(rpr_context context, std::istream *myfile)
{
	rpr_int status = RPR_SUCCESS;


	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_CONTEXT_BEG)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	uint64_t nbParam = 0;
	myfile->read((char*)&nbParam, sizeof(uint64_t));


	for (int i = 0; i < nbParam; i++)
	{
		uint64_t name_length = 0;
		myfile->read((char*)&name_length, sizeof(name_length));
		char* name_data = new char[name_length];
		myfile->read(name_data, name_length);

		rpr_parameter_type type;
		myfile->read((char*)&type, sizeof(rpr_parameter_type));

		uint64_t data_length = 0;
		myfile->read((char*)&data_length, sizeof(data_length));

		if (data_length > 0)
		{

			char* data_data = new char[data_length];
			myfile->read(data_data, data_length);

			if (type == RPR_PARAMETER_TYPE_UINT)
			{
				rpr_uint* data_uint = (rpr_uint*)data_data;
				if (data_length == sizeof(rpr_uint))
				{
					status = rprContextSetParameter1u(context, name_data, (rpr_uint)data_uint[0]);
					if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
				}
				else
				{
					ErrorDetected();
					return RPR_ERROR_INTERNAL_ERROR; 
				}
				
			}
			else if (type == RPR_PARAMETER_TYPE_FLOAT)
			{
				rpr_float* data_float = (rpr_float*)data_data;

				if (data_length == sizeof(rpr_float))
				{

						status = rprContextSetParameter1f(context, name_data, data_float[0]);
						if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that

				}
				else
				{
					ErrorDetected();
					return RPR_ERROR_INTERNAL_ERROR; 
				}
			}
			else if (type == RPR_PARAMETER_TYPE_FLOAT3)
			{
				rpr_float* data_float = (rpr_float*)data_data;

				if (data_length == 3*sizeof(rpr_float))
				{
					status = rprContextSetParameter3f(context, name_data, data_float[0], data_float[1], data_float[2]);
					if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
				}
				else
				{
					ErrorDetected();
					return RPR_ERROR_INTERNAL_ERROR; 
				}
			}
			else if (type == RPR_PARAMETER_TYPE_FLOAT4)
			{
				rpr_float* data_float = (rpr_float*)data_data;

				if (data_length == 4*sizeof(rpr_float))
				{
					status = rprContextSetParameter4f(context, name_data, data_float[0], data_float[1], data_float[2], data_float[3]);
					if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
				}
				else
				{
					ErrorDetected();
					return RPR_ERROR_INTERNAL_ERROR; 
				}
			}

			
			delete[] data_data; data_data = NULL;

		}

		delete[] name_data; name_data = NULL;


	}


	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_CONTEXT_END)
	{
		ErrorDetected();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	return RPR_SUCCESS;
}



rpr_int FRS6::StoreScene(rpr_scene scene, std::ofstream *myfile)
{
	rpr_int status = RPR_SUCCESS;

	myfile->write((const char*)&m_SCENE_BEG, sizeof(m_SCENE_BEG));


	uint64_t nbShape = 0;
	status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_COUNT, sizeof(nbShape), &nbShape, NULL);
	CHECK_STATUS_RETURNERROR;
	uint64_t nbLight = 0;
	status = rprSceneGetInfo(scene, RPR_SCENE_LIGHT_COUNT, sizeof(nbLight), &nbLight, NULL);
	CHECK_STATUS_RETURNERROR;
	//uint64_t nbTexture = 0;
	//status = rprSceneGetInfo(scene, RPR_SCENE_TEXTURE_COUNT, sizeof(nbTexture), &nbTexture, NULL);
	//CHECK_STATUS_RETURNERROR;


	myfile->write((char*)&nbShape, sizeof(nbShape));
	myfile->write((char*)&nbLight, sizeof(nbLight));
	//myfile->write((char*)&nbTexture, sizeof(nbTexture));


	rpr_shape* shapes = new rpr_shape[nbShape];
	rpr_light* light = new rpr_light[nbLight];
	//rpr_image* texture = new rpr_image[nbTexture];
	status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_LIST,   nbShape * sizeof(rpr_shape), shapes, NULL);
	CHECK_STATUS_RETURNERROR;
	status = rprSceneGetInfo(scene, RPR_SCENE_LIGHT_LIST, nbLight * sizeof(rpr_light), light, NULL);
	CHECK_STATUS_RETURNERROR;
	//status = rprSceneGetInfo(scene, RPR_SCENE_TEXTURE_LIST, nbTexture * sizeof(rpr_image), texture, NULL);
	//CHECK_STATUS_RETURNERROR;

//	status = rprSceneGetInfo(scene, RPR_SCENE_TEXTURE_LIST, sizeof(textures), &textures, NULL);

	m_shapeList.clear();
	m_imageList.clear();


	//env light 
	rpr_light envLight = NULL;
	//status = rprSceneGetEnvironmentLight(scene, &envLight);
	if (
		envLight != NULL
		//false 
		)
	{
		int32_t envLightExist = 1;
		myfile->write((char*)&envLightExist, sizeof(envLightExist));
		StoreLight(envLight, scene, myfile);
	}
	else
	{
		int32_t envLightExist = 0;
		myfile->write((char*)&envLightExist, sizeof(envLightExist));
	}



	for (uint64_t i = 0; i < nbShape; i++) // first: store non-instanciate shapes
	{
		rpr_shape_type type;
		status = rprShapeGetInfo(shapes[i], RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if (type == RPR_SHAPE_TYPE_MESH)
		{
			rpr_int err = StoreShape(shapes[i], myfile);
			if (err != RPR_SUCCESS)
			{
				return err; // easier for debug if stop everything
			}
		}
	}

	for (uint64_t i = 0; i < nbShape; i++)// second: store instanciate shapes only
	{
		rpr_shape_type type;
		status = rprShapeGetInfo(shapes[i], RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if (type != RPR_SHAPE_TYPE_MESH)
		{
			rpr_int err = StoreShape(shapes[i], myfile);
			if (err != RPR_SUCCESS)
			{
				return err; // easier for debug if stop everything
			}
		}
	}


	for (uint64_t i = 0; i < nbLight; i++)
	{
		rpr_int err = StoreLight(light[i],scene, myfile);
		if (err != RPR_SUCCESS)
		{
			return err; // easier for debug if stop everything
		}
	}



	rpr_camera camera = NULL;
	status = rprSceneGetCamera(scene, &camera);
	CHECK_STATUS_RETURNERROR;
	
	
	int32_t sceneCameraExists = 0;
	if(camera)
	{
		sceneCameraExists = 1;
	}
	myfile->write((char*)&sceneCameraExists, sizeof(sceneCameraExists)); // since version 06, we specify if camera exists of not

	if (camera)
	{
		status = StoreCamera(camera, myfile);
		CHECK_STATUS_RETURNERROR;
	}



	myfile->write((const char*)&m_SCENE_END, sizeof(m_SCENE_END));

	return RPR_SUCCESS;

}

rpr_scene FRS6::LoadScene(rpr_context context, rpr_material_system materialSystem, std::istream *myfile, rpr_scene sceneExisting)
{
	rpr_int status = RPR_SUCCESS;


	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SCENE_BEG)
	{
		ErrorDetected();
		return 0;
	}


	rpr_scene scene = NULL;
	if ( sceneExisting == NULL )
	{
		scene = NULL;
		status = rprContextCreateScene(context, &scene);
	}
	else
	{
		scene = sceneExisting;
	}
	CHECK_STATUS_RETURNNULL;

	
	uint64_t nbShape = 0;
	myfile->read((char*)&nbShape, sizeof(nbShape));
	uint64_t nbLight = 0;
	myfile->read((char*)&nbLight, sizeof(nbLight));
	//uint64_t nbTexture = 0;
	//myfile->read((char*)&nbTexture, sizeof(nbTexture));

	m_shapeList.clear();
	m_imageList.clear();

	int32_t envLightExist = 0;
	myfile->read((char*)&envLightExist, sizeof(envLightExist));
	if (envLightExist == 1)
	{
		rpr_light newLight = LoadLight(context, scene, myfile);
		if (newLight == 0)
		{
			ErrorDetected();
			return 0;
		}

	}



	for (int i = 0; i < nbShape; i++)
	{
		rpr_shape shape = LoadShape(context, materialSystem, myfile);
		if (shape == 0)
		{
			ErrorDetected();
			return 0; // easier for debug if stop everything
		}
		status = rprSceneAttachShape(scene, shape);
		CHECK_STATUS_RETURNNULL;
	}
	for (int i = 0; i < nbLight; i++)
	{
		rpr_light light = LoadLight(context,scene, myfile);
		if (light == 0)
		{
			ErrorDetected();
			return 0; // easier for debug if stop everything
		}
		status = rprSceneAttachLight(scene, light);
		CHECK_STATUS_RETURNNULL;
	}


	int32_t cameraExists = 1;
	if ( m_fileVersionOfLoadFile >= 6 ) // since version 6, we store if camera exists
	{
		myfile->read((char*)&cameraExists, sizeof(cameraExists));
	}

	if ( cameraExists == 1)
	{
		rpr_camera camera = LoadCamera(context, myfile);
		if ( camera == NULL )
		{
			ErrorDetected();
			return 0;
		}
		status = rprSceneSetCamera(scene, camera);
		CHECK_STATUS_RETURNNULL;
	}

	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SCENE_END)
	{
		ErrorDetected();
		return 0;
	}

	status = rprContextSetScene(context, scene);
	CHECK_STATUS_RETURNNULL;

	return scene;
}

rpr_int FRS6::StoreShader(rpr_material_node shader, std::ofstream *myfile)
{

	rpr_int status = RPR_SUCCESS;

	myfile->write((const char*)&m_SHADER_BEG, sizeof(m_SHADER_BEG));

	rpr_material_node_type nodeType = 0;
	status = rprMaterialNodeGetInfo(shader,RPR_MATERIAL_NODE_TYPE, sizeof(nodeType),&nodeType,NULL);
	CHECK_STATUS_RETURNNULL;
	myfile->write((char*)&nodeType, sizeof(nodeType));

	uint64_t nbInput = 0;
	status = rprMaterialNodeGetInfo(shader,RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(nbInput),&nbInput,NULL);
	CHECK_STATUS_RETURNNULL;
	myfile->write((char*)&nbInput, sizeof(nbInput));
		

	for (int i = 0; i < nbInput; i++)
	{
		size_t shaderParameterName_lenght_size_t = 0;
		status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_NAME, 0, NULL, &shaderParameterName_lenght_size_t);
		uint64_t shaderParameterName_lenght = shaderParameterName_lenght_size_t;
		CHECK_STATUS_RETURNNULL;
		char* shaderParameterName = new char[shaderParameterName_lenght];
		status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_NAME, shaderParameterName_lenght, shaderParameterName, NULL);
		CHECK_STATUS_RETURNNULL;
		myfile->write((char*)&shaderParameterName_lenght, sizeof(shaderParameterName_lenght));
		myfile->write((char*)shaderParameterName, shaderParameterName_lenght);

		rpr_uint nodeInputType = 0;
		status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(nodeInputType), &nodeInputType, NULL);
		CHECK_STATUS_RETURNNULL;
		myfile->write((char*)&nodeInputType, sizeof(nodeInputType));

		size_t shaderParameterValue_lenght_size_t = 0;
		status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_VALUE, 0, NULL, &shaderParameterValue_lenght_size_t);
		uint64_t shaderParameterValue_lenght = shaderParameterValue_lenght_size_t;
		CHECK_STATUS_RETURNNULL;
		char* shaderParameterValue = new char[shaderParameterValue_lenght];
		status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_VALUE, shaderParameterValue_lenght, shaderParameterValue, NULL);
		CHECK_STATUS_RETURNNULL;


		if ( nodeInputType == RPR_MATERIAL_NODE_INPUT_TYPE_NODE )
		{
			if (shaderParameterValue_lenght != sizeof(rpr_material_node))
			{
				ErrorDetected();
				return RPR_ERROR_INTERNAL_ERROR;
			}
			else
			{
				rpr_material_node* shader = (rpr_material_node*)shaderParameterValue;
				if (*shader != NULL)
				{
					int32_t shaderExists = 1;
					myfile->write((char*)&shaderExists, sizeof(shaderExists));
					status = StoreShader(*shader, myfile);
				}
				else
				{
					int32_t shaderExists = 0;
					myfile->write((char*)&shaderExists, sizeof(shaderExists));
				}
				CHECK_STATUS_RETURNERROR;
			}
		}
		else if (nodeInputType == RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
		{
			if (shaderParameterValue_lenght != sizeof(rpr_image))
			{
				ErrorDetected();
				return RPR_ERROR_INTERNAL_ERROR;
			}
			else
			{
				rpr_image* image = (rpr_material_node*)shaderParameterValue;
				if (*image != NULL)
				{
					int32_t imageExists = 1;
					myfile->write((char*)&imageExists, sizeof(imageExists));
					status = StoreImage(*image, myfile);
				}
				else
				{
					int32_t imageExists = 0;
					myfile->write((char*)&imageExists, sizeof(imageExists));
				}
				CHECK_STATUS_RETURNERROR;
			}
		}
		else
		{
			myfile->write((char*)&shaderParameterValue_lenght, sizeof(shaderParameterValue_lenght));
			myfile->write((char*)shaderParameterValue, shaderParameterValue_lenght);
		}

		delete[] shaderParameterName; shaderParameterName = NULL;
		delete[] shaderParameterValue; shaderParameterValue = NULL;
	}


	myfile->write((const char*)&m_SHADER_END, sizeof(m_SHADER_END));

	return RPR_SUCCESS;
}


rpr_material_node FRS6::LoadShader(rpr_material_system materialSystem, rpr_context context, std::istream *myfile)
{
	rpr_int status = RPR_SUCCESS;

	int32_t checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SHADER_BEG)
	{
		ErrorDetected();
		return 0;
	}

	rpr_material_node_type shaderType;
	myfile->read((char*)&shaderType, sizeof(rpr_material_node_type)); 

	rpr_material_node  shader = NULL;
	status = rprMaterialSystemCreateNode(materialSystem, shaderType, &shader);
	CHECK_STATUS_RETURNNULL;

	uint64_t nbInput = 0;
	myfile->read((char*)&nbInput, sizeof(nbInput));


	for (int i = 0; i < nbInput; i++)
	{
		uint64_t paramName_length = 0;
		myfile->read((char*)&paramName_length, sizeof(paramName_length));
		char* paramName_data = new char[paramName_length];
		myfile->read((char*)paramName_data, paramName_length);

		rpr_parameter_type paramType;
		myfile->read((char*)&paramType, sizeof(paramType)); 

		

		if (paramType == RPR_MATERIAL_NODE_INPUT_TYPE_NODE)
		{
			int32_t shaderExists;
			myfile->read((char*)&shaderExists, sizeof(shaderExists));

			if (shaderExists == 1)
			{

				rpr_material_node newShader = LoadShader(materialSystem, context, myfile);
				if (newShader == 0)
				{
					ErrorDetected();
					return 0;
				}
				status = rprMaterialNodeSetInputN(shader, paramName_data, newShader);
				if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that

			}
			else
			{
				status = rprMaterialNodeSetInputN(shader, paramName_data, NULL);
				if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
			}

		}
		else if (paramType == RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
		{
			int32_t imageExists;
			myfile->read((char*)&imageExists, sizeof(imageExists));

			if (imageExists == 1)
			{

				rpr_material_node newImage = LoadImage(context, myfile);
				if (newImage == 0)
				{
					ErrorDetected();
					return 0;
				}
				status = rprMaterialNodeSetInputImageData(shader, paramName_data, newImage);
				if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that

			}
			else
			{
				status = rprMaterialNodeSetInputImageData(shader, paramName_data, NULL);
				if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
			}

		}
		else
		{
			uint64_t paramValue_length = 0;
			myfile->read((char*)&paramValue_length, sizeof(paramValue_length));
			char* paramValue_data = new char[paramValue_length];
			myfile->read((char*)paramValue_data, paramValue_length);

			if ( paramType == RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 )
			{
				if ( paramValue_length != 4*4 ) // check size is float4
				{ 
					ErrorDetected();
					return 0; 
				} 
				float* f4 = (float*)paramValue_data;
				status = rprMaterialNodeSetInputF(shader, paramName_data, f4[0],f4[1],f4[2],f4[3]);
				if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
			}
			else if ( paramType == RPR_MATERIAL_NODE_INPUT_TYPE_UINT )
			{
				if ( paramValue_length != 4 ) { ErrorDetected(); return 0; } // check size is UINT
				unsigned int* ui1 = (unsigned int*)paramValue_data;
				status = rprMaterialNodeSetInputU(shader, paramName_data, ui1[0]);
				if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that

			}
			else
			{
				ErrorDetected();
				return 0;
			}

			delete[] paramValue_data; paramValue_data = NULL;
		}

		delete[] paramName_data; paramName_data = NULL;
		//delete[] value_data; value_data = NULL;
	}


	checkCode = 0;
	myfile->read((char*)&checkCode, sizeof(checkCode));
	if (checkCode != m_SHADER_END)
	{
		ErrorDetected();
		return 0;
	}

	return shader;
}

void FRS6::ErrorDetected()
{

	//add breakpoint here for debugging frLoader
	int a=0;

}

void FRS6::WarningDetected()
{

	//add breakpoint here for debugging frLoader
	int a=0;

}


	
int asup_temp()
{


	return 0;
}