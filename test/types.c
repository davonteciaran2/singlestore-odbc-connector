#include "tap.h"

void is_ok(SQLRETURN err)
{
  if (err)
  {
    exit(1);
  }
}

int main(int argc, char **argv)
{
  // create env handler
  SQLHANDLE henv;
  is_ok(SQLAllocHandle(SQL_HANDLE_ENV, NULL, &henv));
  is_ok(SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                      (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER));

  // create connection handle
  SQLHANDLE hdbc;
  is_ok(SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc));
  is_ok(SQLDriverConnect(hdbc, NULL, (SQLCHAR *)"DRIVER=SingleStore ODBC Unicode Driver;SERVER=127.0.0.1;PORT=5506;PASSWORD=1;UID=root;DATABASE=db;NO_SSPS=0;OPTION=67108866;NO_CACHE=0", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT));

  // create statement handle
  SQLHANDLE hstmt;
  is_ok(SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt));
  printf("AAAAAAAA\n");
  fflush(stdout);
  SQLULEN maxRows = 10;
  CHECK_STMT_RC(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_MAX_ROWS, 10, 0));

  // execute multi statement
  CHECK_STMT_RC(hstmt, SQLPrepare(hstmt, (SQLCHAR *)"SELECT * FROM t FOR UPDATE", SQL_NTS));
  CHECK_STMT_RC(hstmt, SQLExecute(hstmt));
}