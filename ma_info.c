/************************************************************************************
   Copyright (C) 2013, 2017 MariaDB Corporation AB
                 2021 SingleStore, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/
#include <ma_odbc.h>

#define TYPE_INFO_FIELDS_COUNT 19
#define TYPES_COUNT 56

char* fieldNames[TYPE_INFO_FIELDS_COUNT] = {"TYPE_NAME", "DATA_TYPE", "COLUMN_SIZE", "LITERAL_PREFIX", "LITERAL_SUFFIX",
                                            "CREATE_PARAMS", "NULLABLE", "CASE_SENSITIVE", "SEARCHABLE", "UNSIGNED_ATTRIBUTE", "FIXED_PREC_SCALE",
                                            "AUTO_UNIQUE_VALUE", "LOCAL_TYPE_NAME", "MINIMUM_SCALE", "MAXIMUM_SCALE", "SQL_DATA_TYPE", "SQL_DATETIME_SUB",
                                            "NUM_PREC_RADIX", "INTERVAL_PRECISION"};

static const enum enum_field_types fieldTypes[TYPE_INFO_FIELDS_COUNT] = {MYSQL_TYPE_VAR_STRING, MYSQL_TYPE_SHORT, MYSQL_TYPE_LONG,
                                                            MYSQL_TYPE_VAR_STRING, MYSQL_TYPE_VAR_STRING, MYSQL_TYPE_VAR_STRING, MYSQL_TYPE_SHORT, MYSQL_TYPE_SHORT,
                                                            MYSQL_TYPE_SHORT, MYSQL_TYPE_SHORT, MYSQL_TYPE_SHORT, MYSQL_TYPE_SHORT, MYSQL_TYPE_VAR_STRING, MYSQL_TYPE_SHORT,
                                                            MYSQL_TYPE_SHORT, MYSQL_TYPE_SHORT, MYSQL_TYPE_SHORT, MYSQL_TYPE_LONG, MYSQL_TYPE_SHORT};

static const MADB_ShortTypeInfo gtiDefType[19]= {{SQL_VARCHAR, 0, SQL_TRUE, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_INTEGER, 0, 0, 0}, {SQL_VARCHAR, 0, SQL_TRUE, 0}, {SQL_VARCHAR, 0, SQL_TRUE, 0}, {SQL_VARCHAR, 0, SQL_TRUE, 0},
        /*7*/     {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0},
        /*11*/    {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_VARCHAR, 0, SQL_TRUE, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0},
        /*16*/    {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_INTEGER, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0} };


const MADB_TypeInfo* GetTypeInfo(SQLSMALLINT SqlType, MYSQL_FIELD *Field)
{
  int i;
  for (i = 0; i < TYPES_COUNT; ++i)
  {
    if (SqlType == TypeInfoV3[i].DataType && !strcmp(MADB_GetTypeName(Field), TypeInfoV3[i].TypeName))
    {
      return &TypeInfoV3[i];
    }
  }
  return NULL;
}

