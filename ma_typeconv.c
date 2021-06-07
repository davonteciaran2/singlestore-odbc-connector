/************************************************************************************
   Copyright (C) 2017 MariaDB Corporation AB
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

/* ODBC C->SQL and SQL->C type conversion functions */

#include <ma_odbc.h>

/* Borrowed from C/C and adapted */
SQLRETURN MADB_Str2Ts(const char *Str, size_t Length, MYSQL_TIME *Tm, BOOL Interval, MADB_Error *Error, BOOL *isTime)
{
  char *localCopy= MADB_ALLOC(Length + 1), *Start= localCopy, *Frac, *End= Start + Length;
  my_bool isDate= 0;

  if (Start == NULL)
  {
    return MADB_SetError(Error, MADB_ERR_HY001, NULL, 0);
  }

  memset(Tm, 0, sizeof(MYSQL_TIME));
  memcpy(Start, Str, Length);
  Start[Length]= '\0';

  while (Length && isspace(*Start)) ++Start, --Length;

  if (Length == 0)
  {
    goto end;//MADB_SetError(Error, MADB_ERR_22008, NULL, 0);
  }  

  if (Start[0]=='-')
  {
      Tm->neg = 1;
      Start++;
  }

  /* Determine time type:
  MYSQL_TIMESTAMP_DATE: [-]YY[YY].MM.DD
  MYSQL_TIMESTAMP_DATETIME: [-]YY[YY].MM.DD hh:mm:ss.mmmmmm
  MYSQL_TIMESTAMP_TIME: [-]hh:mm:ss.mmmmmm
  */
  if (strchr(Start, '-'))
  {
    if (sscanf(Start, "%d-%u-%u", &Tm->year, &Tm->month, &Tm->day) < 3)
    {
      return MADB_SetError(Error, MADB_ERR_22008, NULL, 0);
    }
    isDate= 1;
    if (!(Start= strchr(Start, ' ')))
    {
      goto check;
    }
  }
  if (!strchr(Start, ':'))
  {
    goto check;
  }

  if (isDate == 0)
  {
    *isTime= 1;
  }

  if ((Frac= strchr(Start, '.')) != NULL) /* fractional seconds */
  {
    size_t FracMulIdx= End - (Frac + 1) - 1/*to get index array index */;
    /* ODBC - nano-seconds */
    if (sscanf(Start, "%d:%u:%u.%6lu", &Tm->hour, &Tm->minute,
      &Tm->second, &Tm->second_part) < 4)
    {
      return MADB_SetError(Error, MADB_ERR_22008, NULL, 0);
    }
    /* 9 digits up to nano-seconds, and -1 since comparing with arr idx  */
    if (FracMulIdx < 6 - 1)
    {
      static unsigned long Mul[]= {100000, 10000, 1000, 100, 10};
      Tm->second_part*= Mul[FracMulIdx];
    }
  }
  else
  {
    if (sscanf(Start, "%d:%u:%u", &Tm->hour, &Tm->minute,
      &Tm->second) < 3)
    {
      return MADB_SetError(Error, MADB_ERR_22008, NULL, 0);
    }
  }

check:
  if (Interval == FALSE)
  {
    if (isDate)
    {
      if (Tm->year > 0)
      {
        if (Tm->year < 70)
        {
          Tm->year+= 2000;
        }
        else if (Tm->year < 100)
        {
          Tm->year+= 1900;
        }
      }
    }
  }

end:
  MADB_FREE(localCopy);
  return SQL_SUCCESS;
}

/* {{{ MADB_ConversionSupported */
BOOL MADB_ConversionSupported(MADB_DescRecord *From, MADB_DescRecord *To)
{
  switch (From->ConciseType)
  {
  case SQL_C_TIMESTAMP:
  case SQL_C_TYPE_TIMESTAMP:
  case SQL_C_TIME:
  case SQL_C_TYPE_TIME:
  case SQL_C_DATE:
  case SQL_C_TYPE_DATE:

    if (To->Type == SQL_INTERVAL)
    {
      return FALSE;
    }

  }
  return TRUE;
}
/* }}} */

/* {{{ MADB_ConvertCharToBit */
char MADB_ConvertCharToBit(MADB_Stmt *Stmt, char *src)
{
  char *EndPtr= NULL;
  float asNumber= strtof(src, &EndPtr);

  if (asNumber < 0 || asNumber > 1)
  {
    /* 22003 */
  }
  else if (asNumber != 0 && asNumber != 1)
  {
    /* 22001 */
  }
  else if (EndPtr != NULL && *EndPtr != '\0')
  {
    /* 22018. TODO: check if condition is correct */
  }

  return asNumber != 0 ? '\1' : '\0';
}
/* }}} */

