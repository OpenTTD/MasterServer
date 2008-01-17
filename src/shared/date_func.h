/* $Id$ */

#ifndef DATE_FUNC_H
#define DATE_FUNC_H

#include "date_type.h"

void ConvertDateToYMD(Date date, YearMonthDay *ymd);
Date ConvertYMDToDate(Year year, Month month, Day day);
void DateToString(Date date, char *dest, size_t length);

#endif /* DATE_FUNC_H */