/* {{{ MADB_GetTypeInfo */
SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle,
                           SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  my_bool   isFirst= TRUE;
  char      StmtStr[10000];
  char      *p= StmtStr;
  int       i;
  const MADB_TypeInfo *TypeInfo= (Stmt->Connection->Environment->OdbcVersion == SQL_OV_ODBC2) ? TypeInfoV2 : TypeInfoV3;

  if (Stmt->Connection->Environment->OdbcVersion == SQL_OV_ODBC2)
  {
    /* We need to map time types */
    DataType = MapToV2Type(DataType);
  }

  StmtStr[0]= 0;
  for (i=0;i<TYPES_COUNT; i++)
  {
    if (DataType == SQL_ALL_TYPES ||
        TypeInfo[i].DataType == DataType)
    {
      if(isFirst)
      {
        isFirst= FALSE;
        p+= _snprintf(p, 10000 - strlen(StmtStr),
                      "SELECT \"%s\" AS TYPE_NAME, %d AS DATA_TYPE, %lu AS COLUMN_SIZE, \"%s\" AS LITERAL_PREFIX, "
                      "\"%s\" AS LITERAL_SUFFIX, %s AS CREATE_PARAMS, %d AS NULLABLE, %d AS CASE_SENSITIVE, "
                      "%d AS SEARCHABLE, %d AS UNSIGNED_ATTRIBUTE, %d AS FIXED_PREC_SCALE, %d AS AUTO_UNIQUE_VALUE, "
                      "\"%s\" AS LOCAL_TYPE_NAME, %d AS MINIMUM_SCALE, %d AS MAXIMUM_SCALE, "
                      "%d AS SQL_DATA_TYPE, "
                      "%d AS SQL_DATETIME_SUB, %d AS NUM_PREC_RADIX, %d AS INTERVAL_PRECISION ",
                      TypeInfo[i].TypeName,TypeInfo[i].DataType,TypeInfo[i].ColumnSize,TypeInfo[i].LiteralPrefix,
                      TypeInfo[i].LiteralSuffix,TypeInfo[i].CreateParams,TypeInfo[i].Nullable,TypeInfo[i].CaseSensitive,
                      TypeInfo[i].Searchable,TypeInfo[i].Unsigned,TypeInfo[i].FixedPrecScale,TypeInfo[i].AutoUniqueValue,
                      TypeInfo[i].LocalTypeName,TypeInfo[i].MinimumScale,TypeInfo[i].MaximumScale,
                      TypeInfo[i].SqlDataType,
                      TypeInfo[i].SqlDateTimeSub,TypeInfo[i].NumPrecRadix, TypeInfo[i].IntervalPrecision);
      }
      else
        p+= _snprintf(p, 10000 - strlen(StmtStr),
                      "UNION ALL SELECT \"%s\", %d, %lu , \"%s\", "
                      "\"%s\", %s, %d, %d, "
                      "%d, %d, %d, %d, "
                      "\"%s\", %d, %d, "
                      "%d, "
                      "%d, %d, %d ",
                      TypeInfo[i].TypeName,TypeInfo[i].DataType,TypeInfo[i].ColumnSize,TypeInfo[i].LiteralPrefix,
                      TypeInfo[i].LiteralSuffix,TypeInfo[i].CreateParams,TypeInfo[i].Nullable,TypeInfo[i].CaseSensitive,
                      TypeInfo[i].Searchable,TypeInfo[i].Unsigned,TypeInfo[i].FixedPrecScale,TypeInfo[i].AutoUniqueValue,
                      TypeInfo[i].LocalTypeName,TypeInfo[i].MinimumScale,TypeInfo[i].MaximumScale,
                      TypeInfo[i].SqlDataType,
                      TypeInfo[i].SqlDateTimeSub,TypeInfo[i].NumPrecRadix, TypeInfo[i].IntervalPrecision);
    }
  }

  // If the DataType argument specifies a data type which is valid for the version of ODBC supported by the driver, but is not supported by the driver, then it will return an empty result set.
  if (!StmtStr[0])
  {
    p+= _snprintf(p, 10000 - strlen(StmtStr),
                  "SELECT \"\" AS TYPE_NAME, 0 AS DATA_TYPE, 0 AS COLUMN_SIZE, \"\" AS LITERAL_PREFIX, "
                  "\"\" AS LITERAL_SUFFIX, NULL AS CREATE_PARAMS, 0 AS NULLABLE, 0 AS CASE_SENSITIVE, "
                  "0 AS SEARCHABLE, 0 AS UNSIGNED_ATTRIBUTE, 0 AS FIXED_PREC_SCALE, 0 AS AUTO_UNIQUE_VALUE, "
                  "\"\" AS LOCAL_TYPE_NAME, 0 AS MINIMUM_SCALE, 0 AS MAXIMUM_SCALE, "
                  "0 AS SQL_DATA_TYPE, "
                  "0 AS SQL_DATETIME_SUB, 0 AS NUM_PREC_RADIX, 0 AS INTERVAL_PRECISION LIMIT 0");
  }

  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
  {
    MADB_FixColumnDataTypes(Stmt, gtiDefType);
  }
  return ret;
}
/* }}} */
