//
//	Simple wrapper around SQLite for caching
//

#ifndef __IRDBASE
#define __IRDBASE

#include <sqlite3.h>

static sqlite3 *db;

#define SQLTYPE_NULL		0
#define SQLTYPE_INT		1
#define SQLTYPE_FLOAT		1
#define SQLTYPE_TEXT		3
#define SQLTYPE_BINARY		4

extern int InitSQL(const char *dbpath);
extern void TermSQL();
extern int RunSQL(const char *sql);
extern char *GetSQLSingle(const char *sql, int *len, int *error);
extern class SQLROWSET *GetSQL(const char *sql, int *error);
extern void FreeSQLString(char *str);
extern int InsertSQLBinary(const char *sql, const char *data, int len);

// Rowset for multiple results

class SQLROWSET	{
	public:
		int w,h,items;

		SQLROWSET(int cols);
		~SQLROWSET();
		int AddRow();

		const void *GetRecord(int x, int *len, int *type);
		const void *GetRecord(int x,int y, int *len, int *type);

		void SetRecord(int x, const void *data, int len, int type);
		void SetRecord(int x,int y, const void *data, int len, int type);

	private:
		void **data;
		int *lens;
		char *types;
	};


#endif