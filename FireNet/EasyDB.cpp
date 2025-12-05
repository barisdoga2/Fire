#include "pch.h"
#include "EasyDB.hpp"

#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>



EasyDB::EasyDB()
{
	try
	{
#ifndef NO_HOST
		driver = get_driver_instance();
		connection = driver->connect("127.0.0.1", "root", "");
		connection->setSchema("fire");
		connected = true;
		std::cout << "DB Manager connected." << std::endl;
#else
		connected = true;
		std::cout << "DB Manager skipped." << std::endl;
#endif
	}
	catch (sql::SQLException e)
	{
		std::cout << "Database Connection failed!" << std::endl;
		connected = false;
	}
}

EasyDB::~EasyDB()
{
	if (connected)
	{
		connected = false;
	}
}

bool EasyDB::IsConnected()
{
	return connected;
}

sql::PreparedStatement* EasyDB::PrepareStatement(std::string query)
{
	if (connected)
	{
		return connection->prepareStatement(query);
	}
	return nullptr;
}
