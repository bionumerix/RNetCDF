/*=============================================================================*\
 *
 *  Name:       common.c
 *
 *  Version:    2.0-1
 *
 *  Purpose:    Common definitions for RNetCDF functions
 *
 *  Author:     Pavel Michna (rnetcdf-devel@bluewin.ch)
 *              Milton Woods (miltonjwoods@gmail.com)
 *
 *  Copyright:  (C) 2004-2017 Pavel Michna, Milton Woods
 *
 *=============================================================================*
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *=============================================================================*
 *  Implementation and Revisions
 *-----------------------------------------------------------------------------*
 * $Header$
\*=============================================================================*/

#include <string.h>

#include <R.h>
#include <Rinternals.h>

#include <netcdf.h>

#ifdef HAVE_UDUNITS2_UDUNITS_H
# include <udunits2/udunits.h>
#else
# include <udunits.h>
#endif

#include "common.h"

static int R_nc_protect_count = 0;

SEXP
R_nc_protect (SEXP obj)
{
  PROTECT(obj);
  R_nc_protect_count++;
  return obj;
}


void
R_nc_unprotect (void)
{
  if (R_nc_protect_count > 0) {
    UNPROTECT (R_nc_protect_count);
    R_nc_protect_count = 0;
  }
}


void
R_nc_error(const char *msg)
{
  R_nc_unprotect ();
  error (msg);
}


int
R_nc_check(int status)
{
  if (status != NC_NOERR) {
    R_nc_error (nc_strerror (status));
  }
  return status;
}


int
R_nc_strcmp (SEXP var, const char *str)
{
  return (isString(var) &&
          xlength(var) >= 1 &&
          strcmp(CHAR (STRING_ELT (var, 0)), str) == 0);
}


int
R_nc_inherits (SEXP var, const char *class)
{
  SEXP classes;
  size_t ii, cnt;
  classes = getAttrib (var, R_ClassSymbol);
  if (isString (classes)) {
    cnt = xlength (classes);
    for (ii=0; ii<cnt; ii++) {
      if (strcmp(class, CHAR (STRING_ELT (classes, ii))) == 0) {
        return 1;
      }
    }
  }
  return 0;
}


int
R_nc_dim_id (SEXP dim, int ncid, int *dimid, int idx)
{
  if (xlength (dim) <= idx) {
    return NC_EINVAL;
  } else if (isInteger (dim)) {
    *dimid = INTEGER (dim)[idx];
    return NC_NOERR;
  } else if (isReal (dim)) {
    *dimid = REAL (dim)[idx];
    return NC_NOERR;
  } else if (isString (dim)) {
    return nc_inq_dimid (ncid, CHAR (STRING_ELT (dim, idx)), dimid);
  } else {
    return NC_EINVAL;
  }
}


int
R_nc_var_id (SEXP var, int ncid, int *varid)
{
  if (xlength (var) <= 0) {
    return NC_EINVAL;
  } else if (isNumeric (var)) {
    *varid = asInteger (var);
    return NC_NOERR;
  } else if (isString (var)) {
    return nc_inq_varid (ncid, CHAR (STRING_ELT (var, 0)), varid);
  } else {
    return NC_EINVAL;
  }
}


int
R_nc_type_id (SEXP type, int ncid, nc_type *xtype, int idx)
{
  if (length (type) <= idx) {
    return NC_EINVAL;
  } else if (isInteger (type)) {
    *xtype = INTEGER (type)[idx];
    return NC_NOERR;
  } else if (isReal (type)) {
    *xtype = REAL (type)[idx];
    return NC_NOERR;
  } else if (isString (type)) {
    return R_nc_str2type (ncid, CHAR (STRING_ELT (type, idx)), xtype);
  } else {
    return NC_EINVAL;
  }
}


int
R_nc_type2str (int ncid, nc_type xtype, char *typename)
{
  char *str;
  switch (xtype) {
  case NC_BYTE:
    str = "NC_BYTE";
    break;
  case NC_UBYTE:
    str = "NC_UBYTE";
    break;
  case NC_CHAR:
    str = "NC_CHAR";
    break;
  case NC_SHORT:
    str = "NC_SHORT";
    break;
  case NC_USHORT:
    str = "NC_USHORT";
    break;
  case NC_INT:
    str = "NC_INT";
    break;
  case NC_UINT:
    str = "NC_UINT";
    break;
  case NC_INT64:
    str = "NC_INT64";
    break;
  case NC_UINT64:
    str = "NC_UINT64";
    break;
  case NC_FLOAT:
    str = "NC_FLOAT";
    break;
  case NC_DOUBLE:
    str = "NC_DOUBLE";
    break;
  case NC_STRING:
    str = "NC_STRING";
    break;
  default:
    /* Try to get name of a user defined type */
    return nc_inq_user_type (ncid, xtype, typename, NULL, NULL, NULL, NULL);
  }
  /* Copy name to output string buffer */
  strcpy (typename, str);
  return NC_NOERR;
}


