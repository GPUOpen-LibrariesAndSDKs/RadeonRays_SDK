#ifndef frLoadStore_FRS7_H_
#define frLoadStore_FRS7_H_

#include "Rpr/RadeonProRender.h"

#include "common.h"

#include <fstream>
#include <vector>
#include <map>
#include <stdint.h>


/*

Architecture of FRS file :

we store ELEMENTS.

there are 4 types of ELEMENTS :
- OBJECT_BEG  --> header of an OBJECT. an OBJECT is a set of PARAMETER and REFERENCE
- OBJECT_END  --> end of an OBJECT
- PARAMETER   --> constains one data (could be an integer, an image, a float4 ...etc...)
- REFERENCE   --> reference to an object  (for example, this avoid to define several time the same image)

The head of each ELEMENT is always the same :
 <int32 : FRS_ELEMENTS_TYPE> <int32 : Element Name string length in byte>  < char[] : Element Name String >


architecture of each ELEMENTS :

OBJECT_BEG :
<int32 = FRSRT_OBJECT_BEG> <int32 : object name string length in byte>  < char[] : object name String >   <int32 : object type string length in byte>  < char[] : object type string >     < int32 : ID >   

OBJECT_END :
<int32 = FRSRT_OBJECT_END> <int32 : string length in byte (usually = 0) >  < char[] : string (usually empty) >   

REFERENCE :
<int32 = FRSRT_REFERENCE> <int32 : reference name string length in byte>  < char[] : reference name String >    <int32 : reference type string length in byte>  < char[] : reference type string >     < int32 : ID >   

PARAMETER :
<int32 = FRSRT_PARAMETER> <int32 : parameter name string length in byte>  < char[] : parameter name String >   < int32 : FRS_PARAMETER_TYPE >  < int64 : Data size in Byte >  < char[] : data >

*/


class FRS7
{

public:

	FRS7(std::fstream* myfile, bool allowWrite);
	~FRS7();

	//call that before calling StoreEverything()
	void AddExtraCustomParam_int(const std::string& name, int value) { m_extraCustomParam_int[name]=value; }
	void AddExtraCustomParam_float(const std::string& name, float value) { m_extraCustomParam_float[name]=value; }

	const std::map<std::string, int>& GetExtraCustomParam_int() { return m_extraCustomParam_int; }
	const std::map<std::string, float>& GetExtraCustomParam_float() { return m_extraCustomParam_float; }

	// return FR_SUCCESS is success
	fr_int StoreEverything(
		fr_context context, 
		fr_scene scene
		);

	// return FR_SUCCESS is success
	fr_int LoadEverything(
		fr_context context, 
		fr_material_system materialSystem,
		fr_scene& scene,
		bool useAlreadyExistingScene
		);


	
	const std::vector<RPS_OBJECT_DECLARED>& GetListObjectDeclared() { return m_listObjectDeclared; }
	

private:


	//in order to switch between fr_parameter_type and FRS_PARAMETER_TYPE,  use FRSPT_to_FRPARAMETERTYPE()  or   FRPARAMETERTYPE_to_FRSPT()
	enum FRS_PARAMETER_TYPE  { 

		// do NOT modify the numbers because there would be a retro-compatibility issue with FRS already saved
		FRSPT_UNDEF    = 0, // undef data

		FRSPT_FLOAT1   = 1, 
		FRSPT_FLOAT2   = 2,
		FRSPT_FLOAT3   = 3, 
		FRSPT_FLOAT4   = 4, 
		FRSPT_FLOAT16   = 5, 

		FRSPT_UINT32_1    = 6, 
		FRSPT_UINT32_2    = 7, 
		FRSPT_UINT32_3    = 8, 
		FRSPT_UINT32_4    = 9, 

		FRSPT_INT32_1    = 10, 
		FRSPT_INT32_2    = 11, 
		FRSPT_INT32_3    = 12, 
		FRSPT_INT32_4    = 13, 

		FRSPT_UINT64_1    = 14, 
		FRSPT_UINT64_2    = 15, 
		FRSPT_UINT64_3    = 16, 
		FRSPT_UINT64_4    = 17, 

		FRSPT_INT64_1    = 18, 
		FRSPT_INT64_2    = 19, 
		FRSPT_INT64_3    = 20, 
		FRSPT_INT64_4    = 21, 

		FRSPT_FORCE_DWORD    = 0xffffffff
	};

	enum FRS_ELEMENTS_TYPE
	{
		FRSRT_UNDEF = 0,

		FRSRT_OBJECT_BEG = 0x0CC01,
		FRSRT_OBJECT_END = 0x0CC02,
		FRSRT_PARAMETER  = 0x0CC03,
		FRSRT_REFERENCE  = 0x0CC04,

		FRSRT_FORCE_DWORD    = 0xffffffff
	};


