#include "table_pub.h"

int do_query(const char *sql, char **rows, int *num) {
    if(strstr(sql, "users")) {
        *rows = "|7|213|9|4|8497|baby|sygdy398798370|1.246747"
            "|7|219|4|6|3465|eyh|hbfherubbfrnfhb|1.478775";
    } else if(strstr(sql, "towns")) {
        *rows = "|9|6|213|9|63|29|256|mytown|wild|usertown"
                "|9|12|213|9|64|29|256|histown|home|usertown";
    }
    *num = 2;
    printf("%s\n", sql);
    return 0;
}

UINT8 the_user(TCOM *tc) {
    User *user = tc->record;
    return user->id == 219;
}

int main() {
    LCOM *user;
    TCOM *u = (TCOM*)0;
    int num = 5, status;
    TCOM *town;

    tables_init();
    //tables_show();

    //find_rows_with_cond_with_tname("users", "" , &u, &num);
    //printf("num : %d\n", num);
    //table_rows_show("users", u, num);
    //table_rows_release("users", u, num);
    //FIND(User, 213, &u, &num, status);
    //table_rows_show("users", u, num);
    //table_rows_release("users", u, num);
    for(num = 1; num <= 1; num ++) {
       user = leader_com_create_by_condition("users", "id = 213");
       table_com_show_data(user->leader);
       u =  table_com_find(user->leader, the_user);
       row_com_show_data(u);
       row_save(u->ti, u->record);
       row_insert(u->ti, u->record);
       town = row_com_has_table_com(user->leader, "towns", (char*)0);
       table_com_show_data(town);
       row_com_reload_data(town->next);
       table_com_destroy(user->leader);
       free(user);
       if(0 == (num % 1)) {
           printf("//////////////////run for %d times\n\n\n", num);
           sleep(3);
       }
    }
    //scanf("%d %d", &num, &status);
    return 0;
}