int
R_nc_str2type (int ncid, const char *str, nc_type * xtype)
{
  size_t typelen;
  typelen = strlen (str);
  *xtype = NC_NAT;
  if (typelen >= 6) {
    switch (str[3]) {
    case 'B':
      if (strcmp (str, "NC_BYTE") == 0) {
        *xtype = NC_BYTE;
      }
      break;
    case 'C':
      if (strcmp (str, "NC_CHAR") == 0) {
        *xtype = NC_CHAR;
      }
      break;
    case 'D':
      if (strcmp (str, "NC_DOUBLE") == 0) {
        *xtype = NC_DOUBLE;
      }
      break;
    case 'F':
      if (strcmp (str, "NC_FLOAT") == 0) {
        *xtype = NC_FLOAT;
      }
      break;
    case 'I':
      switch (str[6]) {
      case '\0':
        if (strcmp (str, "NC_INT") == 0) {
          *xtype = NC_INT;
        }
        break;
      case '6':
        if (strcmp (str, "NC_INT64") == 0) {
          *xtype = NC_INT64;
        }
        break;
      }
      break;
    case 'L':
      if (strcmp (str, "NC_LONG") == 0) {
        *xtype = NC_LONG;
      }
      break;
    case 'S':
      switch (str[4]) {
      case 'H':
        if (strcmp (str, "NC_SHORT") == 0) {
          *xtype = NC_SHORT;
        }
        break;
      case 'T':
        if (strcmp (str, "NC_STRING") == 0) {
          *xtype = NC_STRING;
        }
        break;
      }
      break;
    case 'U':
      if (typelen >= 7) {
        switch (str[7]) {
        case '\0':
          if (strcmp (str, "NC_UINT") == 0) {
            *xtype = NC_UINT;
          }
          break;
        case '6':
          if (strcmp (str, "NC_UINT64") == 0) {
            *xtype = NC_UINT64;
          }
          break;
        case 'E':
          if (strcmp (str, "NC_UBYTE") == 0) {
            *xtype = NC_UBYTE;
          }
          break;
        case 'R':
          if (strcmp (str, "NC_USHORT") == 0) {
            *xtype = NC_USHORT;
          }
          break;
        }
      }
      break;
    }
  }

  if (*xtype == NC_NAT) {
    /* Try to get id of a user defined type */
    return nc_inq_typeid (ncid, str, xtype);
  } else {
    return NC_NOERR;
  }
}


const char *
R_nc_strarg (SEXP str)
{
  if (xlength (str) > 0 && isString (str)) {
    return CHAR (STRING_ELT (str, 0));
  } else {
    RERROR ("Expected character string as argument");
  }
}


size_t
R_nc_sizearg (SEXP size)
{
  int erange=0;
  size_t result=0;
  if (xlength (size) > 0) {
    if (TYPEOF (size) == INTSXP) {
      int ival;
      ival = INTEGER (size)[0];
      erange = (ival < 0 || ival > SIZE_MAX || ival == NA_INTEGER);
      if (!erange) {
        result = ival;
      }
    } else if (TYPEOF (size) == REALSXP) {
      if (R_nc_inherits (size, "integer64")) {
        long long llval;
        llval = *(long long *) REAL (size);
        /* Allow wrapping of negative to positive values
           by converting from signed to unsigned long long
         */
        if (sizeof (long long) > sizeof (size_t)) {
          erange = (llval < 0 || llval > SIZE_MAX || llval == NA_INTEGER64);
        } else {
          /* Allow wrapping of negative to positive values
             in conversion from signed long long to (unsigned) size_t */
          erange = (llval == NA_INTEGER64);
        }
        if (!erange) {
          result = llval;
        }
      } else {
        double dval;
        dval = REAL (size)[0];
        erange = (dval < 0 || dval > SIZE_MAX || ! R_FINITE (dval));
        result = dval;
      }
    } else {
      R_nc_error ("Size argument has unsupported R type");
    }
  } else {
    R_nc_error ("Size argument must contain at least one numeric value");
  }
  if (erange) {
    R_nc_error ("Size argument is outside valid range");
  }
  return result;
}


int
R_nc_redef (int ncid)
{
  int status;
  status = nc_redef(ncid);
  if (status == NC_EINDEFINE) {
    status = NC_NOERR;
  }
  return status;
}


int
R_nc_enddef (int ncid)
{
  nc_enddef(ncid);
  return NC_NOERR;
}

