#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <functional>
#include "common/sutil.h"

typedef websocketpp::server<websocketpp::config::asio> WsServer;
typedef WsServer::message_ptr message_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct RoomUser
{
	int64_t uuid;
	std::string uid;
	websocketpp::connection_hdl hdl;
};

struct Room
{
	int64_t roomid;
	std::unordered_map<std::string, std::shared_ptr<RoomUser>> users;
};

using HandleFunc = std::function<void(Json::Value&, websocketpp::connection_hdl)>;

class RoomManager
{
public:
	RoomManager(std::shared_ptr<WsServer>);

	int add_user(int64_t roomid, std::shared_ptr<RoomUser>);
	void remove_user(int64_t roomid, const std::string& uid);
	std::shared_ptr<Room> get_room(int64_t roomid);
	std::shared_ptr<Room> create_room(int64_t roomid, const std::string& uid, websocketpp::connection_hdl hdl);

private:
	void on_connect(websocketpp::connection_hdl hdl);
	void on_message(WsServer* s, websocketpp::connection_hdl hdl, message_ptr msg);

private:
	std::shared_ptr<WsServer> _server;
	std::unordered_map <int64_t, std::shared_ptr<Room>> _rooms;

	std::unordered_map<std::string, HandleFunc> _handlers;
};

#endif