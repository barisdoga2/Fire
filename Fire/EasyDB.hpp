#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <string>

#include <jdbc/cppconn/prepared_statement.h>

namespace sql {
	class Driver;
	class Connection;
}

class EasyDB {
private:
	sql::Driver* driver;
	sql::Connection* connection;

	bool connected;

public:
	EasyDB();
	~EasyDB();

	bool IsConnected();

	sql::PreparedStatement* PrepareStatement(std::string query);


};
