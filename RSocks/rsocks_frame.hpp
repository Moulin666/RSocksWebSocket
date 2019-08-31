//
// Created by Roman on 30/08/2019.
//

#ifndef RSOCKS_RSOCKS_FRAME_HPP
#define RSOCKS_RSOCKS_FRAME_HPP

#include "rsocks_utilities.hpp"

#include <string>
#include <vector>
#include <cstring>

#include <iostream>
#include <algorithm>

#include <netinet/in.h>

namespace RSocks
{
    class frame
    {
    public:
        enum opcode_s
        {
            CONTINUATION_FRAME = 0x00,
            TEXT_FRAME = 0x01,
            BINARY_FRAME = 0x02,
            CONNECTION_CLOSE = 0x08,
            PING = 0x09,
            PONG = 0x0A
        };
        
        typedef enum opcode_s opcode;
        
        static const uint8_t MAX_FRAME_OPCODE = 0x07;
        
        // basic payload byte flags
        static const uint8_t BPB0_OPCODE = 0x0F;
        static const uint8_t BPB0_RSV3 = 0x10;
        static const uint8_t BPB0_RSV2 = 0x20;
        static const uint8_t BPB0_RSV1 = 0x40;
        static const uint8_t BPB0_FIN = 0x80;
        static const uint8_t BPB1_PAYLOAD = 0x7F;
        static const uint8_t BPB1_MASK = 0x80;
        
        static const uint8_t BASIC_PAYLOAD_LIMIT = 0x7D; // 125
        static const uint8_t BASIC_PAYLOAD_16BIT_CODE = 0x7E; // 126
        static const uint16_t PAYLOAD_16BIT_LIMIT = 0xFFFF; // 2^16, 65535
        static const uint8_t BASIC_PAYLOAD_64BIT_CODE = 0x7F; // 127
        static const uint64_t PAYLOAD_64BIT_LIMIT = 0x7FFFFFFFFFFFFFFF; // 2^63
        
        static const unsigned int BASIC_HEADER_LENGTH = 2;
        static const unsigned int MAX_HEADER_LENGTH = 14;
        static const uint8_t extended_header_length = 12;
        static const uint64_t max_payload_size = 100000000; // 100MB
        
        // create an empty frame for writing into
        frame()
        {
            memset(m_header, 0, MAX_HEADER_LENGTH);
        }
        
        // get pointers to underlying buffers
        char *get_header()
        {
            return m_header;
        }
        
        char *get_extended_header()
        {
            return m_header + BASIC_HEADER_LENGTH;
        }
        
        unsigned int get_header_len() const
        {
            unsigned int result = 2;
            
            if (get_masked())
                tmp += 4;
            
            if (get_basic_size() == 126)
                tmp += 2;
            else if (get_basic_size() == 127)
                tmp += 8;
            
            return result;
        }
        
        char *get_masking_key()
        {
            if (m_extended_header_bytes_needed > 0)
                throw "attempted to get masking_key before reading full header";
            
            return m_masking_key;
        }
        
        // get and set header bits
        bool get_fin() const
        {
            return ((m_header[0] & BPB0_FIN) == BPB0_FIN);
        }
        
        void set_fin(bool fin)
        {
            if (fin)
                m_header[0] |= BPB0_FIN;
            else
                m_header[0] &= (0xFF ^ BPB0_FIN)
        }
        
        // get and set reserved bits
        bool get_rsv1() const
        {
            return ((m_header[0] & BPB0_RSV1) == BPB0_RSV1);
        }
        
        bool get_rsv2() const
        {
            return ((m_header[0] & BPB0_RSV2) == BPB0_RSV2);
        }
        
        bool get_rsv3() const
        {
            return ((m_header[0] & BPB0_RSV3) == BPB0_RSV3);
        }
        
        void set_rsv1(bool b)
        {
            if (b)
                m_header[0] |= BPB0_RSV1;
            else
                m_header[0] &= (0xFF ^ BPB0_RSV1)
        }
        
        void set_rsv2(bool b)
        {
            if (b)
                m_header[0] |= BPB0_RSV2;
            else
                m_header[0] &= (0xFF ^ BPB0_RSV2)
        }
        
        void set_rsv3(bool b)
        {
            if (b)
                m_header[0] |= BPB0_RSV3;
            else
                m_header[0] &= (0xFF ^ BPB0_RSV3)
        }
        
        opcode get_opcode() const
        {
            return opcode(m_header[0] & BPB0_OPCODE);
        }
        
        void set_opcode(opcode op)
        {
            if (op > 0x0F)
                throw "Invalid opcode";
            
            if (get_basic_size() > BASIC_PAYLOAD_LIMIT && is_control())
                throw "Control frames can't have large payloads";
            
            // Clear and set bits to op
            m_header[0] &= (0xFF ^ BPB0_OPCODE);
            m_header[0] |= op;
        }
        
        bool get_masked() const
        {
            return ((m_header[1] & BPB1_MASK) == BPB1_MASK);
        }
        
        void set_masked(bool masked)
        {
            if (masked)
            {
                m_header[1] |= BPB1_MASK;
                generate_masking_key();
            }
            else
            {
                m_header[1] &= (0xFF ^ BPB1_MASK);
                clear_masking_key();
            }
        }
        
