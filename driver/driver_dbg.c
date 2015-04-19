#include "driver.h"

int main() {
    DB_DR *hdr = &agt_mysql_driver;
    DB_CFG cfg ={3306, "localhost","sparta_development", "root", ""};
    DB_CON *hdc;
    DB_REQ req = {0};
    DB_RESP resp = {0};

    _CHECK_RET_EX(
       0 == dr_init(hdr, &cfg),
       -1, "do init error\n"
    )

    _CHECK_RET_EX(
       hdc = dr_new_connector(hdr),
       -1, "do new connector error\n"
    )

    _CHECK_RET_EX(
       0 == co_connect(hdc),
       -1, "do connect error\n"
    )

    req.sql = "select * from users;";

    _CHECK_RET_EX(
       0 == co_query(hdc, &req, &resp),
       -1, "do query error\n"
    )

    iob_probe(resp.iob);

    _CHECK_RET_EX(
       0 == co_close(hdc),
       -1, "do close error\n"
    )

    _CHECK_RET_EX(
       0 == dr_destroy(hdr),
       -1, "do destroy error\n"
    )


    return 0;
}