/* {{{ MADB_ConvertNumericToChar */
size_t MADB_ConvertNumericToChar(SQL_NUMERIC_STRUCT *Numeric, char *Buffer, int *ErrorCode)
{
  long long Numerator= 0;
  long long Denominator= 1;
  unsigned long long Left= 0, Right= 0;
  int Scale= 0;
  int ppos= 0;
  long ByteDenominator= 1;
  int i;
  char *p;
  my_bool hasDot= FALSE;

  Buffer[0]= 0;
  *ErrorCode= 0;

  Scale+= (Numeric->scale < 0) ? -Numeric->scale : Numeric->scale;

  for (i=0; i < SQL_MAX_NUMERIC_LEN; ++i)
  {
    Numerator+= Numeric->val[i] * ByteDenominator;
    ByteDenominator<<= 8;
  }
  if (!Numeric->sign)
    Numerator= -Numerator;
  Denominator= (long long)pow(10, Scale);
  Left= Numerator / Denominator;
  //_i64toa_s(Numerator, Buffer, 38, 10);
  if (Numeric->scale > 0)
  {
    char tmp[38];
    _snprintf(tmp, 38, "%%.%df", Numeric->scale);
    _snprintf(Buffer, 38, tmp, Numerator / pow(10, Scale));
  }
  else
  {
    _snprintf(Buffer, 38, "%lld", Numerator);
    while (strlen(Buffer) < (size_t)(Numeric->precision - Numeric->scale))
      strcat(Buffer, "0");
  }


  if (Buffer[0] == '-')
    Buffer++;

  /* Truncation checks:
  1st ensure, that the digits before decimal point will fit */
  if ((p= strchr(Buffer, '.')))
  {
    if (p - Buffer - 1 > Numeric->precision)
    {
      *ErrorCode= MADB_ERR_22003;
      Buffer[Numeric->precision]= 0;
      goto end;
    }
    if (Numeric->scale > 0 && Left > 0 && (p - Buffer) + strlen(p) > Numeric->precision)
    {
      *ErrorCode= MADB_ERR_01S07;
      Buffer[Numeric->precision + 1]= 0;
      goto end;
    }
  }
  while (Numeric->scale < 0 && strlen(Buffer) < (size_t)(Numeric->precision - Numeric->scale))
    strcat(Buffer, "0");


  if (strlen(Buffer) > (size_t)(Numeric->precision + Scale) && Numeric->scale > 0)
    *ErrorCode= MADB_ERR_01S07;

end:
  /* check if last char is decimal point */
  if (strlen(Buffer) && Buffer[strlen(Buffer)-1] == '.')
    Buffer[strlen(Buffer)-1] = 0;
  if (!Numeric->sign)
    Buffer--;
  return strlen(Buffer);
}
/* }}} */

/* {{{ MADB_ConvertNullValue */
SQLRETURN MADB_ConvertNullValue(MADB_Stmt *Stmt, MYSQL_BIND *MaBind)
{
  MaBind->buffer_type=  MYSQL_TYPE_NULL;
  MaBind->buffer_length= 0;

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_ProcessIndicator */
/* Returns TRUE if indicator contains some special value, and thus no further type conversion is needed */
BOOL MADB_ProcessIndicator(MADB_Stmt *Stmt, SQLLEN Indicator, char * DefaultValue, MYSQL_BIND *MaBind)
{
  switch (Indicator)
  {
  case SQL_COLUMN_IGNORE:
    if (DefaultValue == NULL)
    {
      MADB_ConvertNullValue(Stmt, MaBind);
    }
    else
    {
      MaBind->buffer=       DefaultValue;
      MaBind->buffer_length= (unsigned long)strlen(DefaultValue);
      MaBind->buffer_type=  MYSQL_TYPE_STRING;
    }
    return TRUE;
  case SQL_NULL_DATA:
    MADB_ConvertNullValue(Stmt, MaBind);
    return TRUE;
  }

  return FALSE;
}
/* }}} */

/* {{{ MADB_CalculateLength */
SQLLEN MADB_CalculateLength(MADB_Stmt *Stmt, SQLLEN *OctetLengthPtr, MADB_DescRecord *CRec, void* DataPtr)
{
  /* If no OctetLengthPtr was specified, or OctetLengthPtr is SQL_NTS character
     are considered to be NULL binary data are null terminated */
  if (!OctetLengthPtr || *OctetLengthPtr == SQL_NTS)
  {
    /* Meaning of Buffer Length is not quite clear in specs. Thus we treat in the way, that does not break
        (old) testcases. i.e. we neglect its value if Length Ptr is specified */
    SQLLEN BufferLen= OctetLengthPtr ? -1 : CRec->OctetLength;

    switch (CRec->ConciseType)
    {
    case SQL_C_WCHAR:
      /* CRec->OctetLength eq 0 means not 0-length buffer, but that this value is not specified. Thus -1, for SqlwcsLen
          and SafeStrlen that means buffer len is not specified */
      return SqlwcsLen((SQLWCHAR *)DataPtr, BufferLen/sizeof(SQLWCHAR) - test(BufferLen == 0)) * sizeof(SQLWCHAR);
      break;
    case SQL_C_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
    case SQL_C_CHAR:
      return SafeStrlen((SQLCHAR *)DataPtr, BufferLen != 0 ? BufferLen : -1);
    }
  }
  else
  {
    return *OctetLengthPtr;
  }

  return CRec->OctetLength;
}
/* }}} */

/* {{{ MADB_GetBufferForSqlValue */
void* MADB_GetBufferForSqlValue(MADB_Stmt *Stmt, MADB_DescRecord *CRec, size_t Size)
{
  if (Stmt->RebindParams || CRec->InternalBuffer == NULL)
  {
    MADB_FREE(CRec->InternalBuffer);
    CRec->InternalBuffer= MADB_CALLOC(Size);
    if (CRec->InternalBuffer == NULL)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return NULL;
    }
  }

  return (void *)CRec->InternalBuffer;
}
/* }}} */

