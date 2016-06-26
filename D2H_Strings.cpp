#include "D2H.h"

typedef struct {
	A_u_long	index;
	A_char		str[256];
} TableString;



TableString		g_strs[StrID_NUMTYPES] = {
	StrID_NONE,						"",
	StrID_Name,						"D2H",
	StrID_Description,				"Effect to convert circular domemaster video into hexadeca-domemaster video.",
	StrID_Divide_Number_Param_Name,		"Number of Divide",
};


char	*GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
	