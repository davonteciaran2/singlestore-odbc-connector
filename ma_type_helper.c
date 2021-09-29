/************************************************************************************
   Copyright (C) 2013, 2018 MariaDB Corporation AB
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
#include <mysql.h>
#include <stdint.h>

/**
  Get the column size (in characters) of a field, as defined at:
    http://msdn2.microsoft.com/en-us/library/ms711786.aspx

  @param[in]  field

  @return  The column size of the field
*/
SQLINTEGER S2_GetColumnSize(MYSQL_FIELD *field, MADB_TypeInfo *odbc_type_info, const char *full_type_name)
{
  SQLINTEGER length;
  // cap at INT_MAX32 due to signed value in the ODBC spec
  if (field->length > INT32_MAX)
    length = INT32_MAX;
  else
    length = field->length;

  SQLSMALLINT char_size = 1;
  if (field->charsetnr != BINARY_CHARSETNR)
  {
    MARIADB_CHARSET_INFO *charset = mariadb_get_charset_by_nr(field->charsetnr);
    char_size = charset ? charset->char_maxlen : 1;
  }

  // types "datetime" and "datetime(6)" are not distinguishable by the
  // respective field info. 
  // it's 
  int is_datetime_adjustment_needed = (!strcmp(full_type_name, "datetime") || !strcmp(full_type_name, "timestamp")) ? 1 : 0;

  switch (field->type) {
  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_FLOAT:
  case MYSQL_TYPE_DOUBLE:
  case MYSQL_TYPE_NULL:
  case MYSQL_TYPE_LONGLONG:
  case MYSQL_TYPE_INT24:
  case MYSQL_TYPE_YEAR:
  case MYSQL_TYPE_DATE:
  case MYSQL_TYPE_TIME:
    return odbc_type_info->ColumnSize;
  case MYSQL_TYPE_NEWDATE:
  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_DATETIME:
    return odbc_type_info->ColumnSize - 7 * is_datetime_adjustment_needed;

  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
    return (length -
            (!(field->flags & UNSIGNED_FLAG) ? 1 : 0) - /* sign? */
            (field->decimals ? 1 : 0));             /* decimal point? */

  case MYSQL_TYPE_BIT:
    /*
      We treat a BIT(n) as a SQL_BIT if n == 1, otherwise we treat it
      as a SQL_BINARY, so length is (bits + 7) / 8.
    */
    if (length == 1)
      return 1;
    return (length + 7) / 8;

  case MYSQL_TYPE_GEOMETRY:
  case MYSQL_TYPE_ENUM:
  case MYSQL_TYPE_SET:
  case MYSQL_TYPE_VARCHAR:
  case MYSQL_TYPE_VAR_STRING:
  case MYSQL_TYPE_STRING:
    return length / char_size;
  case MYSQL_TYPE_BLOB:
  case MYSQL_TYPE_TINY_BLOB:
  case MYSQL_TYPE_MEDIUM_BLOB:
    return odbc_type_info->ColumnSize / char_size;
  case MYSQL_TYPE_LONG_BLOB:
    // NECESSARY EVIL: reported column size for long blob is 2GB instead of 4GB,
    // so we need to multiply the number of characters by 2 and add 1
    return odbc_type_info->ColumnSize / char_size * (char_size > 1 ? 2 : 1) + (char_size > 1 ? 1 : 0);
  }

  return SQL_NO_TOTAL;
}

/**
  Get the transfer octet length of a field, as defined at:
    http://msdn2.microsoft.com/en-us/library/ms713979.aspx

  @param[in]  field

  @return  The transfer octet length
*/
SQLLEN S2_GetCharacterOctetLength(MYSQL_FIELD *field, MADB_TypeInfo *odbc_type_info)
{
  SQLLEN length;
  /* cap at INT_MAX32 due to signed value */
  if (field->length > INT32_MAX)
    length = INT32_MAX;
  else
    length = field->length;

  switch (field->type) {
  case MYSQL_TYPE_TINY:
    return 1;

  case MYSQL_TYPE_SHORT:
    return 2;

  case MYSQL_TYPE_INT24:
    return 3;

  case MYSQL_TYPE_LONG:
    return 4;

  case MYSQL_TYPE_FLOAT:
    return 4;

  case MYSQL_TYPE_DOUBLE:
    return 8;

  case MYSQL_TYPE_NULL:
    return 1;

  case MYSQL_TYPE_LONGLONG:
    return 8;

  case MYSQL_TYPE_YEAR:
    return 2;

  case MYSQL_TYPE_DATE:
    return sizeof(SQL_DATE_STRUCT);

  case MYSQL_TYPE_TIME:
    return sizeof(SQL_TIME_STRUCT);

  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_DATETIME:
  case MYSQL_TYPE_NEWDATE:
    return sizeof(SQL_TIMESTAMP_STRUCT);

  case MYSQL_TYPE_BIT:
    /*
      We treat a BIT(n) as a SQL_BIT if n == 1, otherwise we treat it
      as a SQL_BINARY, so length is (bits + 7) / 8. field->length has
      the number of bits.
    */
    return (length + 7) / 8;

  case MYSQL_TYPE_ENUM:
  case MYSQL_TYPE_SET:
  case MYSQL_TYPE_VARCHAR:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_VAR_STRING:
  case MYSQL_TYPE_GEOMETRY:
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
    return length;
  case MYSQL_TYPE_TINY_BLOB:
  case MYSQL_TYPE_MEDIUM_BLOB:
  case MYSQL_TYPE_LONG_BLOB:
  case MYSQL_TYPE_BLOB:
    return odbc_type_info->ColumnSize;
  }

  return SQL_NO_TOTAL;
}

/**
  Get the decimal digits of a field, as defined at:
    https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/decimal-digits

  @param[in]  field

  @return  The decimal digits, or @c SQL_NO_TOTAL where it makes no sense

  The function has to return SQLSMALLINT, since it corresponds to SQL_DESC_SCALE
  or SQL_DESC_PRECISION for some data types.
*/
SQLSMALLINT S2_GetDecimalDigits(MYSQL_FIELD *field)
{
  switch (field->type)
  {
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIME:
      return field->decimals;

    /* All exact numeric types. */
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_LONGLONG:
    case MYSQL_TYPE_INT24:
      return 0;

    default:
      return SQL_NO_TOTAL;
  }
}