	//this is just for debug. advice to call this function each time an error is detected.
	//for debugging, we can add a breakpoint in this function
	void ErrorDetected(const char* function, int32_t line, const char* message);
	void WarningDetected();


	fr_parameter_type FRSPT_to_FRPARAMETERTYPE(FRS_PARAMETER_TYPE in);
	FRS_PARAMETER_TYPE FRPARAMETERTYPE_to_FRSPT(fr_parameter_type in);
	int32_t FRSPT_to_size(FRS_PARAMETER_TYPE in); // return -1 if no fixed size for the input argument

	//return false if FAIL
	bool Store_String( const std::string& str);

	// type = "fr_shape" or "fr_image" ...etc...
	// obj = corresponds to fr_shape or fr_image ...etc...
	bool Store_StartObject( const std::string& objName, const std::string& type, void* obj);

	bool Store_EndObject();

	bool Store_ObjectParameter( const std::string& parameterName,FRS_PARAMETER_TYPE type,uint64_t dataSize, const void* data);

	// [in] type = "fr_shape" or "fr_image" ...etc...
	bool Store_ReferenceToObject(const std::string& objName, const std::string& type, int32_t id);

	fr_int Store_Context(fr_context context);// return FR_SUCCESS is success
	fr_int Store_Scene(fr_scene scene);// return FR_SUCCESS is success
	fr_int Store_Shape(fr_shape shape, const std::string& name); // return FR_SUCCESS is success
	fr_int Store_Light(fr_light light, fr_scene scene);// return FR_SUCCESS is success
	fr_int Store_Camera(fr_camera camera);// return FR_SUCCESS is success
	fr_int Store_MaterialNode(fr_material_node shader, const std::string& name);// return FR_SUCCESS is success
	fr_int Store_Image(fr_image image, const std::string& name);// return FR_SUCCESS is success


	fr_int Read_Context(fr_context context);// return FR_SUCCESS is success
	fr_scene Read_Scene(fr_context context, fr_material_system materialSystem, fr_scene sceneExisting);//return NULL if error.   If sceneExisting not NULL, the we use this scene. Else we create a new scene.
	fr_light Read_Light(fr_context context, fr_scene scene, fr_material_system materialSystem);//return NULL if error
	fr_image Read_Image(fr_context context);//return NULL if error
	fr_shape Read_Shape(fr_context context, fr_material_system materialSystem);//return NULL if error
	fr_material_node Read_MaterialNode(fr_material_system materialSystem, fr_context context);//return NULL if error
	fr_camera Read_Camera(fr_context context);//return NULL if error

	// [out] name       : name of the next element
	// [out] objBegType : only filled if next element is FRSRT_OBJECT_BEG or FRSRT_REFERENCE
	// return FRSRT_UNDEF if error or if end of file
	FRS_ELEMENTS_TYPE Read_whatsNext(std::string& name, std::string& objBegType); 

	fr_int Read_Element_StartObject(std::string& name, std::string& type, int32_t& id);  // return FR_SUCCESS if success
	
	// [in] type = "fr_shape" or "fr_image" ...etc...
	// obj = corresponds to fr_shape or fr_image ...etc...
	fr_int Read_Element_EndObject(const std::string& type, void* obj, int32_t id);  // return FR_SUCCESS if success
	
	fr_int Read_Element_Parameter(std::string& name, FRS_PARAMETER_TYPE& type, uint64_t& dataSize);  // return FR_SUCCESS if success
	fr_int Read_Element_ParameterData(void* data, uint64_t size); // return FR_SUCCESS if success
	fr_int Read_Element_Reference(std::string& name, std::string& type, RPS_OBJECT_DECLARED& objReferenced);  // return FR_SUCCESS if success
	fr_int Read_String(std::string& str);  // return FR_SUCCESS if success

	//should be incremented each time we update the store/load
	//version will be write just after the m_HEADER_CHECKCODE
	static const int32_t m_FILE_VERSION;

	//first 4 bytes of the file.
	//will never change.
	//just tell that it's a FireRender Scene file
	static const char m_HEADER_CHECKCODE[4];
	static const char m_HEADER_BADCHECKCODE[4]; // bad checkcode, indicates that FRS file is not correct.


	int32_t m_level; // increment when enter inside Store_StartObject/Read_Element_ObjBeg.  decrement when Store_EndObject/Read_Element_ObjEnd

	std::fstream* m_frsFile;

	int32_t m_idCounter; // increment each time we declare an object

	

	std::vector<RPS_OBJECT_DECLARED> m_listObjectDeclared;

	int32_t m_fileVersionOfLoadFile;

	std::map<std::string, int> m_extraCustomParam_int;
	std::map<std::string, float> m_extraCustomParam_float;

	const bool m_allowWrite; // = true if allow write the m_frsFile fstream  -   = false if Read Only

};



#endif
