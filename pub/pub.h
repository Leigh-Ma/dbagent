#ifndef _AGT_PUB_H_
#define _AGT_PUB_H_

#define _CHECK_RET(should_be_true, ret)				\
	if(!(should_be_true)) {							\
		return ret;									\
	}

#define _CHECK_RET_EX(should_be_true, ret, msg)		\
	if(!(should_be_true)) {							\
		logger(msg);								\
		return ret;									\
	}


#define _CHECK(should_be_true)						\
	if(!(should_be_true)) {							\
		return;										\
	}

#define _CHECK_EX(should_be_true, msg)				\
	if(!(should_be_true)) {							\
		logger(msg);								\
		return;										\
	}

#include "pub_types.h"
#include "pub_flags.h"


#endif
