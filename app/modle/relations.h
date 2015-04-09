
#ifndef _RELATION_H_
#define _RELATION_H_

#define RELATION_BEGIN(ta)
/*typedef struct _tag_##ta##_has {  */

#define RELATION_HAS(tb)
/*        RT        *tb    */

#define RELATION_BELONG(tb)
#define RELATION_END(ta)
/* } ta##COM;                       */

RELATION_BEGIN(User         )
    RELATION_HAS(Town   )
    RELATION_HAS(Resource)
    RELATION_HAS(Army)
    RELATION_HAS(Item)
RELATION_END(User)

#endif
