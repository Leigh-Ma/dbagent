#ifndef _PUB_ERROR_H_
#define _PUB_ERROR_H_
/* error < 0 for system call errors */
/* error > 0 for user space  errors */
#define ERR_SYS_MEM         -1          /* system allocate memory error     */


#define ERR_MEM             -51         /* error occur when use memory      */
#define ERR_PARAM           -52         /* invalid parameters               */
#define ERR_STATE           -53         /* error found for state machines   */

#define ERR_FLD_NUM        -101         /* field number error               */
#define ERR_FLD_LEN        -102         /* field data length error          */
#define ERR_FLD_TYPE       -103         /* field type error                 */
#define ERR_FLD_NAME       -104         /* field name error, not found      */
#define ERR_FLD_SQL        -105         /* field name sql string error      */

#define ERR_TBL_NO         -111         /* invalid table no.                */
#define ERR_TBL_NAME       -112         /* invalid table name, not found    */
#define ERR_TBL_DATA       -113         /* invalid table data, not found    */

#define ERR_ROW_LEN        -121          /* table row data length error     */

#define ERR_SQL_LEN        -131          /* SQL length error(too long)      */

#define ERR_DR_CONNECT     -501          /* driver: connect database error  */
#define ERR_DR_QUERY       -502          /* driver: query error             */
#define ERR_DR_RESULT      -503          /* driver: deal with result error  */
#define ERR_DR_ROWS        -504          /* driver: affected rows error     */

#define ERR_IOB_FULL       -801          /* iob is full                     */
#define ERR_IOB_FLAG       -802          /* error iob flag                  */
#define ERR_IOB_EMPTY      -803          /* iob has no data at all          */

#define ERR_JOB_JID        -901          /* job jid error(not found)        */
#define ERR_JOB_MEM        -902          /* job memory error                */
#define ERR_JOB_SCOK       -903          /* job socket pair error           */
#define ERR_JOB_CREATE     -904          /* job create thread error         */
#define ERR_JOB_DATA       -905          /* job data error for base entry   */
#define ERR_JTM_PARAM      -941          /* job timer params error          */
#define ERR_JTM_CREATE     -942          /* job timer create error          */

#define ERR_JEV_BASE       -951          /* job event base init error       */
#define ERR_JEV_ADD        -952          /* event add error                 */
#endif
