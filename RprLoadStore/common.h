#ifndef frLoadStore_common_H_
#define frLoadStore_common_H_

#include <string>

struct RPS_OBJECT_DECLARED
{
	int32_t id;

	// "fr_shape" or "fr_image" ...etc...
	// or
	// "rpr_shape" or "rpr_image" ...etc...
	std::string type; 
	
	
	void* obj; // corresponds to rpr_shape or rpr_image ...etc...
};

#endif