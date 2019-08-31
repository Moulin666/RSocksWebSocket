//
// Created by Roman on 30/08/2019.
//

#ifndef base64_hpp
#define base64_hpp

#include <string>

namespace RSocks
{
    static std::string const base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

    // Char is a valid base64
    static inline bool is_base64(unsigned char c)
    {
        return (c == 43 || // +
                (c >= 47 && c <= 57) || // /-9
                (c >= 65 && c <= 90) || // A-Z
                (c >= 97 && c <= 122)); // a-z
    }

    // Encode char buffer into base64
    inline std::string base64_encode(unsigned const char *input, size_t length)
    {
        std::string result;

        int i = 0;
        int j = 0;

        unsigned char tmp_array_3[3];
        unsigned char tmp_array_4[4];

        while (length--)
        {
            tmp_array_3[i++] = *(input++);

            if (i == 3)
            {
                tmp_array_4[0] = (tmp_array_3[0] & 0xfc) >> 2;
                tmp_array_4[1] = ((tmp_array_3[0] & 0x03) << 4) + ((tmp_array_3[1] & 0xf0) >> 4);
                tmp_array_4[2] = ((tmp_array_3[1] & 0x0f) << 2) + ((tmp_array_3[2] & 0xc0) >> 6);
                tmp_array_4[3] = tmp_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++)
                    result += base64_chars[tmp_array_4[i]];

                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 3; j++)
                tmp_array_3[j] = '\0';

            tmp_array_4[0] = (tmp_array_3[0] & 0xfc) >> 2;
            tmp_array_4[1] = ((tmp_array_3[0] & 0x03) << 4) + ((tmp_array_3[1] & 0xf0) >> 4);
            tmp_array_4[2] = ((tmp_array_3[1] & 0x0f) << 2) + ((tmp_array_3[2] & 0xc0) >> 6);
            tmp_array_4[3] = tmp_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++)
                result += base64_chars[tmp_array_4[j]];

            while ((i++ < 3))
                result += '=';
        }

        return result;
    }

    // Encode string into base64
    inline std::string base64_encode(std::string const &input)
    {
        return base64_encode(reinterpret_cast<const unsigned char *>(input.data()), input.size());
    }

    // Decode base64 into string of a raw bytes
    inline std::string base64_decode(std::string const &input)
    {
        std::string result;

        size_t input_length = input.size();

        unsigned char tmp_array_3[3];
        unsigned char tmp_array_4[4];

        int i = 0;
        int j = 0;
        int k = 0;

        while (input_length-- && (input[k] != '=') && is_base64(input[k]))
        {
            tmp_array_4[i++] = input[k];
            k++;

            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                    tmp_array_4[i] = static_cast<unsigned char>(base64_chars.find(tmp_array_4[i]));

                tmp_array_3[0] = (tmp_array_4[0] << 2) + ((tmp_array_4[1] & 0x30) >> 4);
                tmp_array_3[1] = ((tmp_array_4[1] & 0xf) << 4) + ((tmp_array_4[2] & 0x3c) >> 2);
                tmp_array_3[2] = ((tmp_array_4[2] & 0x3) << 6) + tmp_array_4[3];

                for (i = 0; (i < 3); i++)
                    result += tmp_array_3[i];

                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 4; j++)
                tmp_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                tmp_array_4[j] = static_cast<unsigned char>(base64_chars.find(tmp_array_4[j]));

            tmp_array_3[0] = (tmp_array_4[0] << 2) + ((tmp_array_4[1] & 0x30) >> 4);
            tmp_array_3[1] = ((tmp_array_4[1] & 0xf) << 4) + ((tmp_array_4[2] & 0x3c) >> 2);
            tmp_array_3[2] = ((tmp_array_4[2] & 0x3) << 6) + tmp_array_4[3];

            for (j = 0; (j < i - 1); j++)
                result += static_cast<std::string::value_type>(tmp_array_3[j]);
        }

        return result;
    }
}

#endif /* base64_hpp */
