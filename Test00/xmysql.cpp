#include "stdafx.h"
#include "xmysql.h"

#pragma comment(lib, "./mysql/lib/mysqlclient.lib")

/*
#if _MSC_VER >= 1800
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif
*/

LONG XMySQL::m_lMySQL = 0;

XMySQL_Exception::XMySQL_Exception(XMySQL *pMySQL, const char *szFunction, const int iError, const char *szError, const char *szSqlState, const int iLine)
{
	m_pMySQL = pMySQL;

	if (szFunction)
		m_sFunction = szFunction;

	m_iError = iError;

	if (szError)
		m_sError = szError;

	if (szSqlState)
		m_sSqlState = szSqlState;

	m_iLine = iLine;
}

XMySQL_Exception::~XMySQL_Exception()
{
}

XMySQL *XMySQL_Exception::GetXMySQL()
{
	return m_pMySQL;
}

const char *XMySQL_Exception::GetFunction() const
{
	return (const char *)m_sFunction;
}

int XMySQL_Exception::GetErrNo() const
{
	return (const int)m_iError;
}

const char *XMySQL_Exception::GetError() const
{
	return (const char *)m_sError;
}

const char *XMySQL_Exception::GetSqlState() const
{
	return (const char *)m_sSqlState;
}

XMySQL::XMySQL()
{
	if ((m_iInstance = InterlockedIncrement(&m_lMySQL)) == 1)
		mysql_library_init(0, NULL, NULL);	// hoeft niet per se, mysql_init doet t impliciet ook en dit is toch ook niet thread safe

	if (m_iDebug && (m_iInstance == 1))
		OutputDebugString("XMySQL::XMySQL() MySQL client version " + CString(mysql_get_client_info()) + "\n");

	m_iPort				= 0;
	m_iConnectTimeout	= 0;
	m_iReadTimeout		= 0;
	m_iWriteTimeout		= 0;
	m_sCharset			= "utf8";

	m_pMySQL			= NULL;
	m_pResult			= NULL;
	m_pFields			= NULL;
	m_nFields			= 0;

	m_iDebug			= 1;	// 1: alleen queries langer dan 0.5 sec, 2: alles
}

XMySQL::~XMySQL()
{
	if (InterlockedDecrement(&m_lMySQL) == 0)
		mysql_library_end();
}

int XMySQL::SetConnectionString(const char *szConnection)
{
	char szParameter[1024], szValue[1024];

	while (sscanf_s(szConnection, "%[^=]=%[^;]", szParameter, _countof(szParameter), szValue, _countof(szValue)) == 2)
	{
		if (!_stricmp(szParameter, "host") || !_stricmp(szParameter, "server"))
			m_sHost = szValue;
		if (!_stricmp(szParameter, "database"))
			m_sDB = szValue;
		if (!_stricmp(szParameter, "user")||!_stricmp(szParameter, "uid"))
			m_sUser = szValue;
		if (!_stricmp(szParameter, "password") || !_stricmp(szParameter, "pwd"))
			m_sPasswd = szValue;
		if (!_stricmp(szParameter, "port"))
			m_iPort = atoi(szValue);
		if (!_stricmp(szParameter, "connecttimeout"))
			m_iConnectTimeout = atoi(szValue);
		if (!_stricmp(szParameter, "readtimeout"))
			m_iReadTimeout = atoi(szValue);
		if (!_stricmp(szParameter, "writetimeout"))
			m_iWriteTimeout = atoi(szValue);
		if (!_stricmp(szParameter, "charset"))
			m_sCharset = szValue;

		szConnection = strchr(szConnection, ';');
		if (szConnection)
			szConnection++;
		else
			break;
	}

	return 0;
}

int XMySQL::SetConnectionString(const WCHAR *szConnection)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szConnection, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szConnection, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return SetConnectionString(utf8);
}

