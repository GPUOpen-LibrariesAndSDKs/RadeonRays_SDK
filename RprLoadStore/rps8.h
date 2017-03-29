#ifndef frLoadStore_RPS8_H_
#define frLoadStore_RPS8_H_

#include "Rpr/RadeonProRender.h"

#include "common.h"

#include <fstream>
#include <vector>
#include <map>
#include <stdint.h>


/*

Architecture of RPS file :

we store ELEMENTS.

there are 4 types of ELEMENTS :
- OBJECT_BEG  --> header of an OBJECT. an OBJECT is a set of PARAMETER and REFERENCE
- OBJECT_END  --> end of an OBJECT
- PARAMETER   --> constains one data (could be an integer, an image, a float4 ...etc...)
- REFERENCE   --> reference to an object  (for example, this avoid to define several time the same image)

The head of each ELEMENT is always the same :
 <int32 : RPS_ELEMENTS_TYPE> <int32 : Element Name string length in byte>  < char[] : Element Name String >


architecture of each ELEMENTS :

OBJECT_BEG :
<int32 = RPSRT_OBJECT_BEG> <int32 : object name string length in byte>  < char[] : object name String >   <int32 : object type string length in byte>  < char[] : object type string >     < int32 : ID >   

OBJECT_END :
<int32 = RPSRT_OBJECT_END> <int32 : string length in byte (usually = 0) >  < char[] : string (usually empty) >   

REFERENCE :
<int32 = RPSRT_REFERENCE> <int32 : reference name string length in byte>  < char[] : reference name String >    <int32 : reference type string length in byte>  < char[] : reference type string >     < int32 : ID >   

PARAMETER :
<int32 = RPSRT_PARAMETER> <int32 : parameter name string length in byte>  < char[] : parameter name String >   < int32 : RPS_PARAMETER_TYPE >  < int64 : Data size in Byte >  < char[] : data >

*/


class RPS8
{

public:

	RPS8(std::fstream* myfile, bool allowWrite);
	~RPS8();

	//call that before calling StoreEverything()
	void AddExtraCustomParam_int(const std::string& name, int value) { m_extraCustomParam_int[name]=value; }
	void AddExtraCustomParam_float(const std::string& name, float value) { m_extraCustomParam_float[name]=value; }

	const std::map<std::string, int>& GetExtraCustomParam_int() { return m_extraCustomParam_int; }
	const std::map<std::string, float>& GetExtraCustomParam_float() { return m_extraCustomParam_float; }

	// return RPR_SUCCESS is success
	rpr_int StoreEverything(
		rpr_context context, 
		rpr_scene scene
		);

	// return RPR_SUCCESS is success
	rpr_int LoadEverything(
		rpr_context context, 
		rpr_material_system materialSystem,
		rpr_scene& scene,
		bool useAlreadyExistingScene
		);

	// return RPR_SUCCESS is success
	// store a custom list of RPR objects.
	rpr_int StoreCustomList(
		const std::vector<rpr_material_node>& materialNode_List,
		const std::vector<rpr_camera>& cameraList,
		const std::vector<rpr_light>& lightList,
		const std::vector<rpr_shape>& shapeList,
		const std::vector<rpr_image>& imageList
		);

	rpr_int LoadCustomList(
		rpr_context context, 
		rpr_material_system materialSystem ,
		std::vector<rpr_image>& imageList,
		std::vector<rpr_shape>& shapeList,
		std::vector<rpr_light>& lightList,
		std::vector<rpr_camera>& cameraList,
		std::vector<rpr_material_node>& materialNodeList
		);


	const std::vector<RPS_OBJECT_DECLARED>& GetListObjectDeclared() { return m_listObjectDeclared; }
	

private:


	//in order to switch between rpr_parameter_type and RPS_PARAMETER_TYPE,  use RPSPT_to_RPRPARAMETERTYPE()  or   RPRPARAMETERTYPE_to_RPSPT()
	enum RPS_PARAMETER_TYPE  { 

		// do NOT modify the numbers because there would be a retro-compatibility issue with RPS already saved
		RPSPT_UNDEF    = 0, // undef data

		RPSPT_FLOAT1   = 1, 
		RPSPT_FLOAT2   = 2,
		RPSPT_FLOAT3   = 3, 
		RPSPT_FLOAT4   = 4, 
		RPSPT_FLOAT16   = 5, 

		RPSPT_UINT32_1    = 6, 
		RPSPT_UINT32_2    = 7, 
		RPSPT_UINT32_3    = 8, 
		RPSPT_UINT32_4    = 9, 

		RPSPT_INT32_1    = 10, 
		RPSPT_INT32_2    = 11, 
		RPSPT_INT32_3    = 12, 
		RPSPT_INT32_4    = 13, 

		RPSPT_UINT64_1    = 14, 
		RPSPT_UINT64_2    = 15, 
		RPSPT_UINT64_3    = 16, 
		RPSPT_UINT64_4    = 17, 

		RPSPT_INT64_1    = 18, 
		RPSPT_INT64_2    = 19, 
		RPSPT_INT64_3    = 20, 
		RPSPT_INT64_4    = 21, 

		RPSPT_FORCE_DWORD    = 0xffffffff
	};

	enum RPS_ELEMENTS_TYPE
	{
		RPSRT_UNDEF = 0,

