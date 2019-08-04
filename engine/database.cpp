//
//	Simple wrapper around SQLite for caching
//

#include "database.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
//	Database engine functions
//

int InitSQL(const char *dbpath)	{
	int rc;

	if(db)
		return SQLITE_OK;

	rc = sqlite3_open(dbpath,&db);
	if(rc)	{
		sqlite3_close(db);
		db=NULL;
		return rc;
	}

	RunSQL("PRAGMA synchronous=0;");
	
	return SQLITE_OK;
}


void TermSQL()	{
	if(db)
		sqlite3_close(db);
	db=NULL;
}



int RunSQL(const char *sql)	{
	char *errmsg= NULL;
	int rc;

	if(!db || !sql)
		return SQLITE_MISUSE;

	rc=sqlite3_exec(db,sql,NULL,0,&errmsg);
	if(errmsg)
		sqlite3_free(errmsg);
	return rc;
}


char *GetSQLSingle(const char *sql, int *len, int *error)	{
	if(!db || !sql || !len)
		return NULL;
	sqlite3_stmt *statement=NULL;
	const char *tail=NULL;
	const char *dataptr;
	char *data=NULL;
	int cols,ret;

	if(error)
		*error=SQLITE_OK;
	
	ret = sqlite3_prepare_v2(db,sql,strlen(sql)+1,&statement,&tail);
	if(ret != SQLITE_OK)	{
		if(error)
			*error=ret;
		return NULL;
	}
	
	cols = sqlite3_column_count(statement);
	ret=sqlite3_step(statement);
	if(ret == SQLITE_DONE || cols < 1)	{
		if(error)
			*error=SQLITE_EMPTY;
		sqlite3_finalize(statement);
		return NULL;
	}

	dataptr = (const char *)sqlite3_column_text(statement,0);
	if(dataptr)	{
		*len=sqlite3_column_bytes(statement,0);
		data=(char *)calloc(1,(*len)+1);
		if(data)
			memcpy(data,dataptr,*len);
	}

	sqlite3_finalize(statement);
	return data;
}


SQLROWSET *GetSQL(const char *sql, int *error)	{
	if(!db || !sql || !error)
		return NULL;
	sqlite3_stmt *statement=NULL;
	const char *tail=NULL;
	const char *dataptr;
	SQLROWSET *data;
	int cols,row,ret,len;
	int specint;
	double specfloat;
	
	if(error)
		*error=SQLITE_OK;

	ret=sqlite3_prepare_v2(db,sql,strlen(sql)+1,&statement,&tail);
	if(ret != SQLITE_OK)	{
		if(error)
			*error=ret;
		return NULL;
	}
	
	cols = sqlite3_column_count(statement);
	if(cols<1)	{
		if(error)
			*error=SQLITE_EMPTY;
		sqlite3_finalize(statement);
		return NULL;
	}

	data = new SQLROWSET(cols);
	if(!data)	{
		if(error)
			*error=SQLITE_NOMEM;
		sqlite3_finalize(statement);
		return NULL;
	}

	do	{
		ret=sqlite3_step(statement);
		if(ret == SQLITE_DONE)
			break;
		row=data->AddRow();

		for(int ctr=0;ctr<cols;ctr++)	{
			int t=sqlite3_column_type(statement,ctr);
			int type=SQLTYPE_TEXT;
			switch(t)	{
				case SQLITE_NULL:
					type=SQLTYPE_NULL;
					dataptr=NULL;
					break;
/*				
				//Treat these as text instead
				case SQLITE_INTEGER:
					type=SQLTYPE_INT;
					specint = sqlite3_column_int(statement,ctr);
					len=sizeof(int);
					dataptr=(const char *)&specint;
					break;
				case SQLITE_FLOAT:
					type=SQLTYPE_FLOAT;
					specfloat = sqlite3_column_double(statement,ctr);
					len=sizeof(double);
					dataptr=(const char *)&specfloat;
					break;
*/				
				case SQLITE_BLOB:
					type=SQLTYPE_BINARY;
					dataptr = (const char *)sqlite3_column_blob(statement,ctr);
					len=sqlite3_column_bytes(statement,ctr);
					break;
				default:
					type=SQLTYPE_TEXT;
					dataptr = (const char *)sqlite3_column_text(statement,ctr);
					len=strlen(dataptr)+1;
					break;
				}

			data->SetRecord(ctr,row,dataptr,len,type);
			data->items = data->w*data->h;
		}

	} while(1);

	sqlite3_finalize(statement);
	return data;
}


