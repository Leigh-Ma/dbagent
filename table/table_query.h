#ifndef _TABLE_QUERY_H_
#define _TABLE_QUERY_H_

#include "tables.h"

#undef  to_s
#define to_s(X) #X

#define FIND_BY_I(T, F)                \
int T##_find_by_##F (T *r, int F) {    \
	char sql[512];                     \
                                       \
	snprintf(sql, 512, "select * from %ss where %s = %d limit 1", to_lower(to_s(T)), to_s(F), F ); \
	if(do_query(sql)) {                \
		return -1;                     \
	}                                  \
	phrase(tf_##T, &r);                \
                                       \
    return 0;                          \
}

#define FIND_BY_C(T, F)                \
int T##_find_by_##F (T *r, char* F) {  \
	char sql[1024];                    \
                                       \
	snprintf(sql, 1024, "select * from %ss where %s = '%s' limit 1", to_lower(to_s(T)), to_s(F), F ); \
	if(do_query(sql)) {                \
		return -1;                     \
	}                                  \
	phrase(tf_##T, &r);                \
                                       \
	return 0;                          \
}


FIND_BY_I(User, id)
FIND_BY_C(User, name)


#undef to_s
#endif