		RPSRT_OBJECT_BEG = 0x0CC01,
		RPSRT_OBJECT_END = 0x0CC02,
		RPSRT_PARAMETER  = 0x0CC03,
		RPSRT_REFERENCE  = 0x0CC04,

		RPSRT_FORCE_DWORD    = 0xffffffff
	};

	//this is just for debug. advice to call this function each time an error is detected.
	//for debugging, we can add a breakpoint in this function
	void ErrorDetected(const char* function, int32_t line, const char* message);
	void WarningDetected();


	rpr_parameter_type RPSPT_to_RPRPARAMETERTYPE(RPS_PARAMETER_TYPE in);
	RPS_PARAMETER_TYPE RPRPARAMETERTYPE_to_RPSPT(rpr_parameter_type in);
	int32_t RPSPT_to_size(RPS_PARAMETER_TYPE in); // return -1 if no fixed size for the input argument

	//return false if FAIL
	bool Store_String( const std::string& str);

	// type = "rpr_shape" or "rpr_image" ...etc...
	// obj = corresponds to rpr_shape or rpr_image ...etc...
	bool Store_StartObject( const std::string& objName, const std::string& type, void* obj);

	bool Store_EndObject();

	bool Store_ObjectParameter( const std::string& parameterName,RPS_PARAMETER_TYPE type,uint64_t dataSize, const void* data);

	// [in] type = "rpr_shape" or "rpr_image" ...etc...
	bool Store_ReferenceToObject(const std::string& objName, const std::string& type, int32_t id);

	rpr_int Store_Context(rpr_context context);// return RPR_SUCCESS is success
	rpr_int Store_Scene(rpr_scene scene);// return RPR_SUCCESS is success
	rpr_int Store_Shape(rpr_shape shape, const std::string& name); // return RPR_SUCCESS is success
	rpr_int Store_Light(rpr_light light, const std::string& name);// return RPR_SUCCESS is success
	rpr_int Store_Camera(rpr_camera camera, const std::string& name_);// return RPR_SUCCESS is success
	rpr_int Store_MaterialNode(rpr_material_node shader, const std::string& name);// return RPR_SUCCESS is success
	rpr_int Store_Image(rpr_image image, const std::string& name);// return RPR_SUCCESS is success


	rpr_int Read_Context(rpr_context context);// return RPR_SUCCESS is success
	rpr_scene Read_Scene(rpr_context context, rpr_material_system materialSystem, rpr_scene sceneExisting);//return NULL if error.   If sceneExisting not NULL, the we use this scene. Else we create a new scene.
	rpr_light Read_Light(rpr_context context, rpr_scene scene, rpr_material_system materialSystem);//return NULL if error
	rpr_image Read_Image(rpr_context context);//return NULL if error
	rpr_shape Read_Shape(rpr_context context, rpr_material_system materialSystem);//return NULL if error
	rpr_material_node Read_MaterialNode(rpr_material_system materialSystem, rpr_context context);//return NULL if error
	rpr_camera Read_Camera(rpr_context context);//return NULL if error

	// [out] name       : name of the next element
	// [out] objBegType : only filled if next element is RPSRT_OBJECT_BEG or RPSRT_REFERENCE
	// return RPSRT_UNDEF if error or if end of file
	RPS_ELEMENTS_TYPE Read_whatsNext(std::string& name, std::string& objBegType); 

	rpr_int Read_Element_StartObject(std::string& name, std::string& type, int32_t& id);  // return RPR_SUCCESS if success
	
	// [in] type = "rpr_shape" or "rpr_image" ...etc...
	// obj = corresponds to rpr_shape or rpr_image ...etc...
	rpr_int Read_Element_EndObject(const std::string& type, void* obj, int32_t id);  // return RPR_SUCCESS if success
	
	rpr_int Read_Element_Parameter(std::string& name, RPS_PARAMETER_TYPE& type, uint64_t& dataSize);  // return RPR_SUCCESS if success
	rpr_int Read_Element_ParameterData(void* data, uint64_t size); // return RPR_SUCCESS if success
	rpr_int Read_Element_Reference(std::string& name, std::string& type, RPS_OBJECT_DECLARED& objReferenced);  // return RPR_SUCCESS if success
	rpr_int Read_String(std::string& str);  // return RPR_SUCCESS if success

	//should be incremented each time we update the store/load
	//version will be write just after the m_HEADER_CHECKCODE
	static const int32_t m_FILE_VERSION;

	//first 4 bytes of the file.
	//will never change.
	//just tell that it's a FireRender Scene file
	static const char m_HEADER_CHECKCODE[4];
	static const char m_HEADER_BADCHECKCODE[4]; // bad checkcode, indicates that RPS file is not correct.


	int32_t m_level; // increment when enter inside Store_StartObject/Read_Element_ObjBeg.  decrement when Store_EndObject/Read_Element_ObjEnd

	std::fstream* m_rpsFile;

	int32_t m_idCounter; // increment each time we declare an object

	

	std::vector<RPS_OBJECT_DECLARED> m_listObjectDeclared;

	int32_t m_fileVersionOfLoadFile;

	std::map<std::string, int> m_extraCustomParam_int;
	std::map<std::string, float> m_extraCustomParam_float;

	const bool m_allowWrite; // = true if allow write the m_frsFile fstream  -   = false if Read Only

};



#endif