void FreeSQLString(char *str)	{
	if(str)
		free(str);
}


int InsertSQLBinary(const char *sql, const char *data, int len)	{
	if(!db || !sql || !data)
		return SQLITE_MISUSE;

	sqlite3_stmt *statement=NULL;
	const char *tail=NULL;
	int ret;

	ret=sqlite3_prepare_v2(db,sql,strlen(sql)+1,&statement,&tail) ;
	if(ret!= SQLITE_OK)	{
		printf("Prepare '%s' failed with %d\n",sql,ret);
		return ret;
	}

	if(statement)	{
		ret=sqlite3_bind_blob(statement,1,data,len,SQLITE_TRANSIENT);
		if(ret)
			printf("bind '%s' failed with %d\n",sql,ret);
		ret=sqlite3_step(statement);
		if(ret && ret != SQLITE_DONE)
			printf("step'%s' failed with %d\n",sql,ret);
		sqlite3_finalize(statement) ;
	}

	return ret;
}

//
//  Container class
//

// Constructor

SQLROWSET::SQLROWSET(int cols)	{
	data=NULL;
	lens=NULL;
	types=NULL;
	w=cols;
	h=items=0;

	types = (char *)calloc(1,cols);

}

// Destructor
SQLROWSET::~SQLROWSET()	{
	if(types)
		free(types);
	types=NULL;

	if(lens)
		free(lens);
	lens=NULL;

	if(data)	{
		for(int ctr=0;ctr<items;ctr++)
			if(data[ctr])
				free(data[ctr]);
		free(data);
	}
	data=NULL;

	w=h=items=0;
}


int SQLROWSET::AddRow()	{
	int newitems;
	void **newblock;
	int *newlens;
	if(w == 0)
		return -1;

	// Modify the record array
	newitems = items + w;
	newblock = (void **)realloc(data,sizeof(void *)*newitems);
	if(!newblock)
		return -1;
	data=newblock;
	memset(data+items,0,sizeof(void *)*w);

	// Then the length array
	newlens = (int *)realloc(lens,sizeof(int)*newitems);
	if(!newlens)
		return -1;
	lens=newlens;
	memset(lens+items,0,sizeof(int)*w);

	items=newitems;
	h++;

	return h-1;
}


const void *SQLROWSET::GetRecord(int x, int *len, int *type)	{
	if(x<0 || x>=items || w==0)
		return NULL;
	if(!data || !types)
		return NULL;

	if(type)
		*type = (int)types[x%w];
	if(len)
		*len=lens[x];

	return data[x];
}


const void *SQLROWSET::GetRecord(int x, int y, int *len, int *type)	{
	int idx;
	if(x<0 || x>=w || y<0 || y>=h)
		return NULL;
	if(!data || !types)
		return NULL;

	idx = (y*w)+x;
	if(type)
		*type = (int)types[x];
	if(len)
		*len=lens[idx];

	return data[idx];
}


void SQLROWSET::SetRecord(int x, int y, const void *indata, int len, int type)	{
	int idx;
	if(x<0 || x>=w || y<0 || y>=h)
		return;
	if(!data || !types)
		return;

	idx=(y*w)+x;

	if(data[idx])
		return;

	if(!indata)	{
		data[idx]=NULL;
		lens[idx]=0;
		return;
	}

	types[x]=(char)type;

	data[idx] = (void *)calloc(1,len);
	if(data[idx])
		memcpy(data[idx],indata,len);
	lens[idx]=len;
}


void SQLROWSET::SetRecord(int x, const void *indata, int len, int type)	{
	if(x<0 || x>=items)
		return;
	if(!data || !types)
		return;

	if(data[x])
		return;

	types[x%w]=(char)type;

	if(!indata)	{
		data[x]=NULL;
		lens[x]=0;
		return;
	}

	data[x] = (void *)calloc(1,len);
	if(data[x])
		memcpy(data[x],indata,len);
	lens[x]=len;
}

