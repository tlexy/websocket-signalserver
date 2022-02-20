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
	int64_t uid;
	websocketpp::connection_hdl hdl;
};

struct Room
{
	int64_t roomid;
	std::unordered_map<int64_t, std::shared_ptr<RoomUser>> users;
};

class RoomManager
{
public:
	using HandleFunc = std::function<void(Json::Value&, websocketpp::connection_hdl)>;
	RoomManager();

	void do_bind(std::shared_ptr<WsServer>);

	int add_user(int64_t roomid, std::shared_ptr<RoomUser>);
	void remove_user(int64_t roomid, int64_t uid);
	std::shared_ptr<Room> get_room(int64_t roomid);
	std::shared_ptr<Room> create_room(int64_t roomid);

private:
	void on_connect(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_message(WsServer* s, websocketpp::connection_hdl hdl, message_ptr msg);

	//处理协议
	void handle_join(Json::Value& json, websocketpp::connection_hdl);
	void handle_sdp_offer(Json::Value& json, websocketpp::connection_hdl);
	void handle_sdp_answer(Json::Value& json, websocketpp::connection_hdl);
	void handle_candidates(Json::Value& json, websocketpp::connection_hdl);
	void handle_leave(Json::Value& json, websocketpp::connection_hdl);

	//发送消息
	void notify_other_join(std::shared_ptr<Room>, int64_t);
	void resp_join(std::shared_ptr<Room> room_ptr, std::shared_ptr<RoomUser>);

	void notify_other_leave(std::shared_ptr<Room>, int64_t);
	void resp_leave(std::shared_ptr<Room> room_ptr, std::shared_ptr<RoomUser>);

	//发送到房间用户，除了自己
	void send_to_room(std::shared_ptr<Room>, int64_t uid, const std::string& msg);
	//发送到房间的所有用户，包括自己
	void send_to_room_all(std::shared_ptr<Room>, const std::string& msg);

private:
	std::shared_ptr<WsServer> _server;
	std::unordered_map <int64_t, std::shared_ptr<Room>> _rooms;

	std::unordered_map<std::string, HandleFunc> _handlers;

	std::unordered_map<void*, int64_t> _room_records;
	std::unordered_map<void*, int64_t> _user_records;
};

#endif