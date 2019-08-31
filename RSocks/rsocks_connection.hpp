//
// Created by Roman on 30/08/2019.
//

#ifndef RSOCKS_RSOCKS_CONNECTION_HPP
#define RSOCKS_RSOCKS_CONNECTION_HPP

#include <boost/shared_ptr.hpp>

#include <string>
#include <map>

namespace RSocks
{
    class connection
    {
    public:
        virtual bool validate(session_ptr client) = 0;
        virtual bool connect(session_ptr client) = 0;
        virtual bool disconnect(session_ptr client) = 0;
        virtual bool message(session_ptr client, const std::string &msg) = 0;
        virtual bool message(session_ptr client, const std::vector<unsigned char> &data) = 0;
    };

    typedef boost::sharedptr<connection> connection_ptr;
}

#include "rsocks_session.hpp"

#endif //RSOCKS_RSOCKS_CONNECTION_HPP
