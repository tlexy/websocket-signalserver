#include "room_manager.h"

#include <iostream>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
//typedef server::message_ptr message_ptr;

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
        << " and message: " << msg->get_payload()
        << std::endl;

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening") {
        s->stop_listening();
        return;
    }

    try {
        std::string buf = "{\"data\" :\"" + msg->get_payload() + "\",\"type\": \"text\"}";
        s->send(hdl, buf, msg->get_opcode());
    }
    catch (websocketpp::exception const& e) {
        std::cout << "Echo failed because: "
            << "(" << e.what() << ")" << std::endl;
    }
}

void on_connect(websocketpp::connection_hdl hdl)
{
    std::cout << "new connection here..." << std::endl;
}

int main1() {
    // Create a server endpoint
    server echo_server;

    //try {
    //    // Set logging settings
    //    echo_server.set_access_channels(websocketpp::log::alevel::all);
    //    echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    //    // Initialize Asio
    //    echo_server.init_asio();

    //    echo_server.set_open_handler(bind(&on_connect, ::_1));

    //    // Register our message handler
    //    echo_server.set_message_handler(bind(&on_message, &echo_server, ::_1, ::_2));

    //    // Listen on port 9002
    //    echo_server.listen(8010);

    //    // Start the server accept loop
    //    echo_server.start_accept();

    //    // Start the ASIO io_service run loop
    //    echo_server.run();
    //}
    //catch (websocketpp::exception const& e) {
    //    std::cout << e.what() << std::endl;
    //}
    //catch (...) {
    //    std::cout << "other exception" << std::endl;
    //}

    /*std::unordered_map<int, int>  test_map;
    auto ret = test_map.erase(0);*/
    std::cin.get();
    return 0;
}

int main() {

    auto server_ptr = std::make_shared<WsServer>();
    server_ptr->set_access_channels(websocketpp::log::alevel::message_header);
    server_ptr->clear_access_channels(websocketpp::log::alevel::all);

    auto room_mgr = std::make_shared<RoomManager>();
    room_mgr->do_bind(server_ptr);

    int port = 8010;
    try
    {
        server_ptr->init_asio();
        server_ptr->listen(port);
        std::cout << "listen at: " << port << std::endl;
        server_ptr->start_accept();

        server_ptr->run();
    }
    catch (websocketpp::exception const& e)
    {
        std::cout << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "other exception" << std::endl;
    }



    return 0;
}
