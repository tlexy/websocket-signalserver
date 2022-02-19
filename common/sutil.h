#pragma once

#ifndef SUTIL_XX_H
#define SUTIL_XX_H

#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include "json.h"
#include <unordered_map>

#define ASSERT_X(x) if(!(x)){std::cout << "ASSERT FAILED:";std::cout << "LINE:" << __LINE__ << std::endl;std::cout << "FILE:" << __FILE__ << std::endl;abort();}

#define TRACK_t \
	std::cout << __FUNCTION__ << "\t" \
	<< __LINE__  << "\t" << __FILE__ \
	<<"\tthread id: " << std::this_thread::get_id() \
	<< std::endl

#define GET_JSON_STRING(json, item, last) (!json.isNull() && json[item].isString()) ? json[item].asString() : last
#define GET_JSON_INT(json, item, last) (!json.isNull() && json[item].isInt()) ? json[item].asInt() : 0
#define GET_JSON_INT64(json, item, last) (!json.isNull() && json[item].isInt64()) ? json[item].asInt64() : 0
#define GET_JSON_BOOL(json, item, last) (!json.isNull() && json[item].isBool()) ? json[item].asBool() : last;

namespace SUtil
{
	//获得当前的系统时间
	unsigned int getCurrentTime();

	std::string trim(const std::string&);

	std::string toCompactString(const std::string &str);
	void toCompactStringWith(const std::string &in_str, std::string& out_str, size_t header_len);

	size_t getFileLength(std::ifstream &_f);
	size_t readAll(std::ifstream &_ifs, char* _buff, int _buff_len);
	size_t readBinaryFile(const char* path, std::string& buf);
	size_t readTextFile(const char* path, std::string& buf);

	bool readJson(const char* filename, Json::Value& json);
	bool parseJson(const char* text, Json::Value& json);

	std::string toWinStylePath(const std::string path);
	void writeFileBinary(const char* path, const char* buf, size_t len);
	bool writeFileBinary2(const char* path, const char* buf, size_t len);
	void appendFileBinary(const char* path, const char* buf, size_t len);

	uint16_t crc16(const char *buf, int len);

	std::string hex2Str(const char* str, unsigned int len);
	std::string str2Hex(const char* str, unsigned int len);
	std::string buffer2Hex(const char* _buffer, size_t _len);
	std::string toDateTimeString(const time_t time);
	std::string toTimeString(const time_t time);
	std::string toDateTimeString2(const time_t time);

	int parseUrl(const std::string&, std::string& doname, std::string& path);
	int parseUrlPort(const std::string&, std::string& doname, std::string& port);
	int parsePortAndType(const std::string&, unsigned short&, bool& is_ssl);

	int64_t getTimeStampMilli();

	std::string getFileName(const std::string& file_path, const std::string& sp = std::string("/"));

}

#endif