/* {{{ MADB_Wchar2Sql */
SQLRETURN MADB_Wchar2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  SQLULEN mbLength=0;

  MADB_FREE(CRec->InternalBuffer);

  /* conn cs ? */
  CRec->InternalBuffer= MADB_ConvertFromWChar((SQLWCHAR *)DataPtr, (SQLINTEGER)(Length / sizeof(SQLWCHAR)),
    &mbLength, &Stmt->Connection->Charset, NULL);

  if (CRec->InternalBuffer == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  *LengthPtr= (unsigned long)mbLength;
  *Buffer= CRec->InternalBuffer;

  MaBind->buffer_type=  MYSQL_TYPE_STRING;

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Char2Sql */
SQLRETURN MADB_Char2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  switch (SqlRec->Type)
  {
    case SQL_BIT:
      if (*Buffer == NULL)
      {
        CRec->InternalBuffer= (char *)MADB_GetBufferForSqlValue(Stmt, CRec, MaBind->buffer_length);

        if (CRec->InternalBuffer == NULL)
        {
          return Stmt->Error.ReturnValue;
        }
        *Buffer= CRec->InternalBuffer;
      }

      *LengthPtr= 1;
      **(char**)Buffer= *(char*)DataPtr;
      MaBind->buffer_type= MYSQL_TYPE_TINY;
      break;
  case SQL_DATETIME:
  {
    MYSQL_TIME Tm;
    SQL_TIMESTAMP_STRUCT Ts;
    BOOL isTime;

    /* Enforcing constraints on date/time values */
    RETURN_ERROR_OR_CONTINUE(MADB_Str2Ts(DataPtr, Length, &Tm, FALSE, &Stmt->Error, &isTime));
    MADB_CopyMadbTimeToOdbcTs(&Tm, &Ts);
    RETURN_ERROR_OR_CONTINUE(MADB_TsConversionIsPossible(&Ts, SqlRec->ConciseType, &Stmt->Error, MADB_ERR_22018, isTime));
    /* To stay on the safe side - still sending as string in the default branch */
  }
  default:
    /* Bulk shouldn't get here, thus logic for single paramset execution */
    *LengthPtr= (unsigned long)Length;
    *Buffer= DataPtr;
    MaBind->buffer_type= MYSQL_TYPE_STRING;
  }

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Numeric2Sql */
SQLRETURN MADB_Numeric2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  SQL_NUMERIC_STRUCT *p;
  int ErrorCode= 0;

  /* We might need to preserve this pointer to be able to later release the memory */
  CRec->InternalBuffer= (char *)MADB_GetBufferForSqlValue(Stmt, CRec, MADB_CHARSIZE_FOR_NUMERIC);

  if (CRec->InternalBuffer == NULL)
  {
    return Stmt->Error.ReturnValue;
  }

  p= (SQL_NUMERIC_STRUCT *)DataPtr;
  p->scale= (SQLSCHAR)SqlRec->Scale;
  p->precision= (SQLSCHAR)SqlRec->Precision;

  *LengthPtr= (unsigned long)MADB_ConvertNumericToChar((SQL_NUMERIC_STRUCT *)p, CRec->InternalBuffer, &ErrorCode);;
  *Buffer= CRec->InternalBuffer;

  MaBind->buffer_type= MYSQL_TYPE_STRING;

  if (ErrorCode)
  {
    /*TODO: I guess this parameters row should be skipped */
    return MADB_SetError(&Stmt->Error, ErrorCode, NULL, 0);
  }

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_TsConversionIsPossible */
SQLRETURN MADB_TsConversionIsPossible(SQL_TIMESTAMP_STRUCT *ts, SQLSMALLINT SqlType, MADB_Error *Error, enum enum_madb_error SqlState, int isTime)
{
  /* I think instead of MADB_ERR_22008 there should be also SqlState */
  switch (SqlType)
  {
  case SQL_TIME:
  case SQL_TYPE_TIME:
    if (ts->fraction)
    {
      return MADB_SetError(Error, MADB_ERR_22008, "Fractional seconds fields are nonzero", 0);
    }
    if (!VALID_TIME(ts))
    {
        return MADB_SetError(Error, MADB_ERR_22007, "Invalid time", 0);
    }
    break;
  case SQL_DATE:
  case SQL_TYPE_DATE:
    if (ts->hour + ts->minute + ts->second + ts->fraction)
    {
      return MADB_SetError(Error, MADB_ERR_22008, "Time fields are nonzero", 0);
    }
  default:
    /* This only would be good for SQL_TYPE_TIME. If C type is time(isTime!=0), and SQL type is timestamp, date fields may be NULL - driver should set them to current date */
    if ((isTime == 0 && ts->year == 0) || ts->month == 0 || ts->day == 0)
    {
      return MADB_SetError(Error, SqlState, NULL, 0);
    }
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Timestamp2Sql */
SQLRETURN MADB_Timestamp2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME           *tm= NULL;
  SQL_TIMESTAMP_STRUCT *ts= (SQL_TIMESTAMP_STRUCT *)DataPtr;

  RETURN_ERROR_OR_CONTINUE(MADB_TsConversionIsPossible(ts, SqlRec->ConciseType, &Stmt->Error, MADB_ERR_22007, 0));

  if (*Buffer == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *Buffer= tm;
  }
  else
  {
    tm= *Buffer;
  }
  

  /* Default types. Not quite clear if time_type has any effect */
  tm->time_type=       MYSQL_TIMESTAMP_DATETIME;
  MaBind->buffer_type= MYSQL_TYPE_TIMESTAMP;

  switch (SqlRec->ConciseType) {
  case SQL_TYPE_DATE:
  case SQL_DATE:
    if (ts->hour + ts->minute + ts->second + ts->fraction != 0)
    {
      return MADB_SetError(&Stmt->Error, MADB_ERR_22008, "Time fields are nonzero", 0);
    }

    MaBind->buffer_type= MYSQL_TYPE_DATE;
    tm->time_type=       MYSQL_TIMESTAMP_DATE;
    tm->year=  ts->year;
    tm->month= ts->month;
    tm->day=   ts->day;
    break;
  case SQL_TYPE_TIME:
  case SQL_TIME:
    if (ts->fraction != 0)
    {
      return MADB_SetError(&Stmt->Error, MADB_ERR_22008, "Fractional seconds fields are nonzero", 0);
    }
    
    if (!VALID_TIME(ts))
    {
      return MADB_SetError(&Stmt->Error, MADB_ERR_22007, "Invalid time", 0);
    }
    MaBind->buffer_type= MYSQL_TYPE_TIME;
    tm->time_type=       MYSQL_TIMESTAMP_TIME;
    tm->hour=   ts->hour;
    tm->minute= ts->minute;
    tm->second= ts->second;
    break;
  default:
    MADB_CopyOdbcTsToMadbTime(ts, tm);
  }

  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Time2Sql */
SQLRETURN MADB_Time2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME      *tm= NULL;
  SQL_TIME_STRUCT *ts= (SQL_TIME_STRUCT *)DataPtr;

  if ((SqlRec->ConciseType == SQL_TYPE_TIME || SqlRec->ConciseType == SQL_TYPE_TIMESTAMP ||
    SqlRec->ConciseType == SQL_TIME || SqlRec->ConciseType == SQL_TIMESTAMP || SqlRec->ConciseType == SQL_DATETIME) &&
    !VALID_TIME(ts))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_22007, NULL, 0);
  }

  if (*Buffer == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *Buffer= tm;
  }
  else
  {
    tm= *Buffer;
  }

  if(SqlRec->ConciseType == SQL_TYPE_TIMESTAMP ||
    SqlRec->ConciseType == SQL_TIMESTAMP || SqlRec->ConciseType == SQL_DATETIME)
  {
    /* Don't add the current local time logic.
    time_t sec_time;
    struct tm * cur_tm;

    sec_time= time(NULL);
    cur_tm= localtime(&sec_time);

    tm->year= 1900 + cur_tm->tm_year;
    tm->month= cur_tm->tm_mon + 1;
    tm->day= cur_tm->tm_mday;
    tm->second_part= 0;*/
    
    tm->time_type= MYSQL_TIMESTAMP_DATETIME;
    MaBind->buffer_type= MYSQL_TYPE_TIMESTAMP;
  }
  else
  {
    tm->year=  0;
    tm->month= 0;
    tm->day=   0;

    tm->time_type = MYSQL_TIMESTAMP_TIME;
    MaBind->buffer_type= MYSQL_TYPE_TIME;
  }

  tm->hour=   ts->hour;
  tm->minute= ts->minute;
  tm->second= ts->second;

  tm->second_part= 0;

  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_IntervalHtoMS2Sql */
SQLRETURN MADB_IntervalHtoMS2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME          *tm= NULL;
  SQL_INTERVAL_STRUCT *is= (SQL_INTERVAL_STRUCT *)DataPtr;

  if (*Buffer == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *Buffer= tm;
  }
  else
  {
    tm= *Buffer;
  }

  tm->hour=   is->intval.day_second.hour;
  tm->minute= is->intval.day_second.minute;
  tm->second= CRec->ConciseType == SQL_C_INTERVAL_HOUR_TO_SECOND ? is->intval.day_second.second : 0;

  tm->second_part= 0;

  tm->time_type= MYSQL_TIMESTAMP_TIME;
  MaBind->buffer_type= MYSQL_TYPE_TIME;
  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Date2Sql */
SQLRETURN MADB_Date2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME      *tm= NULL, **BuffPtr= (MYSQL_TIME**)Buffer;
  SQL_DATE_STRUCT *ts= (SQL_DATE_STRUCT *)DataPtr;

  if (*BuffPtr == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *BuffPtr= tm;
  }
  else
  {
    tm= *BuffPtr;
  }

  tm->year=  ts->year;
  tm->month= ts->month;
  tm->day=   ts->day;

  tm->hour= tm->minute= tm->second= tm->second_part= 0;
  tm->time_type= MYSQL_TIMESTAMP_DATE;

  MaBind->buffer_type= MYSQL_TYPE_DATE;
  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_ConvertC2Sql */
SQLRETURN MADB_ConvertC2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                            MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  if (Buffer == NULL)
  {
    MaBind->buffer= NULL;
    Buffer= &MaBind->buffer;
  }
  if (LengthPtr == NULL)
  {
    LengthPtr= &MaBind->buffer_length;
  }
  /* Switch to fill BIND structures based on C and SQL type */
  switch (CRec->ConciseType)
  {
  case WCHAR_TYPES:
    RETURN_ERROR_OR_CONTINUE(MADB_Wchar2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case CHAR_BINARY_TYPES:
    RETURN_ERROR_OR_CONTINUE(MADB_Char2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_NUMERIC:
    RETURN_ERROR_OR_CONTINUE(MADB_Numeric2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_TIMESTAMP:
  case SQL_TYPE_TIMESTAMP:
    RETURN_ERROR_OR_CONTINUE(MADB_Timestamp2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_TIME:
  case SQL_C_TYPE_TIME:
    RETURN_ERROR_OR_CONTINUE(MADB_Time2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
    RETURN_ERROR_OR_CONTINUE(MADB_IntervalHtoMS2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_DATE:
  case SQL_TYPE_DATE:
    RETURN_ERROR_OR_CONTINUE(MADB_Date2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  default:
    /* memset(MaBind, 0, sizeof(MYSQL_BIND));
    MaBind->buffer_length= 0; */
    MaBind->buffer_type=   0;
    MaBind->is_unsigned=   0;

    *LengthPtr= (unsigned long)Length;
    MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType,
      &MaBind->is_unsigned, &MaBind->buffer_length);

    if (!CRec->OctetLength)
    {
      CRec->OctetLength= MaBind->buffer_length;
    }
    *Buffer= DataPtr;
  }           /* End of switch (CRec->ConsiseType) */

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_C2SQL */
/* Main entrance function for C type to SQL type conversion*/
SQLRETURN MADB_C2SQL(MADB_Stmt* Stmt, MADB_DescRecord *CRec, MADB_DescRecord *SqlRec, SQLULEN ParamSetIdx, MYSQL_BIND *MaBind)
{
  SQLLEN *IndicatorPtr= NULL;
  SQLLEN *OctetLengthPtr= NULL;
  void   *DataPtr= NULL;
  SQLLEN  Length= 0;

  IndicatorPtr=   (SQLLEN *)GetBindOffset(Stmt->Apd, CRec, CRec->IndicatorPtr, ParamSetIdx, sizeof(SQLLEN));
  OctetLengthPtr= (SQLLEN *)GetBindOffset(Stmt->Apd, CRec, CRec->OctetLengthPtr, ParamSetIdx, sizeof(SQLLEN));

  if (PARAM_IS_DAE(OctetLengthPtr))
  {
    if (!DAE_DONE(Stmt))
    {
      return SQL_NEED_DATA;
    }
    else
    {
      MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType, &MaBind->is_unsigned, &MaBind->buffer_length);
      /* I guess we can leave w/out this. Keeping it so far for safety */
      MaBind->long_data_used= '\1';
      return SQL_SUCCESS;
    }
  }    /* -- End of DAE parameter processing -- */

  if (IndicatorPtr && MADB_ProcessIndicator(Stmt, *IndicatorPtr, CRec->DefaultValue, MaBind))
  {
    return SQL_SUCCESS;
  }

  /* -- Special cases are done, i.e. not a DAE etc, general case -- */
 
  DataPtr= GetBindOffset(Stmt->Apd, CRec, CRec->DataPtr, ParamSetIdx, CRec->OctetLength);

  /* If indicator wasn't NULL_DATA, but data pointer is still NULL, we convert NULL value */
  if (!DataPtr)
  {
    return MADB_ConvertNullValue(Stmt, MaBind);
  }
  
  Length= MADB_CalculateLength(Stmt, OctetLengthPtr, CRec, DataPtr);

  RETURN_ERROR_OR_CONTINUE(MADB_ConvertC2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, NULL, NULL));

  return SQL_SUCCESS;
}
/* }}} */

#define CALC_ALL_FLDS_RC(_agg_rc, _field_rc) if (_field_rc != SQL_SUCCESS && _agg_rc != SQL_ERROR) _agg_rc= _field_rc

/* {{{ MADB_CspsConvertSql2C
 Converts a single value from the result set into the desired type and stores it into the bound data buffer.
 This the client-side prepared statement function only, as the "value" is represented differently for CSPS and SSPS. */
SQLRETURN MADB_CspsConvertSql2C(MADB_Stmt *Stmt, MYSQL_FIELD *field, MYSQL_BIND *bind, char* val, unsigned long fieldLen)
{
    char *value = val;
    my_bool alloced = FALSE;
    if (field->type == MYSQL_TYPE_BIT)
    {
        // BIT comes in the big-endian format, so we need to convert it to the little-endian.
        value = MADB_CALLOC(8);
        alloced = TRUE;

        unsigned long curPos = fieldLen >= 8 ? fieldLen - 8 : 0;
        unsigned int revPos = 0;
        while (curPos < fieldLen)
        {
            value[revPos] = *(val + fieldLen - curPos - 1);
            curPos++;
            revPos++;
        }
    }

    int rc = SQL_SUCCESS;
    switch(bind->buffer_type)
    {
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        {
            int ret;
            // We determine the data representation based on the field type. We could ideally do that by
            // leveraging the field->charsetnr, but for integer types the charsetnr is reported as binary,
            // whereas the data is stored as a string.
            // So the only source of truth we have is the field type reported by the engine.
            switch(field->type)
            {
                case MYSQL_TYPE_BIT:
                case MYSQL_TYPE_TINY_BLOB:
                case MYSQL_TYPE_BLOB:
                case MYSQL_TYPE_MEDIUM_BLOB:
                case MYSQL_TYPE_LONG_BLOB:
                    // Binary representation.
                    ret = MADB_ConvertBinaryToInteger(bind, value, fieldLen);
                    break;
                default:
                    // String representation.
                    ret = MADB_ConvertCharToInteger(bind, value, fieldLen);
            }
            if (!SQL_SUCCEEDED(ret))
            {
                CALC_ALL_FLDS_RC(rc, ret);
            }
            break;
        }
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        {
            // Should we return an error here for an invalid argument?
            char *stopscan;
            SQLDOUBLE val = strtod(value, &stopscan);
            if (bind->buffer_type == MYSQL_TYPE_FLOAT)
            {
                *(float *) bind->buffer = val;
                bind->buffer_length = sizeof(float);
            } else
            {
                *(SQLDOUBLE *) bind->buffer = val;
                bind->buffer_length = sizeof(double);
            }
            break;
        }
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIMESTAMP:
        {
            BOOL isTime;
            int ret = MADB_Str2Ts(value, fieldLen, (MYSQL_TIME *) bind->buffer, FALSE, &Stmt->Error, &isTime);
            if (!SQL_SUCCEEDED(ret))
            {
                CALC_ALL_FLDS_RC(rc, ret);
            }
            break;
        }
        default:
        {
            // A lot of types will be processed here:
            // case MYSQL_TYPE_STRING:
            // case MYSQL_TYPE_VAR_STRING:
            // case MYSQL_TYPE_VARCHAR:
            // case MYSQL_TYPE_TINY_BLOB:
            // case MYSQL_TYPE_BLOB:
            // case MYSQL_TYPE_MEDIUM_BLOB:
            // case MYSQL_TYPE_LONG_BLOB:
            // case MYSQL_TYPE_DECIMAL:
            // case MYSQL_TYPE_NEWDECIMAL:
            // case MYSQL_TYPE_JSON:
            // case MYSQL_TYPE_SET:
            // case MYSQL_TYPE_ENUM:
            // Offset is handled here to allow buffered fetches when the application calls SQLGetData.
            unsigned long offset = bind->offset;
            if (fieldLen >= offset)
            {
                const char* start = value + offset;
                const char* end = value + fieldLen;
                unsigned long copyLen = end - start;
                if (start < end)
                {
                    if (copyLen > bind->buffer_length)
                    {
                        copyLen = bind->buffer_length;
                        rc = MYSQL_DATA_TRUNCATED;
                        *bind->error = 1;
                    }
                    memcpy(bind->buffer, start, copyLen);
                }
                if (copyLen < bind->buffer_length)
                {
                    ((char *) bind->buffer)[copyLen] = '\0';
                }
            }
            *bind->length = fieldLen;
            break;
        }
    }

    if (alloced)
    {
        MADB_FREE(value);
    }

    return rc;
}
/* }}} */

#undef CALC_ALL_FLDS_RC

/* {{{ MADB_ConvertIntegerToChar */
/* Converts Src into Dest based on the integer SourceType. */
SQLLEN MADB_ConvertIntegerToChar(MADB_Stmt *Stmt, int SourceType, void* Src, char* Dest)
{
    SQLBIGINT num;
    my_bool isUnsigned = FALSE;
    switch (SourceType)
    {
        case SQL_C_UBIGINT:
            num = *(SQLUBIGINT*)Src;
            isUnsigned = TRUE;
            break;
        case SQL_C_ULONG:
            num = *(SQLUINTEGER*)Src;
            isUnsigned = TRUE;
            break;
        case SQL_C_USHORT:
            num = *(SQLUSMALLINT*)Src;
            isUnsigned = TRUE;
            break;
        case SQL_C_UTINYINT:
            num = *(SQLCHAR*)Src;
            isUnsigned = TRUE;
            break;
        case SQL_BIGINT:
        case SQL_C_SBIGINT:
            num = *(SQLBIGINT*)Src;
            break;
        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            num = *(SQLSMALLINT*)Src;
            break;
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            num = *(SQLCHAR*)Src;
            break;
        default:
            num = *(SQLINTEGER*)Src;
            break;
    }

    char* formatting = isUnsigned ? "%llu" : "%lld";

#ifdef _WIN32
    formatting = isUnsigned ? "%I64u" : "%I64d";
#endif

    sprintf(Dest, formatting, num);
    return strlen(Dest);
}
/* }}} */

#define NEEDS_DATE_FIELDS(Type) Type == SQL_DATE || Type == SQL_TYPE_DATE || Type == SQL_TIMESTAMP || Type == SQL_TYPE_TIMESTAMP
#define NEEDS_TIME_FIELDS(Type) Type == SQL_TIME || Type == SQL_TYPE_TIME || Type == SQL_TIMESTAMP || Type == SQL_TYPE_TIMESTAMP

/* {{{ */
/* Converts Src into Dest based on the Datetime SourceType. */
SQLRETURN MADB_ConvertDatetimeToChar(MADB_Stmt *Stmt, int SourceType, int SqlType, void* Src, char* Dest)
{
    int ret = SQL_SUCCESS;
    char *DestIter = Dest;

    // Let's use YYYY-MM-DD HH:MM:SS.MS format. We don't need to prepend zeros for single-digit values, because the
    // engine correctly handles such input (e.g. 2020-1-1).
    // I'm not sure if it's a right place to do any sanity checks, maybe we should just pass over whatever was
    // set by the client.
    switch (SourceType)
    {
        case SQL_C_TIME:
        case SQL_C_TYPE_TIME:
        {
            SQL_TIME_STRUCT *ts = (SQL_TIME_STRUCT *) Src;
            if (!VALID_TIME(ts))
            {
                return MADB_SetError(&Stmt->Error, MADB_ERR_22007, "Invalid time", 0);
            }
            DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->hour, DestIter);
            *DestIter++ = ':';
            DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->minute, DestIter);
            *DestIter++ = ':';
            DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->second, DestIter);
            *DestIter = '\0';
            break;
        }
        case SQL_C_DATE:
        case SQL_C_TYPE_DATE:
        {
            SQL_DATE_STRUCT *ds = (SQL_DATE_STRUCT*) Src;
            DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_SHORT, &ds->year, DestIter);
            *DestIter++ = '-';
            DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ds->month, DestIter);
            *DestIter++ = '-';
            DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ds->day, DestIter);
            *DestIter = '\0';
            break;
        }
        case SQL_C_TIMESTAMP:
        case SQL_C_TYPE_TIMESTAMP:
        {
            SQL_TIMESTAMP_STRUCT *ts = (SQL_TIMESTAMP_STRUCT *) Src;
            // Before we convert let's check for invalid cases:
            // If we convert into DATE, SQL_TIMESTAMP_STRUCT cannot contain any TIME-related fields or fraction.
            // If we convert into TIME, SQL_TIMESTAMP_STRUCT must contain a valid time and have no fraction.
            RETURN_ERROR_OR_CONTINUE(MADB_TsConversionIsPossible(ts, SqlType, &Stmt->Error, MADB_ERR_22007, 0));

            // For DATE and TIMESTAMP we create the YYYY-MM-DD part.
            if (NEEDS_DATE_FIELDS(SqlType))
            {
                DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_SHORT, &ts->year, DestIter);
                *DestIter++ = '-';
                DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->month, DestIter);
                *DestIter++ = '-';
                DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->day, DestIter);
                if (SqlType == SQL_TIMESTAMP || SqlType == SQL_TYPE_TIMESTAMP)
                {
                    *DestIter++ = ' ';
                }
            }

            // For TIME and TIMESTAMP we create the HH:MM:SS part.
            if (NEEDS_TIME_FIELDS(SqlType))
            {
                DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->hour, DestIter);
                *DestIter++ = ':';
                DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->minute, DestIter);
                *DestIter++ = ':';
                DestIter += MADB_ConvertIntegerToChar(Stmt, SQL_C_USHORT, &ts->second, DestIter);
            }

            // For TIMESTAMP we also add a fraction.
            if (SqlType == SQL_TIMESTAMP || SqlType == SQL_TYPE_TIMESTAMP)
            {
                if (ts->fraction / 1000)
                {
                    SQLUINTEGER fraction = ts->fraction / 1000;
                    *DestIter++ = '.';

                    // frac array cannot have more than 6 characters that represent fraction, but allocating big enough,
                    // just in case.
                    SQLLEN fracLen, fillZeros = 0;
                    char frac[10];
                    memset(frac, 0, sizeof(frac));
                    fracLen = MADB_ConvertIntegerToChar(Stmt, SQL_C_ULONG, &fraction, frac);
                    while (fracLen + fillZeros++ < 6)
                    {
                        *DestIter++ = '0';
                    }
                    strcpy(DestIter, frac);
                    DestIter += fracLen;
                }
            }
            *DestIter = '\0';
            break;
        }
        default:
            // Invalid parameter value.
            ret = MADB_SetError(&Stmt->Error, MADB_ERR_22023, "Invalid date/time parameter type", 0);
    }

    return ret;
}
/* }}} */

