#pragma once

#include <sqlite3.h>
#include <string>
#include <iostream>
#include <vector>
#include <optional>



class SqLite3 {
    sqlite3* db = nullptr;

public:
    bool Init(const std::string& filePath)
    {
        if (sqlite3_open(filePath.c_str(), &db) != SQLITE_OK)
        {
            std::cerr << "Could not open DB! Reason: " << sqlite3_errmsg(db) << "\n";
            return false;
        }

        // important performance and stability settings
        Execute("PRAGMA journal_mode=WAL;");
        Execute("PRAGMA synchronous=NORMAL;");
        Execute("PRAGMA foreign_keys=ON;");

        return true;
    }

    void Close()
    {
        if (db)
            sqlite3_close(db);
        db = nullptr;
    }

    bool Execute(const std::string& sql)
    {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL Exec Error: " << (errMsg ? errMsg : "no message") << "\n";
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }

    std::vector<std::vector<std::string>> Query(const std::string& sql)
    {
        std::vector<std::vector<std::string>> results;

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "SQL Prepare Error: " << sqlite3_errmsg(db) << "\n";
            return results;
        }

        int cols = sqlite3_column_count(stmt);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            std::vector<std::string> row;
            row.reserve(cols);

            for (int i = 0; i < cols; i++)
            {
                const unsigned char* val = sqlite3_column_text(stmt, i);
                row.push_back(val ? (const char*)val : "");
            }

            results.push_back(row);
        }

        sqlite3_finalize(stmt);
        return results;
    }

    std::optional<int> QueryInt(const std::string& sql)
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "SQL Prepare Error: " << sqlite3_errmsg(db) << "\n";
            return std::nullopt;
        }

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int value = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return value;
        }

        sqlite3_finalize(stmt);
        return std::nullopt;
    }

};
