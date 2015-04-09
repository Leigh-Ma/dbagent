#ifndef _AGT_PUB_H_
#define _AGT_PUB_H_
#include <assert.h>
#include <stdlib.h>

#define logger  printf
#define debuger printf
#define _FUNCTION_PARAMETER_ERROR_LOG_              \
    "Parameters for %s error\n", __FUNCTION__


#define _CHECK_RET(should_be_true, ret)				\
	if(!(should_be_true)) {							\
		return ret;									\
	}

#define _CHECK_RET_EX(should_be_true, ret, msg)		\
	if(!(should_be_true)) {							\
		logger(msg);								\
		return ret;									\
	}

#define _CHECK_PARAMS_RET(should_be_true, ret)      \
     _CHECK_RET_EX(should_be_true, ret, _FUNCTION_PARAMETER_ERROR_LOG_)


#define _CHECK(should_be_true)						\
	if(!(should_be_true)) {							\
		return;										\
	}

#define _CHECK_EX(should_be_true, msg)				\
	if(!(should_be_true)) {							\
		logger(msg);								\
		return;										\
	}

#define _CHECK_PARAMS(should_be_true)               \
     _CHECK(should_be_true, _FUNCTION_PARAMETER_ERROR_LOG_)

#include "pub_types.h"
#include "pub_flags.h"


#endif
