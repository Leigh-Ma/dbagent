#include "table_pub.h"

int do_query(const char *sql, char **rows, int *num) {
    *rows = "|7|213|9|4|8497|baby|sygdy398798370|1.246747"
            "|7|219|4|6|3465|eyh|hbfherubbfrnfhb|1.478775";
    *num = 2;
    printf("%s\n", sql);
    return 0;
}

int main() {
    User *u = (User*)0;
    int num = 5, status;
    tables_init();
    tables_show();

    //find_rows_with_cond_with_tname("users", "" , &u, &num);
    //printf("num : %d\n", num);
    //table_rows_show("users", u, num);
    //table_rows_release("users", u, num);
    FIND(User, 213, &u, &num, status);
    table_rows_show("users", u, num);
    table_rows_release("users", u, num);
    return 0;
}
