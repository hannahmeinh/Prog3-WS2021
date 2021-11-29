#include "BoardRepository.hpp"
#include "Core/Exception/NotImplementedException.hpp"
#include <ctime>
#include <filesystem>
#include <string.h>

using namespace Prog3::Repository::SQLite;
using namespace Prog3::Core::Model;
using namespace Prog3::Core::Exception;
using namespace std;

#ifdef RELEASE_SERVICE
string const BoardRepository::databaseFile = "./data/kanban-board.db";
#else
string const BoardRepository::databaseFile = "../data/kanban-board.db";
#endif

BoardRepository::BoardRepository() : database(nullptr) {

    string databaseDirectory = filesystem::path(databaseFile).parent_path().string();

    if (filesystem::is_directory(databaseDirectory) == false) {
        filesystem::create_directory(databaseDirectory);
    }

    int result = sqlite3_open(databaseFile.c_str(), &database);

    if (SQLITE_OK != result) {
        cout << "Cannot open database: " << sqlite3_errmsg(database) << endl;
    }

    initialize();
}

BoardRepository::~BoardRepository() {
    sqlite3_close(database);
}

void BoardRepository::initialize() {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlCreateTableColumn =
        "create table if not exists column("
        "id integer not null primary key autoincrement,"
        "name text not null,"
        "position integer not null UNIQUE);";

    string sqlCreateTableItem =
        "create table if not exists item("
        "id integer not null primary key autoincrement,"
        "title text not null,"
        "date text not null,"
        "position integer not null,"
        "column_id integer not null,"
        "unique (position, column_id),"
        "foreign key (column_id) references column (id));";

    result = sqlite3_exec(database, sqlCreateTableColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
    result = sqlite3_exec(database, sqlCreateTableItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    // only if dummy data is needed ;)
    //createDummyData();
}

Board BoardRepository::getBoard() {
    throw NotImplementedException();
}

std::vector<Column> BoardRepository::getColumns() {
    string sqlGetColumns = "SELECT * FROM column";

    int result;
    char *errorMessage = nullptr;
    vector<Column> columns;

    result = sqlite3_exec(database, sqlGetColumns.c_str(), queryCallbackAllColumns, &columns, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result && !columns.empty()) {
        for (Column &c : columns) {
            vector<Item> items = getItems(c.getId());
            for (Item &i : items) {
                c.addItem(i);
            }
        }
        return columns;
    }

    return {};
}

std::optional<Column> BoardRepository::getColumn(int id) {
    string sqlGetColumn = "SELECT column.id, name, column.position FROM column WHERE id = '" + to_string(id) + "'";

    int result = 0;
    char *errorMessage = nullptr;
    Column column(-1, "", 0);

    result = sqlite3_exec(database, sqlGetColumn.c_str(), queryCallbackSingleColumn, &column, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        if (column.getId() != -1) {
            for (Item i : getItems(id)) {
                column.addItem(i);
            }
            return column;
        }
    }
    return std::nullopt;
}

std::optional<Column> BoardRepository::postColumn(std::string name, int position) {
    string sqlPostItem =
        "INSERT INTO column('name', 'position') "
        "VALUES('" +
        name + "', '" + to_string(position) + "')";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlPostItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        int columnId = sqlite3_last_insert_rowid(database);
        return Column(columnId, name, position);
    }

    return std::nullopt;
}

std::optional<Prog3::Core::Model::Column> BoardRepository::putColumn(int id, std::string name, int position) {
    throw NotImplementedException();
}

void BoardRepository::deleteColumn(int id) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlDeleteColumn =
        "delete from column "
        "where id = " +
        std::to_string(id);

    result = sqlite3_exec(database, sqlDeleteColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

std::vector<Item> BoardRepository::getItems(int columnId) {
    string sqlGetItems = "SELECT id, title, position, date FROM item WHERE column_id = " + to_string(columnId);

    int result = 0;
    char *errormessage = nullptr;
    std::vector<Item> items;

    result = sqlite3_exec(database, sqlGetItems.c_str(), queryCallbackAllItems, &items, &errormessage);
    handleSQLError(result, errormessage);
    if (items.empty())
        return {};
    return items;
}

std::optional<Item> BoardRepository::getItem(int columnId, int itemId) {
    string sqlGetItem = "SELECT id, title, position, date FROM item WHERE column_id = '" + to_string(columnId) + "' AND id = '" + to_string(itemId) + "'";
    int result = 0;
    char *errorMessage = nullptr;
    Item returnItem(-1, "", -1, "");

    result = sqlite3_exec(database, sqlGetItem.c_str(), queryCallbackSingleItem, &returnItem, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        if (returnItem.getId() != -1)
            return returnItem;
    }
    return std::nullopt;
}

std::optional<Item> BoardRepository::postItem(int columnId, std::string title, int position) {

    time_t now = time(0);
    char *datetime = ctime(&now);

    string sqlPostItem =
        "INSERT INTO item ('title', 'date', 'position', 'column_id') "
        "VALUES ('" +
        title + "', '" + datetime + "', " + to_string(position) + ", " + to_string(columnId) + ");";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlPostItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    int itemId = INVALID_ID;
    if (SQLITE_OK == result) {
        itemId = sqlite3_last_insert_rowid(database);
        return Item(itemId, title, position, datetime);
    }
    return std::nullopt;
}

std::optional<Prog3::Core::Model::Item> BoardRepository::putItem(int columnId, int itemId, std::string title, int position) {
    throw NotImplementedException();
}

void BoardRepository::deleteItem(int columnId, int itemId) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlDeleteItem =
        "delete from item "
        "where id = " +
        std::to_string(itemId) +
        " and column_id = " + std::to_string(columnId);

    result = sqlite3_exec(database, sqlDeleteItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

void BoardRepository::handleSQLError(int statementResult, char *errorMessage) {

    if (statementResult != SQLITE_OK) {
        cout << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
    }
}

void BoardRepository::createDummyData() {

    cout << "creatingDummyData ..." << endl;

    int result = 0;
    char *errorMessage;
    string sqlInsertDummyColumns =
        "insert into column (name, position)"
        "VALUES"
        "(\"prepare\", 1),"
        "(\"running\", 2),"
        "(\"finished\", 3);";

    result = sqlite3_exec(database, sqlInsertDummyColumns.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    string sqlInserDummyItems =
        "insert into item (title, date, position, column_id)"
        "VALUES"
        "(\"in plan\", date('now'), 1, 1),"
        "(\"some running task\", date('now'), 1, 2),"
        "(\"finished task 1\", date('now'), 1, 3),"
        "(\"finished task 2\", date('now'), 2, 3);";

    result = sqlite3_exec(database, sqlInserDummyItems.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

/*
  I know source code comments are bad, but this one is to guide you through the use of sqlite3_exec() in case you want to use it.
  sqlite3_exec takes a "Callback function" as one of its arguments, and since there are many crazy approaches in the wild internet,
  I want to show you how the signature of this "callback function" may look like in order to work with sqlite3_exec()
*/
int BoardRepository::queryCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    return 0;
}

//Wussten nicht wohin, copy-paste
int BoardRepository::queryCallbackAllItems(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    vector<Item> *items = static_cast<vector<Item> *>(data);

    Item item(stoi(fieldValues[0]), fieldValues[1], stoi(fieldValues[2]), fieldValues[3]);
    items->push_back(item);

    return 0;
}

int BoardRepository::queryCallbackSingleItem(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    Item *item = static_cast<Item *>(data);

    item->setID(stoi(fieldValues[0]));
    item->setTitle(fieldValues[1]);
    item->setPos(stoi(fieldValues[2]));
    item->setTimestamp(fieldValues[3]);

    return 0;
}

int BoardRepository::queryCallbackSingleColumn(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    Column *column = static_cast<Column *>(data);

    column->setID(stoi(fieldValues[0]));
    column->setName(fieldValues[1]);
    column->setPos(stoi(fieldValues[2]));

    return 0;
}

int BoardRepository::queryCallbackAllColumns(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    vector<Column> *columns = static_cast<vector<Column> *>(data);

    Column column(stoi(fieldValues[0]), fieldValues[1], stoi(fieldValues[2]));

    columns->push_back(column);

    return 0;
}
