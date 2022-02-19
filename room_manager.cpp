#include "room_manager.h"

RoomManager::RoomManager(std::shared_ptr<WsServer> server)
	:_server(server)
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

void RoomManager::remove_user(int64_t roomid, const std::string& uid)
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

std::shared_ptr<Room> RoomManager::create_room(int64_t roomid, const std::string& uid, websocketpp::connection_hdl hdl)
{
	if (_rooms.find(roomid) != _rooms.end())
	{
		return _rooms[roomid];
	}
	auto roomptr = std::make_shared<Room>();
	roomptr->roomid = roomid;

	auto userptr = std::make_shared<RoomUser>();
	userptr->uid = uid;
	userptr->hdl = hdl;
	roomptr->users[uid] = userptr;
	return roomptr;
}

void RoomManager::on_connect(websocketpp::connection_hdl hdl)
{
	std::cout << "new connection here..." << std::endl;
}

void RoomManager::on_message(WsServer* s, websocketpp::connection_hdl hdl, message_ptr msg)
{
	std::cout << "on_message called with hdl: " << hdl.lock().get()
		<< " and message: " << msg->get_payload()
		<< std::endl;

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
	std::string cmdid = json["cmdid"].asString();
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