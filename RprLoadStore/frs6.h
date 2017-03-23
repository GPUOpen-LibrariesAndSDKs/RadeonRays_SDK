#ifndef frLoadStore_FRS6_H_
#define frLoadStore_FRS6_H_

#include <Rpr/RadeonProRender.h>

#include <fstream>
#include <vector>
#include <map>
#include <stdint.h>



// FRS6 is an old version. I keep it here for legacy purpose.
// the version 6 is not supposed to be used anymore
class FRS6
{

public:

	//when we store a scene from 3DS max, we store additionnal data.
	//data is contain in this struct
	struct LOADER_DATA_FROM_3DS
	{
		// added in version 04 :
		bool used; // if 'used' is false, all other parameters of this struct are useless 
		float exposure;
		bool isNormals;
		bool shouldToneMap;

		// added in version 06 :
		int32_t gamma_Enabled; // 0 or 1
		float   gamma_disp;
		float   gamma_fileIn;
		float   gamma_fileOut;

	};

	// return RPR_SUCCESS is success
	static rpr_int StoreEverything(
		rpr_context context, 
		rpr_scene scene,
		const LOADER_DATA_FROM_3DS* dataFrom3DS, // <--- can be NULL if not used
		std::ofstream *myfile);
	
	//  
	// example :
	//   rpr_context context = rprCreateContext(...); <-- must not be null, else return fail
	//   rpr_scene scene = 0; <--- must be NULL. else function returns fail.
	//   rpr_material_system matsys = NULL;
	//   rprContextCreateMaterialSystem(context, 0, &matsys);
	//   LoadEverything( context, matsys, myfile, scene, NULL, false );
	//
	// return RPR_SUCCESS is success
	static rpr_int LoadEverything(
		rpr_context context, 
		rpr_material_system materialSystem,
		std::istream *myfile, 
		rpr_scene& scene,
		LOADER_DATA_FROM_3DS* dataFrom3DS, // <--- can be NULL if not used
		bool useAlreadyExistingScene
		);

private:

	static rpr_int StoreCamera(rpr_camera camera, std::ofstream *myfile);// return RPR_SUCCESS is success
	static rpr_camera LoadCamera(rpr_context context, std::istream *myfile);//return NULL if error
	static rpr_int StoreShader(rpr_material_node shader, std::ofstream *myfile);// return RPR_SUCCESS is success
	static rpr_material_node LoadShader(rpr_material_system materialSystem, rpr_context context, std::istream *myfile);//return NULL if error
	static rpr_int StoreShape(rpr_shape shape, std::ofstream *myfile); // return RPR_SUCCESS is success
	static rpr_shape LoadShape(rpr_context context, rpr_material_system materialSystem, std::istream *myfile);//return NULL if error
	static rpr_int StoreScene(rpr_scene scene, std::ofstream *myfile);// return RPR_SUCCESS is success
	static rpr_scene LoadScene(rpr_context context, rpr_material_system materialSystem, std::istream *myfile, rpr_scene sceneExisting);//return NULL if error.   If sceneExisting not NULL, the we use this scene. Else we create a new scene.
	static rpr_int StoreLight(rpr_light light, rpr_scene scene, std::ofstream *myfile);// return RPR_SUCCESS is success
	static rpr_light LoadLight(rpr_context context, rpr_scene scene, std::istream *myfile);//return NULL if error
	static rpr_int StoreImage(rpr_image image, std::ofstream *myfile);// return RPR_SUCCESS is success
	static rpr_image LoadImage(rpr_context context, std::istream *myfile);//return NULL if error
	static rpr_int StoreContext(rpr_context context, std::ofstream *myfile);// return RPR_SUCCESS is success
	static rpr_int LoadContext(rpr_context context, std::istream *myfile);// return RPR_SUCCESS is success

	//this is just for debug. advice to call this function each time an error is detected.
	//for debugging, we can add a breakpoint in this function
	static void ErrorDetected();
	static void WarningDetected();


	//just for more safety, the data is encapsulated around keywords.
	//so we can check that we are reading correct data
	static const int32_t m_SHAPE_BEG;
	static const int32_t m_SHAPE_END;
	static const int32_t m_CAMERA_BEG;
	static const int32_t m_CAMERA_END;
	static const int32_t m_LIGHT_BEG;
	static const int32_t m_LIGHT_END;
	static const int32_t m_SHADER_BEG;
	static const int32_t m_SHADER_END;
	static const int32_t m_SCENE_BEG;
	static const int32_t m_SCENE_END;
	static const int32_t m_CONTEXT_BEG;
	static const int32_t m_CONTEXT_END;
	static const int32_t m_EVERYTHING_BEG;
	static const int32_t m_EVERYTHING_END;
	static const int32_t m_IMAGE_BEG;
	static const int32_t m_IMAGE_END;
	static const int32_t m_3DSDATA_BEG;
	static const int32_t m_3DSDATA_END;

	//first 4 bytes of the file.
	//will never change.
	//just tell that it's a FireRender Scene file
	static const char m_HEADER_CHECKCODE[4];

	//should be incremented each time we update the store/load
	//version will be write just after the m_HEADER_CHECKCODE
	static const int32_t m_FILE_VERSION;

	static std::vector<rpr_shape> m_shapeList; // list of shape non-instancied

	static std::vector<rpr_image> m_imageList; //list to avoid to save several time the same image
	
	static int32_t m_fileVersionOfLoadFile;

};



#endif