/* $Id: string_type.h 23940 2012-02-12 19:46:40Z smatz $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_type.h Types for strings. */

#ifndef STRING_TYPE_H
#define STRING_TYPE_H

#include "core/enum_type.hpp"

/** Settings for the string validation. */
enum StringValidationSettings {
	SVS_NONE                       = 0,      ///< Allow nothing and replace nothing.
	SVS_REPLACE_WITH_QUESTION_MARK = 1 << 0, ///< Replace the unknown/bad bits with question marks.
	SVS_ALLOW_NEWLINE              = 1 << 1, ///< Allow newlines.
	SVS_ALLOW_CONTROL_CODE         = 1 << 2, ///< Allow the special control codes.
};
DECLARE_ENUM_AS_BIT_SET(StringValidationSettings)

#endif /* STRING_TYPE_H */
