#include "SQLiteWrapper.h"

SQLiteWrapper::SQLiteWrapper() : db_(0) {
}

bool SQLiteWrapper::Open(std::string const& db_file) {
	if (sqlite3_open(db_file.c_str(), &db_) != SQLITE_OK) {
		return false;
	}
	return true;
}

bool SQLiteWrapper::SelectStmt(std::string const& stmt, ResultTable& res) {
	char *errmsg;
	int   ret;

	res.reset();

	ret = sqlite3_exec(db_, stmt.c_str(), SelectCallback, static_cast<void*> (&res), &errmsg);

	if (ret != SQLITE_OK) {
		return false;
	}
	return true;
}

// TODO parameter p_col_names
int SQLiteWrapper::SelectCallback(void *p_data, int num_fields, char **p_fields, char** p_col_names) {
	ResultTable* res = reinterpret_cast<ResultTable*>(p_data);

	ResultRecord record;

#ifdef SQLITE_WRAPPER_REPORT_COLUMN_NAMES
	// Hubert Castelain: column names in the first row of res if res is empty

	if (res->records_.size() == 0) {
		ResultRecord col_names;

		for (int i = 0; i < num_fields; i++) {
			if (p_fields[i]) col_names.fields_.push_back(p_col_names[i]);
			else
				col_names.fields_.push_back("(null)"); // or what else ?
		}
		res->records_.push_back(col_names);
	}
#endif

	for (int i = 0; i < num_fields; i++) {
		// Hubert Castelain: special treatment if null
		if (p_fields[i]) record.fields_.push_back(p_fields[i]);
		else             record.fields_.push_back("<null>");
	}

	res->records_.push_back(record);

	return 0;
}

SQLiteStatement* SQLiteWrapper::Statement(std::string const& statement) {
	SQLiteStatement* stmt;
	try {
		stmt = new SQLiteStatement(statement, db_);
		return stmt;
	}
	catch (const char* e) {
		return 0;
	}
}

SQLiteStatement::SQLiteStatement(std::string const& statement, sqlite3* db) {
	if (sqlite3_prepare(
		db,
		statement.c_str(),  // stmt
		-1,                  // If than zero, then stmt is read up to the first nul terminator
		&stmt_,
		0                   // Pointer to unused portion of stmt
	)
		!= SQLITE_OK) {
		throw sqlite3_errmsg(db);
	}

	if (!stmt_) {
		throw "stmt_ is 0";
	}
}

SQLiteStatement::~SQLiteStatement() {
	if (stmt_) sqlite3_finalize(stmt_);
}

SQLiteStatement::SQLiteStatement() :
	stmt_(0)
{
}

bool SQLiteStatement::Bind(int pos_zero_indexed, std::string const& value) {
	if (sqlite3_bind_text(
		stmt_,
		pos_zero_indexed + 1,  // Index of wildcard
		value.c_str(),
		value.length(),      // length of text
		SQLITE_TRANSIENT     // SQLITE_TRANSIENT: SQLite makes its own copy
	)
		!= SQLITE_OK) {
		return false;
	}
	return true;
}

bool SQLiteStatement::Bind(int pos_zero_indexed, double value) {
	if (sqlite3_bind_double(
		stmt_,
		pos_zero_indexed + 1,  // Index of wildcard
		value
	)
		!= SQLITE_OK) {
		return false;
	}
	return true;
}

bool SQLiteStatement::Bind(int pos_zero_indexed, int value) {
	if (sqlite3_bind_int(
		stmt_,
		pos_zero_indexed + 1,  // Index of wildcard
		value
	)
		!= SQLITE_OK) {
		return false;
	}
	return true;
}

bool SQLiteStatement::BindNull(int pos_zero_indexed) {
	if (sqlite3_bind_null(
		stmt_,
		pos_zero_indexed + 1  // Index of wildcard
	)
		!= SQLITE_OK) {
		return false;
	}
	return true;
}

bool SQLiteStatement::Execute() {
	int rc = sqlite3_step(stmt_);
	if (rc == SQLITE_BUSY) {
		return false;
	}
	if (rc == SQLITE_ERROR) {
		return false;
	}
	if (rc == SQLITE_MISUSE) {
		return false;
	}
	if (rc != SQLITE_DONE) {
		return false;
	}
	sqlite3_reset(stmt_);
	return true;
}

SQLiteStatement::dataType SQLiteStatement::DataType(int pos_zero_indexed) {
	return dataType(sqlite3_column_type(stmt_, pos_zero_indexed));
}

int SQLiteStatement::ValueInt(int pos_zero_indexed) {
	return sqlite3_column_int(stmt_, pos_zero_indexed);
}

std::string SQLiteStatement::ValueString(int pos_zero_indexed) {
	return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt_, pos_zero_indexed)));
}

bool SQLiteStatement::RestartSelect() {
	sqlite3_reset(stmt_);
	return true;
}

bool SQLiteStatement::Reset() {
	int rc = sqlite3_step(stmt_);

	sqlite3_reset(stmt_);

	if (rc == SQLITE_ROW) return true;
	return false;
}

bool SQLiteStatement::NextRow() {
	int rc = sqlite3_step(stmt_);

	if (rc == SQLITE_ROW) {
		return true;
	}
	if (rc == SQLITE_DONE) {
		sqlite3_reset(stmt_);
		return false;
	}
	else if (rc == SQLITE_MISUSE) {
	}
	else if (rc == SQLITE_BUSY) {
	}
	else if (rc == SQLITE_ERROR) {
	}
	return false;
}

bool SQLiteWrapper::DirectStatement(std::string const& stmt) {
	char *errmsg;
	int   ret;

	ret = sqlite3_exec(db_, stmt.c_str(), 0, 0, &errmsg);

	if (ret != SQLITE_OK) {
		return false;
	}
	return true;
}

std::string SQLiteWrapper::LastError() {
	return sqlite3_errmsg(db_);
}

bool SQLiteWrapper::Begin() {
	return DirectStatement("begin");
}

bool SQLiteWrapper::Commit() {
	return DirectStatement("commit");
}

bool SQLiteWrapper::Rollback() {
	return DirectStatement("rollback");
}