int XMySQL::OpenConnection(bool bException)
{
	int		iConnected = 0;
	MYSQL	*pMySQL;
	double	dDuration;
	CString	sDebug;

	if (m_pMySQL == NULL)
		m_pMySQL = mysql_init(NULL);

	m_pResult = NULL;
	m_pFields = NULL;
	m_nFields = 0;

	mysql_options(m_pMySQL, MYSQL_OPT_COMPRESS, 0);
	mysql_options(m_pMySQL, MYSQL_OPT_CONNECT_TIMEOUT, &m_iConnectTimeout);	// doet niks met standaard connector, maar nu wel met aanpassingen id source (zie patch in mysql folder :)
	mysql_options(m_pMySQL, MYSQL_OPT_READ_TIMEOUT, &m_iReadTimeout);
//	mysql_options(m_pMySQL, MYSQL_OPT_RECONNECT, &m_bReconnect);
	mysql_options(m_pMySQL, MYSQL_OPT_WRITE_TIMEOUT, &m_iWriteTimeout);
	mysql_options(m_pMySQL, MYSQL_SET_CHARSET_NAME, m_sCharset);

	m_rTimer.StartTimer();
	pMySQL = mysql_real_connect(m_pMySQL, 
								m_sHost, 
								m_sUser, 
								m_sPasswd, 
								m_sDB.IsEmpty()?(const char *)NULL:(const char *)m_sDB, 
								m_iPort, 
								NULL, 
								CLIENT_MULTI_RESULTS | 
								CLIENT_MULTI_STATEMENTS | 
								CLIENT_COMPRESS);
	dDuration = m_rTimer.GetElapsedTime();

	if (m_iDebug)
	{
		if (dDuration > 0.5)
		{
			sDebug.Format(_T("XMySQL::OpenConnection() mysql_real_connect %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
		else if (m_iDebug > 1)
		{
			sDebug.Format(_T("XMySQL::OpenConnection() mysql_real_connect %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
	}

	if (pMySQL == NULL)
	{
		if (bException)
			throw XMySQL_Exception(this, "XMySQL::OpenConnection() mysql_real_connect", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
		else if (m_iDebug)
			OutputDebugString("XMySQL::OpenConnection() mysql_real_connect: " + CString(mysql_error(m_pMySQL)) + "\n");
	}
	else
	{
		iConnected = 1;

		if (m_iDebug)
		{
			OutputDebugString("XMySQL::OpenConnection() MySQL host info: " + CString(mysql_get_host_info(m_pMySQL)) + "\n");
			sDebug.Format(_T("XMySQL::OpenConnection() MySQL protocol version %d\n"), mysql_get_proto_info(m_pMySQL));
			OutputDebugString(sDebug);
			OutputDebugString("XMySQL::OpenConnection() MySQL server version " + CString(mysql_get_server_info(m_pMySQL)) + "\n");
		}
	}

	return iConnected;
}

int XMySQL::CheckConnection(bool bReconnect)
{
	int		iPing, iConnected = 0;
	double	dDuration;
	CString	sDebug;

	m_rTimer.StartTimer();
	iPing = mysql_ping(m_pMySQL);
	dDuration = m_rTimer.GetElapsedTime();

	if (m_iDebug)
	{
		if (dDuration > 0.5)
		{
			sDebug.Format(_T("XMySQL::CheckConnection() mysql_ping %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
		else if (m_iDebug > 1)
		{
			sDebug.Format(_T("XMySQL::CheckConnection() mysql_ping %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
	}

	if (iPing)
	{
//		if (bException)
//			throw XMySQL_Exception("XMySQL::CheckConnection() mysql_ping", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
//		else
//			OutputDebugString("XMySQL::CheckConnection() mysql_ping: " + CString(mysql_error(m_pMySQL)) + "\n");

		if (m_iDebug)
			OutputDebugString("XMySQL::CheckConnection() mysql_ping: " + CString(mysql_error(m_pMySQL)) + "\n");
		
		
		iConnected = 0;
		if (bReconnect)
		{
			CloseConnection();
			if (OpenConnection())
				iConnected = 2;
		}
	}
	else
		iConnected = 1;

	return iConnected;
}

int XMySQL::CloseConnection()
{
	if (m_pMySQL)
		mysql_close(m_pMySQL);
	else if (m_iDebug)
		OutputDebugString(_T("XMySQL::CloseConnection() m_pMySQL == NULL\n"));

	m_pMySQL  = NULL;
	m_pResult = NULL;
	m_pFields = NULL;
	m_nFields = 0;

	return 0;
}
/*
int XMySQL::QueryWithoutResultDebug(const char *szQuery, bool bException)
{
	int			iBegin = 0, iEnd;
	CStringA	sSubQuery;

	m_sQuery = szQuery;

	try
	{
		while ((iEnd = m_sQuery.Find(';', iBegin)) >= 0)
		{
			iEnd++;
			sSubQuery = m_sQuery.Mid(iBegin, iEnd - iBegin);
			OutputDebugString(CString(sSubQuery) + "\n");
			QueryWithoutResultDebug(sSubQuery, bException);

			iBegin = iEnd;
		}
	}
	catch (XMySQL_Exception e)
	{
		OutputDebugString("* " + CString(sSubQuery) + "\n");
		e;
	}

	return 0;
}
*/
int XMySQL::QueryWithoutResult(const char *szQuery, bool bException)
{
	int		iResult = 1, iQuery;
	double	dDuration;
	CString	sDebug;

	m_sQueryPrev = m_sQuery;
	m_sQuery	 = szQuery;

	if (m_iDebug && m_pResult)
		OutputDebugString(_T("XMySQL::QueryWithoutResult() m_pResult != NULL\n"));

	m_rTimer.StartTimer();
	iQuery = mysql_query(m_pMySQL, szQuery);
	dDuration = m_rTimer.GetElapsedTime();

	if (m_iDebug)
	{
		if (dDuration > 0.5)
		{
			sDebug.Format(_T("XMySQL::QueryWithoutResult() mysql_query %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
		else if (m_iDebug > 1)
		{
			sDebug.Format(_T("XMySQL::QueryWithoutResult() mysql_query %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
	}

	if (iQuery)
	{
		if (bException)
			throw XMySQL_Exception(this, "XMySQL::QueryWithoutResult() mysql_query", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
		else if (m_iDebug)
			OutputDebugString("XMySQL::QueryWithoutResult() mysql_query: " + CString(mysql_error(m_pMySQL)) + ", query: " + CString(m_sQuery) + "\n");

		iResult = 0;
	}
	else
	{
		int iCount = 0;
		while (true)
		{
			MYSQL_RES	*pResult;
			int			iNext;
		
			m_rTimer.StartTimer();
			pResult = mysql_store_result(m_pMySQL);
			dDuration = m_rTimer.GetElapsedTime();

			if (pResult)
			{	// doe iets met het resultaat...
				if (m_iDebug)
				{
					if (dDuration > 0.5)
					{
						sDebug.Format(_T("XMySQL::QueryWithoutResult() mysql_store_result (%d) %.2f ms%s\n"), iCount++, dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
						OutputDebugString(sDebug);
					}
					else if (m_iDebug > 1)
					{
						sDebug.Format(_T("XMySQL::QueryWithoutResult() mysql_store_result (%d) %.2f ms%s\n"), iCount++, dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
						OutputDebugString(sDebug);
					}

					OutputDebugString(_T("XMySQL::QueryWithoutResult() with result(s)...\n"));
				}

				mysql_free_result(pResult);
				m_pResult = NULL;
			}
			else
			{
				if (mysql_field_count(m_pMySQL) == 0)
				{
//					unsigned long long lluAffectedRows = mysql_affected_rows(m_pMySQL);
//					OutputDebugString(_T("mysql_affected rows: %llu\n"), lluAffectedRows);
				}
				else
				{
					if (bException)
						throw XMySQL_Exception(this, "XMySQL::QueryWithoutResult() mysql_store_result", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
					else if (m_iDebug)
						OutputDebugString("XMySQL::QueryWithoutResult() mysql_store_result: " + CString(mysql_error(m_pMySQL)) + "\n");

					break;
				}
			}

			m_rTimer.StartTimer();
			iNext = mysql_next_result(m_pMySQL);
			dDuration = m_rTimer.GetElapsedTime();

			if (m_iDebug)
			{
				if (dDuration > 0.5)
				{
					sDebug.Format(_T("XMySQL::QueryWithoutResult() mysql_next_result %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
					OutputDebugString(sDebug);
				}
				else if (m_iDebug > 1)
				{
					sDebug.Format(_T("XMySQL::QueryWithoutResult() mysql_next_result %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
					OutputDebugString(sDebug);
				}
			}

			if (iNext == 0)
				continue;
			else if (iNext == -1)
				break;
			else
			{
				if (bException)
					throw XMySQL_Exception(this, "XMySQL::QueryWithoutResult() mysql_next_result", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
				else if (m_iDebug)
					OutputDebugString("XMySQL::QueryWithoutResult() mysql_next_result: " + CString(mysql_error(m_pMySQL)) + ", query: " + CString(m_sQuery) + "\n");

				iResult = 0;
				break;
			}
		}
	}

	return iResult;
}

int XMySQL::QueryWithoutResult(const WCHAR *szQuery, bool bException)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szQuery, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szQuery, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return QueryWithoutResult(utf8, bException);
}

int XMySQL::Query(const char *szQuery, bool bException)
{
	int		iResult = 1, iQuery;
	double	dDuration;
	CString	sDebug;

	m_sQueryPrev = m_sQuery;
	m_sQuery	 = szQuery;

	if (m_iDebug && m_pResult)
		OutputDebugString(_T("XMySQL::Query() m_pResult != NULL\n"));

	m_rTimer.StartTimer();
	iQuery = mysql_query(m_pMySQL, szQuery);
	dDuration = m_rTimer.GetElapsedTime();

	if (m_iDebug)
	{
		if (dDuration > 0.5)
		{
			sDebug.Format(_T("XMySQL::Query() mysql_query %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
		else if (m_iDebug > 1)
		{
			sDebug.Format(_T("XMySQL::Query() mysql_query %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
	}

	if (iQuery)
	{
		if (bException)
			throw XMySQL_Exception(this, "XMySQL::Query() mysql_query", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
		else if (m_iDebug)
			OutputDebugString("XMySQL::Query() mysql_query: " + CString(mysql_error(m_pMySQL)) + ", query: " + CString(m_sQuery) + "\n");

		iResult = 0;
	}
	else
	{
		m_rTimer.StartTimer();
		m_pResult = mysql_store_result(m_pMySQL);
		dDuration = m_rTimer.GetElapsedTime();

		if (m_iDebug)
		{
			if (dDuration > 0.5)
			{
				sDebug.Format(_T("XMySQL::Query() mysql_store_result %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
				OutputDebugString(sDebug);
			}
			else if (m_iDebug > 1)
			{
				sDebug.Format(_T("XMySQL::Query() mysql_store_result %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
				OutputDebugString(sDebug);
			}
		}

		if (m_pResult)
		{
			m_pFields = mysql_fetch_fields(m_pResult);
			m_nFields = mysql_num_fields(m_pResult);

			m_nRows	  = mysql_num_rows(m_pResult);
		}
		else if (mysql_errno(m_pMySQL))
		{
			if (bException)
				throw XMySQL_Exception(this, "XMySQL::Query() mysql_store_result", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
			else if (m_iDebug)
				OutputDebugString("XMySQL::Query() mysql_store_result: " + CString(mysql_error(m_pMySQL)) + ", query: " + CString(m_sQuery) + "\n");

			iResult = 0;
		}
	}

	return iResult;
}

int XMySQL::Query(const WCHAR *szQuery, bool bException)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szQuery, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szQuery, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return Query(utf8, bException);	
}

int XMySQL::StoredProcedureWithOneResultSet(const WCHAR *szQuery, bool bException)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szQuery, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szQuery, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return StoredProcedureWithOneResultSet(utf8, bException);	
}

int XMySQL::StoredProcedureWithOneResultSet(const char *szQuery, bool bException)
{	
	int		iResult = 1, iQuery;
	double	dDuration;
	CString	sDebug;

	m_sQueryPrev = m_sQuery;
	m_sQuery	 = szQuery;

	if (m_iDebug && m_pResult)
		OutputDebugString(_T("XMySQL::StoredProcedureWithOneResultSet() m_pResult != NULL"));

	m_rTimer.StartTimer();
	iQuery = mysql_query(m_pMySQL, szQuery);
	dDuration = m_rTimer.GetElapsedTime();

	if (m_iDebug)
	{
		if (dDuration > 0.5)
		{
			sDebug.Format(_T("XMySQL::StoredProcedureWithOneResultSet() mysql_query %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
		else if (m_iDebug > 1)
		{
			sDebug.Format(_T("XMySQL::StoredProcedureWithOneResultSet() mysql_query %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
			OutputDebugString(sDebug);
		}
	}

	if (iQuery)
	{
		if (bException)
			throw XMySQL_Exception(this, "XMySQL::StoredProcedureWithOneResultSet() mysql_query", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
		else if (m_iDebug)
			OutputDebugString("XMySQL::StoredProcedureWithOneResultSet() mysql_query: " + CString(mysql_error(m_pMySQL)) + ", query: " + CString(m_sQuery) + "\n");

		iResult = 0;
	}
	else
	{
		int status = 0;		
		
		m_rTimer.StartTimer();
		m_pResult = mysql_store_result(m_pMySQL);
		dDuration = m_rTimer.GetElapsedTime();

		if (m_iDebug)
		{
			if (dDuration > 0.5)
			{
				sDebug.Format(_T("XMySQL::StoredProcedureWithOneResultSet() mysql_store_result %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
				OutputDebugString(sDebug);
			}
			else if (m_iDebug > 1)
			{
				sDebug.Format(_T("XMySQL::StoredProcedureWithOneResultSet() mysql_store_result %.2f ms%s\n"), dDuration * 1000.0, (dDuration > 0.5)?_T(" (duurt lang...)"):_T(""));
				OutputDebugString(sDebug);
			}
		}

		if (m_pResult)
		{
			m_pFields = mysql_fetch_fields(m_pResult);
			m_nFields = mysql_num_fields(m_pResult);
			m_nRows	  = mysql_num_rows(m_pResult);			
		}
		else if (mysql_errno(m_pMySQL))
		{
			if (bException)
				throw XMySQL_Exception(this, "XMySQL::StoredProcedureWithOneResultSet() mysql_store_result", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
			else if (m_iDebug)
				OutputDebugString("XMySQL::StoredProcedureWithOneResultSet() mysql_store_result: " + CString(mysql_error(m_pMySQL)) + ", query: " + CString(m_sQuery) + "\n");

			iResult = 0;
		}
			
		//call mysql_next_result because a call to a stored procedure can return more than one resultset which has to be processed by mysql_next_result
		while (mysql_next_result(m_pMySQL) > 0)
		{
			OutputDebugString(_T("Could not execute statement"));
		}
		
	}	
	return iResult;
}

int XMySQL::GetFields()
{
	return m_nFields;
}

unsigned long long XMySQL::GetRows()
{
	return m_nRows;
}

int XMySQL::GetField(const int iField, char *&szValue)
{
	int iRet;

	if ((iField >= 0) && (iField < m_nFields))
	{
		szValue	= m_pFields[iField].name;
		iRet	= 1;
	}
	else
	{
		szValue	= NULL;
		iRet	= 0;
	}

	return iRet;
}

int XMySQL::NextRow(bool bException)
{
	if (m_pResult)
	{
		m_sqlRow = mysql_fetch_row(m_pResult);

		if (mysql_errno(m_pMySQL))
		{
			if (bException)
				throw XMySQL_Exception(this, "XMySQL::NextRow() mysql_fetch_row", mysql_errno(m_pMySQL), mysql_error(m_pMySQL), mysql_sqlstate(m_pMySQL), __LINE__);
			else if (m_iDebug)
				OutputDebugString("XMySQL::NextRow() mysql_fetch_row: " + CString(mysql_error(m_pMySQL)) + "\n");

			m_sqlRow = NULL;
		}
	}
	else
		m_sqlRow = NULL;

	return (m_sqlRow != NULL);
}

int XMySQL::GetFieldValue(const int iField, char *&szValue)
{
	int iRet;

	if ((iField >= 0) && (iField < m_nFields))
	{
		szValue	= m_sqlRow[iField];
		iRet	= 1;
	}
	else
	{
		szValue	= NULL;
		iRet	= 0;
	}

	return iRet;
}

int XMySQL::GetFieldValue(const int iField, CString &sValue)
{
	char	*szValue = NULL;
	int		i = GetFieldValue(iField, szValue);

	if (i && szValue)
		sValue = szValue;

	return i;
}

int XMySQL::GetFieldValue(const char *szField, char *&szValue)
{
	int i;

	szValue = NULL;
	for (i = 0; i < m_nFields; i++)
	{
		if (m_pFields && (strcmp(szField, m_pFields[i].name) == 0))
		{
			szValue = m_sqlRow[i];
			break;
		}
	}

	return (i == m_nFields)?0:1;
}

int XMySQL::GetFieldValue(const char *szField, double &dValue)
{
	char	*szValue = NULL;
	int		i = GetFieldValue(szField, szValue);

	if (i && szValue)
		dValue = atof(szValue);

	return i;
}

int XMySQL::GetFieldValue(const WCHAR *szField, double &dValue)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szField, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szField, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return GetFieldValue(utf8, dValue);	
}

int XMySQL::GetFieldValue(const char *szField, int &iValue)
{
	char	*szValue = NULL;
	int		i = GetFieldValue(szField, szValue);

	if (i && szValue)
		iValue = atoi(szValue);

	return i;
}

int XMySQL::GetFieldValue(const WCHAR *szField, int &iValue)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szField, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szField, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return GetFieldValue(utf8, iValue);	
}

int XMySQL::GetFieldValue(const char *szField, CString &sValue)
{
	int i;
	for (i = 0; i < m_nFields; i++)
	{
		if (m_pFields && (strcmp(szField, m_pFields[i].name) == 0))
		{
			LPSTR p = m_sqlRow[i];
			int n = MultiByteToWideChar(CP_UTF8, 0, p, -1, NULL, 0);
			CString result;
			LPWSTR w = result.GetBuffer(n);
			MultiByteToWideChar(CP_UTF8, 0, p, -1, w, n);
			result.ReleaseBuffer();
			sValue = result;
			break;
		}
	}
	return (i == m_nFields)? 0 : 1;
}

int XMySQL::GetFieldValue(const WCHAR *szField, CString &sValue)
{
	CStringA utf8;
	int	l = WideCharToMultiByte(CP_UTF8, 0, szField, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szField, -1, utf8.GetBuffer(l), l, NULL, NULL);
	return GetFieldValue(utf8, sValue);	
}

int XMySQL::DumpResult(XMySQL_Result *&pResult)
{
	int		nFields	= 0;
	int		nRows	= 0;
	char	*pBase, *pOffset;
	int		*pInteger, *pRow;
	int		iSize = 0;
	
	iSize += sizeof(int);	// int iSize
	iSize += sizeof(int);	// int nFields
	iSize += sizeof(int);	// int nRows
	iSize += sizeof(int);	// int ppFields, offset to offset(s) to szFieldName
	iSize += sizeof(int);	// int pppData, offset to offset(s) to offset(s) to datum/data

	for (int j = 0; j < m_nFields; j++)
	{
		iSize += sizeof(int);
		iSize += strlen(m_pFields[j].name) + 1;
	}

	mysql_data_seek(m_pResult, 0);
	for (int i = 0; i < m_nRows; i++)
	{
		iSize += sizeof(int);

		MYSQL_ROW		sqlRow	= mysql_fetch_row(m_pResult);
		unsigned long	*l		= mysql_fetch_lengths(m_pResult);

		for (int j = 0; j < m_nFields; j++)
		{
			iSize += sizeof(int);
			iSize += l[j] + 1;
		}
	}

	pResult = (XMySQL_Result *)(pBase = pOffset = new char[iSize]);
	memset(pBase, 0, iSize);

	pResult->iSize		= iSize;
	pResult->nFields	= m_nFields;
	pResult->nRows		= (int)m_nRows;
	pResult->pFields	= -1;
	pResult->pRows		= -1;
	pOffset				+= sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int);

	pResult->pFields	= (int)(pOffset - pBase);

	pInteger = (int *)pOffset;
	pOffset += m_nFields * sizeof(int);
	for (int j = 0; j < m_nFields; j++, pInteger++)
	{
		*pInteger = (int)(pOffset - pBase);
		memcpy(pOffset, m_pFields[j].name, strlen(m_pFields[j].name));
		pOffset  += strlen(m_pFields[j].name) + 1;
	}

	pResult->pRows = (int)(pOffset - pBase);

	pRow	 = (int *)pOffset;
	pOffset	+= m_nRows * sizeof(int);

	mysql_data_seek(m_pResult, 0);
	for (int i = 0; i < m_nRows; i++, pRow++)
	{
		*pRow = (int)(pOffset - pBase);

		MYSQL_ROW		sqlRow	= mysql_fetch_row(m_pResult);
		unsigned long	*l		= mysql_fetch_lengths(m_pResult);

		pInteger = (int *)pOffset;
		pOffset += m_nFields * sizeof(int);
		for (int j = 0; j < m_nFields; j++, pInteger++)
		{
			*pInteger = (int)(pOffset - pBase);
			memcpy(pOffset, sqlRow[j], l[j]);
			pOffset += l[j] + 1;
		}
	}

	return 0;
}

int XMySQL::CloseResult()
{
	while (NextRow(false));

	if (m_pResult)
	{
		mysql_free_result(m_pResult);
		m_pResult = NULL;
		m_pFields = NULL;
		m_nFields = 0;
	}

	return 0;
}