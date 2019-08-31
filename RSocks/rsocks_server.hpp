//
// Created by Roman on 30/08/2019.
//

#ifndef RSOCKS_RSOCKS_SERVER_HPP
#define RSOCKS_RSOCKS_SERVER_HPP

#include "rsocks_session.hpp"

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <iostream>

using boost::asio::ip::tcp;

namespace RSocks
{
    class server
    {
    public:
        // ctor
        server(boost::asio::io_service &io_service,
               const tcp::endpoint &endpoint,
               const std::string &host,
               connection_ptr defc) : m_host(host),
                                      m_io_service(io_service),
                                      m_acceptor(io_service, endpoint),
                                      m_def_con_handler(defc)
        {
            this->start_accept();
        }
        
        // Creates a new session object and connects the next websocket
        // connection to it.
        void start_accept()
        {
            session_ptr new_ws(new session(m_io_service, m_host, m_def_connection));
    
            m_acceptor.async_accept(
                    new_ws->socket(),
                    boost::bind(
                            &server::handle_accept,
                            this,
                            new_ws,
                            boost::asio::placeholders::error
                    )
            );
        }
        
        // If no errors starts the session's read loop and returns to the
        // start_accept phase.
        void handle_accept(session_ptr session, const boost::system::error_code &error)
        {
            if (!error)
                session->start();
            else
                throw "Has exception";
            
            this->start_accept();
        }
    
    private:
        std::string m_host;
        boost::asio::io_service &m_io_service;
        tcp::acceptor m_acceptor;
        connection_ptr m_def_connection;
    };
    
    typedef boost::shared_ptr <server> server_ptr;
}

#endif //RSOCKS_RSOCKS_SERVER_HPP
