/* $Id$ */

#ifndef DATE_TYPE_H
#define DATE_TYPE_H

/** The minimum starting year/base year of the original TTD */
#define ORIGINAL_BASE_YEAR 1920

/**
 * The offset in days from the '_date == 0' till
 * 'ConvertYMDToDate(ORIGINAL_BASE_YEAR, 0, 1)'
 */
#define DAYS_TILL_ORIGINAL_BASE_YEAR (365 * ORIGINAL_BASE_YEAR + ORIGINAL_BASE_YEAR / 4 - ORIGINAL_BASE_YEAR / 100 + ORIGINAL_BASE_YEAR / 400)

/* The absolute minimum & maximum years in OTTD */
#define MIN_YEAR 0
/* MAX_YEAR, nicely rounded value of the number of years that can
 * be encoded in a single 32 bits date, about 2^31 / 366 years. */
#define MAX_YEAR 5000000

typedef int32  Date;
typedef int32  Year;
typedef uint8  Month;
typedef uint8  Day;

/** Structure to store the unencoded version of a Date in */
struct YearMonthDay {
	Year  year;  ///< The year of the Date
	Month month; ///< The month of the Date
	Day   day;   ///< The day of the Date
};

#endif /* DATE_TYPE_H */
