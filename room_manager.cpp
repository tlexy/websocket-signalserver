#include "room_manager.h"

#define GET_ROOM_ID(json) GET_JSON_STRING(json, "roomId", "0")
#define GET_UID(json) GET_JSON_STRING(json, "uid", "0")
#define GET_CMD(json) GET_JSON_STRING(json, "cmd", "")
#define GET_REMOTE_UID(json) GET_JSON_STRING(json, "remote_uid", "0");

RoomManager::RoomManager()
{}

int RoomManager::add_user(int64_t roomid, std::shared_ptr<RoomUser> user)
{
	auto it = _rooms.find(roomid);
	if (it == _rooms.end())
	{
		return 0;
	}
	it->second->users[user->uid] = user;
	return 1;
}

void RoomManager::remove_user(int64_t roomid, int64_t uid)
{
	auto it = _rooms.find(roomid);
	if (it == _rooms.end())
	{
		return;
	}
	if (it->second->users.find(uid) != it->second->users.end())
	{
		it->second->users.erase(uid);
	}
}

std::shared_ptr<Room> RoomManager::get_room(int64_t roomid)
{
	auto it = _rooms.find(roomid);
	if (it == _rooms.end())
	{
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Room> RoomManager::create_room(int64_t roomid)
{
	if (_rooms.find(roomid) != _rooms.end())
	{
		return _rooms[roomid];
	}
	auto roomptr = std::make_shared<Room>();
	roomptr->roomid = roomid;
	_rooms[roomid] = roomptr;
	/*auto userptr = std::make_shared<RoomUser>();
	userptr->uid = uid;
	userptr->hdl = hdl;
	roomptr->users[uid] = userptr;*/
	return roomptr;
}

void RoomManager::on_connect(websocketpp::connection_hdl hdl)
{
	std::cout << "new connection here..." << std::endl;
}

void RoomManager::on_close(websocketpp::connection_hdl hdl)
{
	int64_t roomid = 0;
	int64_t uid = 0;
	if (_room_records.find(hdl.lock().get()) == _room_records.end())
	{
		return;
	}
	roomid = _room_records[hdl.lock().get()];

	if (_user_records.find(hdl.lock().get()) == _user_records.end())
	{
		return;
	}
	uid = _user_records[hdl.lock().get()];

	std::cout << "user connection broken, uid: " << uid << std::endl;
	remove_user(roomid, uid);
	_user_records.erase(hdl.lock().get());
}

void RoomManager::do_bind(std::shared_ptr<WsServer> server)
{
	_server = server;
	_server->set_open_handler(bind(&RoomManager::on_connect, this, ::_1));
	_server->set_message_handler(bind(&RoomManager::on_message, this, _server.get(), ::_1, ::_2));
	_server->set_close_handler(bind(&RoomManager::on_close, this, ::_1));

	//handle function...
	_handlers["join"] = std::bind(&RoomManager::handle_join, this, std::placeholders::_1, std::placeholders::_2);
	_handlers["leave"] = std::bind(&RoomManager::handle_leave, this, std::placeholders::_1, std::placeholders::_2);
	_handlers["sdp-offer"] = std::bind(&RoomManager::handle_sdp_offer, this, std::placeholders::_1, std::placeholders::_2);
	_handlers["sdp-answer"] = std::bind(&RoomManager::handle_sdp_answer, this, std::placeholders::_1, std::placeholders::_2);
	_handlers["candidate"] = std::bind(&RoomManager::handle_candidates, this, std::placeholders::_1, std::placeholders::_2);
}

void RoomManager::on_message(WsServer* s, websocketpp::connection_hdl hdl, message_ptr msg)
{
	std::cout << "on_message called with hdl: " << hdl.lock().get()
		<< " and message: " << msg->get_payload()
		<< std::endl;

	//WsServer::connection_ptr conn_ptr = s->get_con_from_hdl(hdl);

	// check for a special command to instruct the server to stop listening so
	// it can be cleanly exited.
	if (msg->get_payload() == "stop-listening") {
		s->stop_listening();
		return;
	}
	std::string& raw_payload = msg->get_raw_payload();
	Json::Value json;
	bool flag = SUtil::parseJson(raw_payload.c_str(), json);
	if (!flag)
	{
		std::cout << "message not json..." << std::endl;
		return;
	}

	if (json["cmd"].isNull() || !json["cmd"].isString())
	{
		return;
	}
	std::string cmdid = json["cmd"].asString();
	if (_handlers.find(cmdid) != _handlers.end())
	{
		_handlers[cmdid](json, hdl);
	}

	/*try {
		std::string buf = "{\"data\" :\"" + msg->get_payload() + "\",\"type\": \"text\"}";
		s->send(hdl, buf, msg->get_opcode());
	}
	catch (websocketpp::exception const& e) {
		std::cout << "Echo failed because: "
			<< "(" << e.what() << ")" << std::endl;
	}*/
}

void RoomManager::handle_join(Json::Value& json, websocketpp::connection_hdl hdl)
{
	std::string sroomid = GET_ROOM_ID(json);
	std::string suid = GET_UID(json);

	int64_t roomid = std::atoll(sroomid.c_str());
	int64_t uid = std::atoll(suid.c_str());

	auto room_ptr = get_room(roomid);
	if (!room_ptr)
	{
		room_ptr = create_room(roomid);
	}
	if (room_ptr->users.find(uid) != room_ptr->users.end())
	{
		std::cout << "uid in room: " << uid << std::endl;
		return;
	}
	_room_records[hdl.lock().get()] = roomid;
	_user_records[hdl.lock().get()] = uid;

	auto userptr = std::make_shared<RoomUser>();
	userptr->uid = uid;
	userptr->hdl = hdl;
	room_ptr->users[userptr->uid] = userptr;

	resp_join(room_ptr, userptr);
	notify_other_join(room_ptr, uid);
}

void RoomManager::handle_sdp_offer(Json::Value& json, websocketpp::connection_hdl hdl)
{
	std::string sroomid = GET_ROOM_ID(json);
	std::string suid = GET_UID(json);
	std::string remote_suid = GET_REMOTE_UID(json);

	int64_t roomid = std::atoll(sroomid.c_str());
	int64_t uid = std::atoll(suid.c_str());
	int64_t remote_uid = std::atoll(remote_suid.c_str());

	auto room_ptr = get_room(roomid);
	if (!room_ptr)
	{
		std::cout << "handle_sdp_offer, room inexist: " << roomid << std::endl;
		return;
	}
	if (room_ptr->users.find(remote_uid) == room_ptr->users.end())
	{
		std::cout << "handle_sdp_offer, uid not room: " << remote_uid << std::endl;
		return;
	}

	auto user_ptr = room_ptr->users[remote_uid];
	_server->send(user_ptr->hdl, json.toStyledString(), websocketpp::frame::opcode::text);
}

void RoomManager::handle_sdp_answer(Json::Value& json, websocketpp::connection_hdl hdl)
{
	std::string sroomid = GET_ROOM_ID(json);
	std::string suid = GET_UID(json);
	std::string remote_suid = GET_REMOTE_UID(json);

	int64_t roomid = std::atoll(sroomid.c_str());
	int64_t uid = std::atoll(suid.c_str());
	int64_t remote_uid = std::atoll(remote_suid.c_str());

	auto room_ptr = get_room(roomid);
	if (!room_ptr)
	{
		std::cout << "handle_sdp_answer, room inexist: " << roomid << std::endl;
		return;
	}
	if (room_ptr->users.find(remote_uid) == room_ptr->users.end())
	{
		std::cout << "handle_sdp_answer, uid not room: " << remote_uid << std::endl;
		return;
	}

	auto user_ptr = room_ptr->users[remote_uid];
	_server->send(user_ptr->hdl, json.toStyledString(), websocketpp::frame::opcode::text);
}

void RoomManager::handle_candidates(Json::Value& json, websocketpp::connection_hdl hdl)
{
	std::string sroomid = GET_ROOM_ID(json);
	std::string suid = GET_UID(json);
	std::string remote_suid = GET_REMOTE_UID(json);

	int64_t roomid = std::atoll(sroomid.c_str());
	int64_t uid = std::atoll(suid.c_str());
	int64_t remote_uid = std::atoll(remote_suid.c_str());

	auto room_ptr = get_room(roomid);
	if (!room_ptr)
	{
		std::cout << "handle_candidates, room inexist: " << roomid << std::endl;
		return;
	}
	if (room_ptr->users.find(remote_uid) == room_ptr->users.end())
	{
		std::cout << "handle_candidates, uid not room: " << remote_uid << std::endl;
		return;
	}

	auto user_ptr = room_ptr->users[remote_uid];
	_server->send(user_ptr->hdl, json.toStyledString(), websocketpp::frame::opcode::text);
}

void RoomManager::handle_leave(Json::Value& json, websocketpp::connection_hdl hdl)
{
	std::string sroomid = GET_ROOM_ID(json);
	std::string suid = GET_UID(json);

	int64_t roomid = std::atoll(sroomid.c_str());
	int64_t uid = std::atoll(suid.c_str());

	auto room_ptr = get_room(roomid);
	if (!room_ptr)
	{
		std::cout << "handle_leave, room inexist: " << roomid << std::endl;
		return;
	}
	if (room_ptr->users.find(uid) == room_ptr->users.end())
	{
		std::cout << "handle_leave, uid not room: " << uid << std::endl;
		return;
	}

	resp_leave(room_ptr, room_ptr->users[uid]);
	notify_other_leave(room_ptr, uid);
	room_ptr->users.erase(uid);
}

void RoomManager::send_to_room(std::shared_ptr<Room> room_ptr, int64_t uid, const std::string& msg)
{
	auto it = room_ptr->users.begin();
	for (; it != room_ptr->users.end(); ++it)
	{
		if (it->second->uid != uid)
		{
			_server->send(it->second->hdl, msg, websocketpp::frame::opcode::text);
		}
	}
}

void RoomManager::send_to_room_all(std::shared_ptr<Room> room_ptr, const std::string& msg)
{
	auto it = room_ptr->users.begin();
	for (; it != room_ptr->users.end(); ++it)
	{
		_server->send(it->second->hdl, msg, websocketpp::frame::opcode::text);
	}
}

void RoomManager::notify_other_join(std::shared_ptr<Room> room_ptr, int64_t uid)
{
	Json::Value json = Json::nullValue;
	json["cmd"] = "new-peer";
	json["uid"] = std::to_string(uid);
	json["roomid"] = std::to_string(room_ptr->roomid);
	send_to_room(room_ptr, uid, json.toStyledString());
}

void RoomManager::resp_join(std::shared_ptr<Room> room_ptr, std::shared_ptr<RoomUser> user_ptr)
{
	Json::Value json = Json::nullValue;
	json["cmd"] = "resp-join";
	json["uid"] = std::to_string(user_ptr->uid);
	json["roomid"] = std::to_string(room_ptr->roomid);
	json["remote_uid"] = "-1";
	auto it = room_ptr->users.begin();
	for (; it != room_ptr->users.end(); ++it)
	{
		if (it->second->uid != user_ptr->uid)
		{
			json["remote_uid"] = std::to_string(it->second->uid);
			break;
		}
	}
	_server->send(user_ptr->hdl, json.toStyledString(), websocketpp::frame::opcode::text);
}

void RoomManager::notify_other_leave(std::shared_ptr<Room> room_ptr, int64_t uid)
{
	Json::Value json = Json::nullValue;
	json["cmd"] = "peer-leave";
	json["uid"] = std::to_string(uid);
	json["roomid"] = std::to_string(room_ptr->roomid);
	send_to_room(room_ptr, uid, json.toStyledString());
}

void RoomManager::resp_leave(std::shared_ptr<Room> room_ptr, std::shared_ptr<RoomUser> user_ptr)
{
	Json::Value json = Json::nullValue;
	json["cmd"] = "resp-leave";
	json["uid"] = std::to_string(user_ptr->uid);
	json["roomid"] = std::to_string(room_ptr->roomid);

	_server->send(user_ptr->hdl, json.toStyledString(), websocketpp::frame::opcode::text);
}