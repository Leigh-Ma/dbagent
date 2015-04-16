
void dr_init(DB_DR *dr) {

    if(dr == (DB_DR *)0) {
       return;
    }
}

DB_CON *dr_new_connector(DB_DR *dr) {
    DB_CON *con;

    if(dr == (DB_DR *)0) {
        return (DB_CON *)0;
    }

    con = malloc(sizeof(DB_CON));
    if(con == (DB_CON *)0) {
        return (DB_CON *)0;
    }

    memset(con, 0x00, sizeof(DB_CON));
    con->iob = iob_alloc(8);
    if(con->iob == (DIOB*)0) {
        free(con);
        return (DB_CON*)0;
    }

   return dr;
}


