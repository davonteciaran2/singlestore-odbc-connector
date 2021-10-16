/************************************************************************************
   Copyright (C) 2013, 2016 MariaDB Corporation AB

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
#ifndef _ma_info_h_
#define _ma_info_h_

typedef struct
{
  char *TypeName;
  SQLSMALLINT DataType;
  SQLINTEGER ColumnSize;
  /* size in bytes for binary/string data; otherwise
     the number of characters in the character representation of data */
  char *LiteralPrefix;
  char *LiteralSuffix;
  char *CreateParams;
  SQLSMALLINT Nullable;
  SQLSMALLINT CaseSensitive;
  SQLSMALLINT Searchable;
  SQLSMALLINT Unsigned;
  SQLSMALLINT FixedPrecScale;
  SQLSMALLINT AutoUniqueValue;
  char *LocalTypeName;
  SQLSMALLINT MinimumScale;
  SQLSMALLINT MaximumScale;
  SQLSMALLINT SqlDateTimeSub;
  SQLSMALLINT IntervalPrecision;
  SQLINTEGER NumPrecRadix;
  SQLSMALLINT SqlDataType;
} MADB_TypeInfo;

static const MADB_TypeInfo TypeInfoV3[]=
{
  {"json"              ,SQL_WLONGVARCHAR  ,16777216  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"json"              ,0   ,0  ,0                 ,0,0 ,SQL_WLONGVARCHAR },
  {"long varchar"      ,SQL_WLONGVARCHAR  ,16777215  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"long varchar"      ,0   ,0  ,0                 ,0,0 ,SQL_WLONGVARCHAR },
  {"longtext"          ,SQL_WLONGVARCHAR  ,2147483647,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"longtext"          ,0   ,0  ,0                 ,0,0 ,SQL_WLONGVARCHAR },
  {"mediumtext"        ,SQL_WLONGVARCHAR  ,16777215  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"mediumtext"        ,0   ,0  ,0                 ,0,0 ,SQL_WLONGVARCHAR },
  {"text"              ,SQL_WLONGVARCHAR  ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"text"              ,0   ,0  ,0                 ,0,0 ,SQL_WLONGVARCHAR },
  {"tinytext"          ,SQL_WLONGVARCHAR  ,255       ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"tinytext"          ,0   ,0  ,0                 ,0,0 ,SQL_WLONGVARCHAR },
  {"enum"              ,SQL_WVARCHAR      ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"enum"              ,0   ,0  ,0                 ,0,0 ,SQL_WVARCHAR     },
  {"set"               ,SQL_WVARCHAR      ,64        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"set"               ,0   ,0  ,0                 ,0,0 ,SQL_WVARCHAR     },
  {"varchar"           ,SQL_WVARCHAR      ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"varchar"           ,0   ,0  ,0                 ,0,0 ,SQL_WVARCHAR     },
  {"char"              ,SQL_WCHAR         ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"char"              ,0   ,0  ,0                 ,0,0 ,SQL_WCHAR        },
  {"bool"              ,SQL_TINYINT       ,1         ,"" ,"" ,"NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"bool"              ,0   ,0  ,0                 ,0,10,SQL_TINYINT      },
  {"boolean"           ,SQL_TINYINT       ,1         ,"" ,"" ,"NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"boolean"           ,0   ,0  ,0                 ,0,10,SQL_TINYINT      },
  {"tinyint"           ,SQL_TINYINT       ,3         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"tinyint"           ,0   ,0  ,0                 ,0,10,SQL_TINYINT      },
  {"tinyint unsigned"  ,SQL_TINYINT       ,3         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"tinyint unsigned"  ,0   ,0  ,0                 ,0,10,SQL_TINYINT      },
  {"bigint"            ,SQL_BIGINT        ,19        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"bigint"            ,0   ,0  ,0                 ,0,10,SQL_BIGINT       },
  {"bigint unsigned"   ,SQL_BIGINT        ,20        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"bigint unsigned"   ,0   ,0  ,0                 ,0,10,SQL_BIGINT       },
  {"blob"              ,SQL_LONGVARBINARY ,65535     ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"blob"              ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARBINARY},
  {"long varbinary"    ,SQL_LONGVARBINARY ,16777215  ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"long varbinary"    ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARBINARY},
  {"longblob"          ,SQL_LONGVARBINARY ,2147483647,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"longblob"          ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARBINARY},
  {"mediumblob"        ,SQL_LONGVARBINARY ,16777215  ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"mediumblob"        ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARBINARY},
  {"tinyblob"          ,SQL_LONGVARBINARY ,255       ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"tinyblob"          ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARBINARY},
  {"geography"         ,SQL_VARBINARY     ,255       ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"geography"         ,0   ,0  ,0                 ,0,0 ,SQL_VARBINARY    },
  {"geographypoint"    ,SQL_VARBINARY     ,20        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"geographypoint"    ,0   ,0  ,0                 ,0,0 ,SQL_VARBINARY    },
  {"varbinary"         ,SQL_VARBINARY     ,255       ,"'","'","'length'"         ,1,1,SQL_SEARCHABLE,0,0,0,"varbinary"         ,0   ,0  ,0                 ,0,0 ,SQL_VARBINARY    },
  {"binary"            ,SQL_BINARY        ,255       ,"'","'","'length'"         ,1,1,SQL_SEARCHABLE,0,0,0,"binary"            ,0   ,0  ,0                 ,0,0 ,SQL_BINARY       },
  {"bit"               ,SQL_BINARY        ,8         ,"" ,"" ,"NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"bit"               ,0   ,0  ,0                 ,0,0 ,SQL_BINARY       },
  {"json"              ,SQL_LONGVARCHAR   ,16777216  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"json"              ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARCHAR  },
  {"longtext"          ,SQL_LONGVARCHAR   ,2147483647,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"longtext"          ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARCHAR  },
  {"mediumtext"        ,SQL_LONGVARCHAR   ,16777215  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"mediumtext"        ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARCHAR  },
  {"text"              ,SQL_LONGVARCHAR   ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"text"              ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARCHAR  },
  {"tinytext"          ,SQL_LONGVARCHAR   ,255       ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"tinytext"          ,0   ,0  ,0                 ,0,0 ,SQL_LONGVARCHAR  },
  {"char"              ,SQL_CHAR          ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"char"              ,0   ,0  ,0                 ,0,0 ,SQL_CHAR         },
  {"numeric"           ,SQL_NUMERIC       ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"numeric"           ,-308,308,0                 ,0,10,SQL_NUMERIC      },
  {"dec"               ,SQL_DECIMAL       ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"dec"               ,-308,308,0                 ,0,10,SQL_DECIMAL      },
  {"decimal"           ,SQL_DECIMAL       ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"decimal"           ,-308,308,0                 ,0,10,SQL_DECIMAL      },
  {"fixed"             ,SQL_DECIMAL       ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"fixed"             ,-308,308,0                 ,0,10,SQL_DECIMAL      },
  {"int"               ,SQL_INTEGER       ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"int"               ,0   ,0  ,0                 ,0,10,SQL_INTEGER      },
  {"int unsigned"      ,SQL_INTEGER       ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"int unsigned"      ,0   ,0  ,0                 ,0,10,SQL_INTEGER      },
  {"integer"           ,SQL_INTEGER       ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"integer"           ,0   ,0  ,0                 ,0,10,SQL_INTEGER      },
  {"integer unsigned"  ,SQL_INTEGER       ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"integer unsigned"  ,0   ,0  ,0                 ,0,10,SQL_INTEGER      },
  {"mediumint"         ,SQL_INTEGER       ,7         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"mediumint"         ,0   ,0  ,0                 ,0,10,SQL_INTEGER      },
  {"mediumint unsigned",SQL_INTEGER       ,8         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"mediumint unsigned",0   ,0  ,0                 ,0,10,SQL_INTEGER      },
  {"smallint"          ,SQL_SMALLINT      ,5         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"smallint"          ,0   ,0  ,0                 ,0,10,SQL_SMALLINT     },
  {"smallint unsigned" ,SQL_SMALLINT      ,5         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"smallint unsigned" ,0   ,0  ,0                 ,0,10,SQL_SMALLINT     },
  {"year"              ,SQL_SMALLINT      ,4         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"year"              ,0   ,0  ,0                 ,0,10,SQL_SMALLINT     },
  {"float"             ,SQL_REAL          ,7        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"float"             ,-38 ,38 ,0                 ,0,10,SQL_REAL        },
  {"double"            ,SQL_DOUBLE        ,15        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"double"            ,-308,308,0                 ,0,10,SQL_DOUBLE       },
  {"double precision"  ,SQL_DOUBLE        ,15        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"double precision"  ,-308,308,0                 ,0,10,SQL_DOUBLE       },
  {"real"              ,SQL_DOUBLE        ,15        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"real"              ,-308,308,0                 ,0,10,SQL_DOUBLE       },
  {"enum"              ,SQL_VARCHAR       ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"enum"              ,0   ,0  ,0                 ,0,0 ,SQL_VARCHAR      },
  {"set"               ,SQL_VARCHAR       ,64        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"set"               ,0   ,0  ,0                 ,0,0 ,SQL_VARCHAR      },
  {"varchar"           ,SQL_VARCHAR       ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"varchar"           ,0   ,0  ,0                 ,0,0 ,SQL_VARCHAR      },
  {"date"              ,SQL_TYPE_DATE     ,10        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"date"              ,0   ,0  ,SQL_CODE_DATE     ,0,0 ,SQL_DATETIME     },
  {"time"              ,SQL_TYPE_TIME     ,8         ,"'","'","'scale'"          ,1,0,SQL_SEARCHABLE,0,0,0,"time"              ,0   ,0  ,SQL_CODE_TIME     ,0,0 ,SQL_DATETIME     },
  {"datetime"          ,SQL_TYPE_TIMESTAMP,26        ,"'","'","'scale'"          ,1,0,SQL_SEARCHABLE,0,0,0,"datetime"          ,0   ,0  ,SQL_CODE_TIMESTAMP,0,0 ,SQL_DATETIME     },
  {"timestamp"         ,SQL_TYPE_TIMESTAMP,26        ,"'","'","'scale'"          ,1,0,SQL_SEARCHABLE,0,0,0,"timestamp"         ,0   ,0  ,SQL_CODE_TIMESTAMP,0,0 ,SQL_DATETIME     },
};

static const MADB_TypeInfo TypeInfoV2[]=
{
  {"json"              ,SQL_WLONGVARCHAR ,16777216  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"json"              ,0   ,0  ,0,0,0 ,SQL_WLONGVARCHAR },
  {"long varchar"      ,SQL_WLONGVARCHAR ,16777215  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"long varchar"      ,0   ,0  ,0,0,0 ,SQL_WLONGVARCHAR },
  {"longtext"          ,SQL_WLONGVARCHAR ,2147483647,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"longtext"          ,0   ,0  ,0,0,0 ,SQL_WLONGVARCHAR },
  {"mediumtext"        ,SQL_WLONGVARCHAR ,16777215  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"mediumtext"        ,0   ,0  ,0,0,0 ,SQL_WLONGVARCHAR },
  {"text"              ,SQL_WLONGVARCHAR ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"text"              ,0   ,0  ,0,0,0 ,SQL_WLONGVARCHAR },
  {"tinytext"          ,SQL_WLONGVARCHAR ,255       ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"tinytext"          ,0   ,0  ,0,0,0 ,SQL_WLONGVARCHAR },
  {"enum"              ,SQL_WVARCHAR     ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"enum"              ,0   ,0  ,0,0,0 ,SQL_WVARCHAR     },
  {"set"               ,SQL_WVARCHAR     ,64        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"set"               ,0   ,0  ,0,0,0 ,SQL_WVARCHAR     },
  {"varchar"           ,SQL_WVARCHAR     ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"varchar"           ,0   ,0  ,0,0,0 ,SQL_WVARCHAR     },
  {"char"              ,SQL_WCHAR        ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"char"              ,0   ,0  ,0,0,0 ,SQL_WCHAR        },
  {"bool"              ,SQL_TINYINT      ,1         ,"" ,"" ,"NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"bool"              ,0   ,0  ,0,0,10,SQL_TINYINT      },
  {"boolean"           ,SQL_TINYINT      ,1         ,"" ,"" ,"NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"boolean"           ,0   ,0  ,0,0,10,SQL_TINYINT      },
  {"tinyint"           ,SQL_TINYINT      ,3         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"tinyint"           ,0   ,0  ,0,0,10,SQL_TINYINT      },
  {"tinyint unsigned"  ,SQL_TINYINT      ,3         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"tinyint unsigned"  ,0   ,0  ,0,0,10,SQL_TINYINT      },
  {"bigint"            ,SQL_BIGINT       ,19        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"bigint"            ,0   ,0  ,0,0,10,SQL_BIGINT       },
  {"bigint unsigned"   ,SQL_BIGINT       ,20        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"bigint unsigned"   ,0   ,0  ,0,0,10,SQL_BIGINT       },
  {"blob"              ,SQL_LONGVARBINARY,65535     ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"blob"              ,0   ,0  ,0,0,0 ,SQL_LONGVARBINARY},
  {"long varbinary"    ,SQL_LONGVARBINARY,16777215  ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"long varbinary"    ,0   ,0  ,0,0,0 ,SQL_LONGVARBINARY},
  {"longblob"          ,SQL_LONGVARBINARY,2147483647,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"longblob"          ,0   ,0  ,0,0,0 ,SQL_LONGVARBINARY},
  {"mediumblob"        ,SQL_LONGVARBINARY,16777215  ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"mediumblob"        ,0   ,0  ,0,0,0 ,SQL_LONGVARBINARY},
  {"tinyblob"          ,SQL_LONGVARBINARY,255       ,"'","'","NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"tinyblob"          ,0   ,0  ,0,0,0 ,SQL_LONGVARBINARY},
  {"geography"         ,SQL_VARBINARY    ,255       ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"geography"         ,0   ,0  ,0,0,0 ,SQL_VARBINARY    },
  {"geographypoint"    ,SQL_VARBINARY    ,20        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"geographypoint"    ,0   ,0  ,0,0,0 ,SQL_VARBINARY    },
  {"varbinary"         ,SQL_VARBINARY    ,255       ,"'","'","'length'"         ,1,1,SQL_SEARCHABLE,0,0,0,"varbinary"         ,0   ,0  ,0,0,0 ,SQL_VARBINARY    },
  {"binary"            ,SQL_BINARY       ,255       ,"'","'","'length'"         ,1,1,SQL_SEARCHABLE,0,0,0,"binary"            ,0   ,0  ,0,0,0 ,SQL_BINARY       },
  {"bit"               ,SQL_BINARY       ,8         ,"" ,"" ,"NULL"             ,1,1,SQL_SEARCHABLE,0,0,0,"bit"               ,0   ,0  ,0,0,0 ,SQL_BINARY       },
  {"json"              ,SQL_LONGVARCHAR  ,16777216  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"json"              ,0   ,0  ,0,0,0 ,SQL_LONGVARCHAR  },
  {"longtext"          ,SQL_LONGVARCHAR  ,2147483647,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"longtext"          ,0   ,0  ,0,0,0 ,SQL_LONGVARCHAR  },
  {"mediumtext"        ,SQL_LONGVARCHAR  ,16777215  ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"mediumtext"        ,0   ,0  ,0,0,0 ,SQL_LONGVARCHAR  },
  {"text"              ,SQL_LONGVARCHAR  ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"text"              ,0   ,0  ,0,0,0 ,SQL_LONGVARCHAR  },
  {"tinytext"          ,SQL_LONGVARCHAR  ,255       ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"tinytext"          ,0   ,0  ,0,0,0 ,SQL_LONGVARCHAR  },
  {"char"              ,SQL_CHAR         ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"char"              ,0   ,0  ,0,0,0 ,SQL_CHAR         },
  {"numeric"           ,SQL_NUMERIC      ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"numeric"           ,-308,308,0,0,10,SQL_NUMERIC      },
  {"dec"               ,SQL_DECIMAL      ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"dec"               ,-308,308,0,0,10,SQL_DECIMAL      },
  {"decimal"           ,SQL_DECIMAL      ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"decimal"           ,-308,308,0,0,10,SQL_DECIMAL      },
  {"fixed"             ,SQL_DECIMAL      ,65        ,"" ,"" ,"'precision,scale'",1,0,SQL_SEARCHABLE,0,0,1,"fixed"             ,-308,308,0,0,10,SQL_DECIMAL      },
  {"int"               ,SQL_INTEGER      ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"int"               ,0   ,0  ,0,0,10,SQL_INTEGER      },
  {"int unsigned"      ,SQL_INTEGER      ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"int unsigned"      ,0   ,0  ,0,0,10,SQL_INTEGER      },
  {"integer"           ,SQL_INTEGER      ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"integer"           ,0   ,0  ,0,0,10,SQL_INTEGER      },
  {"integer unsigned"  ,SQL_INTEGER      ,10        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"integer unsigned"  ,0   ,0  ,0,0,10,SQL_INTEGER      },
  {"mediumint"         ,SQL_INTEGER      ,7         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"mediumint"         ,0   ,0  ,0,0,10,SQL_INTEGER      },
  {"mediumint unsigned",SQL_INTEGER      ,8         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"mediumint unsigned",0   ,0  ,0,0,10,SQL_INTEGER      },
  {"smallint"          ,SQL_SMALLINT     ,5         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"smallint"          ,0   ,0  ,0,0,10,SQL_SMALLINT     },
  {"smallint unsigned" ,SQL_SMALLINT     ,5         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,1,0,1,"smallint unsigned" ,0   ,0  ,0,0,10,SQL_SMALLINT     },
  {"year"              ,SQL_SMALLINT     ,4         ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"year"              ,0   ,0  ,0,0,10,SQL_SMALLINT     },
  {"float"             ,SQL_REAL         ,7        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"float"             ,-38 ,38 ,0,0,10,SQL_REAL         },
  {"double"            ,SQL_DOUBLE       ,15        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"double"            ,-308,308,0,0,10,SQL_DOUBLE       },
  {"double precision"  ,SQL_DOUBLE       ,15        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"double precision"  ,-308,308,0,0,10,SQL_DOUBLE       },
  {"real"              ,SQL_DOUBLE       ,15        ,"" ,"" ,"NULL"             ,1,0,SQL_SEARCHABLE,0,0,1,"real"              ,-308,308,0,0,10,SQL_DOUBLE       },
  {"date"              ,SQL_DATE         ,10        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"date"              ,0   ,0  ,0,0,0 ,SQL_DATETIME     },
  {"time"              ,SQL_TIME         ,18        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"time"              ,0   ,0  ,0,0,0 ,SQL_DATETIME     },
  {"datetime"          ,SQL_TIMESTAMP    ,26        ,"'","'","'scale'"          ,1,0,SQL_SEARCHABLE,0,0,0,"datetime"          ,0   ,0  ,0,0,0 ,SQL_DATETIME     },
  {"timestamp"         ,SQL_TIMESTAMP    ,26        ,"'","'","'scale'"          ,1,0,SQL_SEARCHABLE,0,0,0,"timestamp"         ,0   ,0  ,0,0,0 ,SQL_DATETIME     },
  {"enum"              ,SQL_VARCHAR      ,65535     ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"enum"              ,0   ,0  ,0,0,0 ,SQL_VARCHAR      },
  {"set"               ,SQL_VARCHAR      ,64        ,"'","'","NULL"             ,1,0,SQL_SEARCHABLE,0,0,0,"set"               ,0   ,0  ,0,0,0 ,SQL_VARCHAR      },
  {"varchar"           ,SQL_VARCHAR      ,255       ,"'","'","'length'"         ,1,0,SQL_SEARCHABLE,0,0,0,"varchar"           ,0   ,0  ,0,0,0 ,SQL_VARCHAR      },
};

const MADB_TypeInfo* GetTypeInfo(SQLSMALLINT SqlType, MYSQL_FIELD *Field);
SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle,
                           SQLSMALLINT DataType);

#endif /* _ma_info_h_ */
