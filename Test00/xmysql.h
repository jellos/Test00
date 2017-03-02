#pragma once

#define my_socket SOCKET
#include <winsock.h>
#include <atlstr.h>
#include "rtimer.h"
#include "mysql/include/mysql.h"

struct XMySQL_Result
{
	int iSize;
	int nFields;
	int nRows;
	int pFields;
	int pRows;
};

class XMySQL;

class XMySQL_Exception
{
public:
	XMySQL_Exception(XMySQL *pMySQL, const char *szFunction, const int iError, const char *szError, const char *szSqlState, const int iLine);
	~XMySQL_Exception();

	XMySQL		*GetXMySQL();
	const char	*GetFunction() const;
	int			GetErrNo() const;
	const char	*GetError() const;
	const char	*GetSqlState() const;

private:
	XMySQL		*m_pMySQL;
	CStringA	m_sFunction;
	int			m_iError;
	CStringA	m_sError;
	CStringA	m_sSqlState;
	int			m_iLine;
};

class XMySQL
{
public:
	XMySQL();
	~XMySQL();

	int SetConnectionString(const char *szConnection);
	int SetConnectionString(const WCHAR *szConnection);

	int OpenConnection(bool bException = false);
	int CheckConnection(bool bReconnect = false);
	int CloseConnection();

	int QueryWithoutResult(const char *szQuery, bool bException = true);
	int QueryWithoutResult(const WCHAR *szQuery, bool bException = true);

	int Query(const char *szQuery, bool bException = true);
	int Query(const WCHAR *szQuery, bool bException = true);

	int StoredProcedureWithOneResultSet(const WCHAR *szQuery, bool bException = true);
	int StoredProcedureWithOneResultSet(const char *szQuery, bool bException = true);

	int GetFields();
	int GetField(const int iField, char *&szValue);
	int GetField(const int iField, CString &szValue);

	unsigned long long GetRows();

	int NextRow(bool bException = true);

	int GetFieldValue(const int iField, char *&szValue);
	int GetFieldValue(const int iField, CString &sValue);

	int GetFieldValue(const char *szField, char *&szValue);

	int GetFieldValue(const char *szField, double &dValue);
	int GetFieldValue(const char *szField, int &iValue);
	int GetFieldValue(const char *szField, CString &sValue);

	int GetFieldValue(const WCHAR *szField, double &dValue);
	int GetFieldValue(const WCHAR *szField, int &iValue);
	int GetFieldValue(const WCHAR *szField, CString &sValue);

	int DumpResult(XMySQL_Result *&pResult);

	int CloseResult();

private:
	static LONG			m_lMySQL;
	LONG				m_iInstance;

	MYSQL				*m_pMySQL;
	CStringA			m_sHost;
	CStringA			m_sUser;
	CStringA			m_sPasswd;
	CStringA			m_sDB;
	int					m_iPort;
	int					m_iReadTimeout;
	int					m_iWriteTimeout;
	int					m_iConnectTimeout;
	CStringA			m_sCharset;

	CStringA			m_sQuery;
	CStringA			m_sQueryPrev;

	MYSQL_RES			*m_pResult;
	MYSQL_FIELD			*m_pFields;
	int					m_nFields;
	MYSQL_ROW			m_sqlRow;
	unsigned long long	m_nRows;

	RTimer				m_rTimer;
	int 				m_iDebug;
};