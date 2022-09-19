
//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef Base64_H
#define Base64_H

#include <string>

#define BASE64_CONVERSION_CHARACTER_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

class Base64
{
    static const char fillchar = '=';
/*
    // 00000000001111111111222222
    // 01234567890123456789012345
    static const char* convert ( "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

                       // 22223333333333444444444455
                       // 67890123456789012345678901
                       "abcdefghijklmnopqrstuvwxyz"

                       // 555555556666
                       // 234567890123
                       "0123456789+/");
*/

public:
    static std::string encode(const std::string data)
    {
	    std::string convert_string(BASE64_CONVERSION_CHARACTER_STRING);
	    std::string::size_type  i;
	    char               c;
	    std::string::size_type  len = data.length();
	    std::string             ret;

	    for (i = 0; i < len; ++i)
	    {
		    c = (data[i] >> 2) & 0x3f;
		    ret.append(1, convert_string[c]);
		    c = (data[i] << 4) & 0x3f;

		    if (++i < len)
			    c |= (data[i] >> 4) & 0x0f;

		    ret.append(1, convert_string[c]);

		    if (i < len)
		    {
			    c = (data[i] << 2) & 0x3f;

			    if (++i < len)
				    c |= (data[i] >> 6) & 0x03;

			    ret.append(1, convert_string[c]);
		    }
		    else
		    {
			    ++i;
			    ret.append(1, fillchar);
		    }

		    if (i < len)
		    {
			    c = data[i] & 0x3f;
			    ret.append(1, convert_string[c]);
		    }
		    else
		    {
			    ret.append(1, fillchar);
		    }
	    }

	    return(ret);
    }

    static std::string decode(std::string data)
    {
	    std::string convert_string(BASE64_CONVERSION_CHARACTER_STRING);
	    std::string::size_type  i;
	    char               c;
	    char               c1;
	    std::string::size_type  len = data.length();
	    std::string             ret;

	    for (i = 0; i < len; ++i)
	    {
		    c = (char) convert_string.find(data[i]);
		    ++i;
		    c1 = (char) convert_string.find(data[i]);
		    c = (c << 2) | ((c1 >> 4) & 0x3);
		    ret.append(1, c);

		    if (++i < len)
		    {
			    c = data[i];

			    if (fillchar == c)
				    break;

			    c = (char) convert_string.find(c);
			    c1 = ((c1 << 4) & 0xf0) | ((c >> 2) & 0xf);
			    ret.append(1, c1);
		    }

		    if (++i < len)
		    {
			    c1 = data[i];

			    if (fillchar == c1)
				    break;

			    c1 = (char) convert_string.find(c1);
			    c = ((c << 6) & 0xc0) | c1;
			    ret.append(1, c);
		    }
	    }

	    return(ret);
    }
};

#endif