#undef NEEDS_DATE_FIELDS
#undef NEEDS_TIME_FIELDS

#ifndef _WIN32
#define _strtoi64 strtoll
#endif

#define TINYINT_MIN (int)(-128)
#define TINYINT_MAX 127
#define UTINYINT_MAX 255
#define SMALLINT_MIN (int)(-32768)
#define SMALLINT_MAX 32767
#define USMALLINT_MAX 65535
#define INTEGER_MIN (int)(-2147483648)
#define INTEGER_MAX 2147483647
#define UINTEGER_MAX 4294967295


/* {{{ MADB_ConvertCharToInteger
 Converts SQL char into the relevant C integer type and stores the result in the MYSQL_BIND. */
SQLRETURN MADB_ConvertCharToInteger(MYSQL_BIND* Dest, char* Src, unsigned int fieldLen)
{
    SQLRETURN rc = SQL_SUCCESS;
    char *badPtr;
    SQLBIGINT val = Dest->is_unsigned ? strtoull(Src, &badPtr, 10) : _strtoi64(Src, &badPtr, 10);
    if (badPtr && badPtr - Src < fieldLen)
    {
        rc = MYSQL_DATA_TRUNCATED;
        *Dest->error = 1;
    }

    switch (Dest->buffer_type)
    {
        case MYSQL_TYPE_TINY:
            if (Dest->is_unsigned)
            {
                if (val < 0 || val > UTINYINT_MAX)
                {
                    rc = MYSQL_DATA_TRUNCATED;
                    *Dest->error = 1;
                }
                *(unsigned char*)Dest->buffer = val;
            } else
            {
                if (val < TINYINT_MIN || val > TINYINT_MAX)
                {
                    rc = MYSQL_DATA_TRUNCATED;
                    *Dest->error = 1;
                }
                *(char*)Dest->buffer = val;
            }
            Dest->buffer_length = sizeof(SQLCHAR);
            break;
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_YEAR:
            if (Dest->is_unsigned)
            {
                if (val < 0 || val > USMALLINT_MAX)
                {
                    rc = MYSQL_DATA_TRUNCATED;
                    *Dest->error = 1;
                }
                *(SQLUSMALLINT *)Dest->buffer = val;
            } else
            {
                if (val < SMALLINT_MIN || val > SMALLINT_MAX)
                {
                    rc = MYSQL_DATA_TRUNCATED;
                    *Dest->error = 1;
                }
                *(SQLSMALLINT *)Dest->buffer = val;
            }
            Dest->buffer_length = sizeof(SQLSMALLINT);
            break;
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
            if (Dest->is_unsigned)
            {
                if (val < 0 || val > UINTEGER_MAX)
                {
                    rc = MYSQL_DATA_TRUNCATED;
                    *Dest->error = 1;
                }
                *(SQLUINTEGER *)Dest->buffer = val;
            } else
            {
                if (val < INTEGER_MIN || val > INTEGER_MAX)
                {
                    rc = MYSQL_DATA_TRUNCATED;
                    *Dest->error = 1;
                }
                *(SQLINTEGER *)Dest->buffer = val;
            }
            Dest->buffer_length = sizeof(SQLINTEGER);
            break;
        case MYSQL_TYPE_LONGLONG:
            if (Dest->is_unsigned)
            {
                *(SQLUBIGINT *)Dest->buffer = val;
            } else
            {
                *(SQLBIGINT *)Dest->buffer = val;
            }
            Dest->buffer_length = sizeof(SQLBIGINT);
            break;
        default:
        	break;
    }

    return rc;
}
/* }}} */

/* {{{ MADB_ConvertBinaryToInteger
 This simply copies the binary data into the integer variable and handles truncation. */
SQLRETURN MADB_ConvertBinaryToInteger(MYSQL_BIND* Dest, char* Src, unsigned int fieldLen)
{
    int rc = SQL_SUCCESS;
    unsigned long offset = Dest->offset;
    if (fieldLen >= offset)
    {
        const char* start = Src + offset;
        const char* end = Src + fieldLen;
        unsigned long copyLen = end - start;
        if (start < end)
        {
            if (copyLen > Dest->buffer_length)
            {
                copyLen = Dest->buffer_length;
                rc = MYSQL_DATA_TRUNCATED;
                *Dest->error = 1;
            }
            memcpy(Dest->buffer, start, copyLen);
        }
    }
    *Dest->length = fieldLen;

    return rc;
}
/* }}} */

#undef TINYINT_MIN
#undef TINYINT_MAX
#undef UTINYINT_MAX
#undef SMALLINT_MIN
#undef SMALLINT_MAX
#undef USMALLINT_MAX
#undef INTEGER_MIN
#undef INTEGER_MAX
#undef UINTEGER_MAX