        uint8_t get_basic_size() const
        {
            return m_header[1] & BPB1_PAYLOAD;
        }
        
        size_t get_payload_size() const
        {
            if (m_extended_header_bytes_needed > 0)
                throw "attempted to get payload size before reading full header";
            
            return m_payload.size();
        }
        
        std::vector<unsigned char> &get_payload()
        {
            return m_payload;
        }
        
        void set_payload(const std::vector<unsigned char> source)
        {
            set_payload_helper(source.size());
            std::copy(source.begin(), source.end(), m_payload.begin());
        }
        
        void set_payload(const std::string source)
        {
            set_payload_helper(source.size());
            std::copy(source.begin(), source.end(), m_payload.begin());
        }
        
        void set_payload_helper(size_t s)
        {
            if (s > max_payload_size)
                throw "requested payload is over implementation defined limit";
            
            // limits imposed by the websocket spec
            if (s > BASIC_PAYLOAD_LIMIT && get_opcode() > MAX_FRAME_OPCODE)
                throw "control frames can't have large payloads";
            
            if (s <= BASIC_PAYLOAD_LIMIT)
                m_header[1] = s;
            else if (s <= PAYLOAD_16BIT_LIMIT)
            {
                m_header[1] = BASIC_PAYLOAD_16BIT_CODE;
                
                // this reinterprets the second pair of bytes in m_header as a
                // 16 bit int and writes the payload size there as an integer
                // in network byte order
                *reinterpret_cast<uint16_t *>(&m_header[BASIC_HEADER_LENGTH]) = htons(s);
            }
            else if (s <= PAYLOAD_64BIT_LIMIT)
            {
                m_header[1] = BASIC_PAYLOAD_64BIT_CODE;
                *reinterpret_cast<uint64_t *>(&m_header[BASIC_HEADER_LENGTH]) = htonll(s);
            }
            else
                throw "payload size limit is 63 bits";
            
            m_payload.resize(s);
        }
        
        bool is_control() const
        {
            return (get_opcode() > MAX_FRAME_OPCODE);
        }
        
        // reads basic header, sets and returns m_header_bits_needed
        unsigned int process_basic_header()
        {
            m_extended_header_bytes_needed = 0;
            m_payload.empty();
            
            m_extended_header_bytes_needed = get_header_len() - BASIC_HEADER_LENGTH;
            
            return m_extended_header_bytes_needed;
        }
        
        void process_extended_header()
        {
            m_extended_header_bytes_needed = 0;
            
            uint8_t s = get_basic_size();
            uint64_t payload_size;
            
            int mask_index = BASIC_HEADER_LENGTH;
            
            if (s <= BASIC_PAYLOAD_LIMIT)
                payload_size = s;
            else if (s == BASIC_PAYLOAD_16BIT_CODE)
            {
                payload_size = ntohs(*(
                        reinterpret_cast<uint16_t *>(&m_header[BASIC_HEADER_LENGTH])
                ));
                mask_index += 2;
            }
            else if (s == BASIC_PAYLOAD_64BIT_CODE)
            {
                payload_size = ntohll(*(
                        reinterpret_cast<uint64_t *>(&m_header[BASIC_HEADER_LENGTH])
                ));
                
                mask_index += 8;
            }
            else
                throw "Invalid basic size in process_extended_header";
        }
        
        void process_payload()
        {
            // unmask payload one byte at a time
            for (uint64_t i = 0; i < m_payload.size(); i++)
                m_payload[i] = (m_payload[i] ^ m_masking_key[i % 4]);
        }
        
        void process_payload2()
        {
            uint32_t key = *((uint32_t *) m_masking_key);
            uint64_t s = (m_payload.size() / 4);
            uint64_t i = 0;
            
            // chunks of 4
            for (i = 0; i < s; i += 4)
                ((uint32_t * )(&m_payload[0]))[i] = (((uint32_t * )(&m_payload[0]))[i] ^ key);
            
            // finish the last few
            for (i = s; i < m_payload.size(); i++)
                m_payload[i] = (m_payload[i] ^ m_masking_key[i % 4]);
        }
        
        bool validate_basic_header() const
        {
            // check for control frame size
            if (get_basic_size() > BASIC_PAYLOAD_LIMIT && is_control())
                return false;
            
            // check for reserved opcodes
            if (get_rsv1() || get_rsv2() || get_rsv3())
                return false;
            
            // check for reserved opcodes
            opcode op = get_opcode();
            if (op > 0x02 && op < 0x08)
                return false;
            else if (op > 0x0A)
                return false;
            
            // check for fragmented control message
            if (is_control() && !get_fin())
                return false;
            
            return true;
        }
        
        void generate_masking_key()
        {
            throw "Not implemented exception"
        }
        
        void clear_masking_key()
        {
            m_masking_key[0] = 0;
            m_masking_key[1] = 0;
            m_masking_key[2] = 0;
            m_masking_key[3] = 0;
        }
    
    private:
        char m_header[MAX_HEADER_LENGTH];
        std::vector<unsigned char> m_payload;
        
        char m_masking_key[4];
        unsigned int m_extended_header_bytes_needed;
    };
}

#endif //RSOCKS_RSOCKS_FRAME_HPP
