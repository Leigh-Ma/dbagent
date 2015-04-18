#ifndef _ROWS_H_
#define _ROWS_H_

#define ROW_SQL_BUFF_LEN                                     2048

#define _save_begin(tf)                                         \
    switch((tf)->type) {                                        \

#define _save_end(tf)                                           \
    default:                                                    \
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = *error* \n", (tf)->name);                  \
        return ERR_FLD_TYPE;                                    \
    }

#define _save_try(type, tf, p, fmt)                             \
    case FT_##type :                                            \
    {                                                           \
        type val = *((type *)(p));                              \
        if(i + 1 == ti->tfn) {                                  \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = "fmt" ",  (tf)->name,  val);           \
        } else {                                                \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = "fmt", ", (tf)->name,  val);           \
        }                                                       \
    }                                                           \
    break;

#define _save_try_string(type, tf, p, fmt)                      \
    case FT_##type :                                            \
    {                                                           \
        if(i + 1 == ti->tfn) {                                  \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = \""fmt"\" ",  (tf)->name, (char*)(p)); \
        } else {                                                \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = \""fmt"\", ", (tf)->name, (char*)(p)); \
        }                                                       \
    }                                                           \
    break;

#define _save_try_far_string(type, tf, p , fmt)                 \
    case FT_##type :                                            \
    {                                                           \
        if(i + 1 == ti->tfn) {                                  \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = \""fmt"\" ",  (tf)->name, *(char**)(p));\
        } else {                                                \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = \""fmt"\", ", (tf)->name, *(char**)(p));\
        }                                                       \
    }                                                           \
    break;





#define _show_begin(tf)                                         \
    switch((tf)->type) {                                        \

#define _show_end(tf)                                           \
    default:                                                    \
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s: *error*, ", (tf)->name);                    \
        break;                                                  \
    }

#define _show_try(type, tf, p, fmt)                             \
    case FT_##type :                                            \
    {                                                           \
        type val = *((type *)(p));                              \
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s: "fmt", ", (tf)->name,  val);                \
    }                                                           \
    break;

#define _show_try_string(type, tf, p , fmt)                     \
    case FT_##type :                                            \
    {                                                           \
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s: \""fmt"\", ", (tf)->name, (char*) (p));     \
    }                                                           \
    break;

#define _show_try_far_string(type, tf, p , fmt)                 \
    case FT_##type :                                            \
    {                                                           \
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s: \""fmt"\", ", (tf)->name, *(char**)(p));    \
    }                                                           \
    break;




#define _insert_begin(tf)                                       \
    switch((tf)->type) {                                        \

#define _insert_end(tf)                                         \
    default:                                                    \
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "%s = *error* \n", (tf)->name);                  \
        return ERR_FLD_TYPE;                                    \
    }

#define _insert_try(type, tf, p, fmt)                           \
    case FT_##type :                                            \
    {                                                           \
        type val = *((type *)(p));                              \
        if(i + 1 == ti->tfn) {                                  \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, fmt" ",  val);                               \
        } else {                                                \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, fmt", ", val);                               \
        }                                                       \
    }                                                           \
    break;

#define _insert_try_string(type, tf, p, fmt)                    \
    case FT_##type :                                            \
    {                                                           \
        if(i + 1 == ti->tfn) {                                  \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "\""fmt"\" ",  (char*)(p));                  \
        } else {                                                \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "\""fmt"\", ", (char*)(p));                  \
        }                                                       \
    }                                                           \
    break;

#define _insert_try_far_string(type, tf, p , fmt)               \
    case FT_##type :                                            \
    {                                                           \
        if(i + 1 == ti->tfn) {                                  \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "\""fmt"\" ",  *(char**)(p));                \
        } else {                                                \
            len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "\""fmt"\", ", *(char**)(p));                \
        }                                                       \
    }                                                           \
    break;

#endif
