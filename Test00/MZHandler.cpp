#include "stdafx.h"
#include "MZ.h"
#include "MZHandler.h"
#include "xmysql.h"
#include <iostream>


MZHandler::MZHandler()
{
	m_bConnectedToMySQL = false;
	m_bVerbose = true;
	m_iGenerateNMZ = 100;
}

MZHandler::~MZHandler()
{
}

int MZHandler::Go(bool bGenerateNew)
{
	MZ* mz;

	if (!bGenerateNew)
	{
		mz = new MZ();
		CString mzString = _T("6bac9aa259cd5516361c9a67");
		if (!mz->setMZ(4, 6, 7, 0, mzString))
			return 0;

		mz->printMZString();
		mz->printVisual(true);
		mz->printIntersections();
		mz->printRoutesAndStats();
		delete mz;
	}
	else
	{
		XMySQL*	m_pMySQL = new XMySQL();
		CString sConn = (_T("Server=127.0.0.1;Port=3306;User=root;PWD=zeelandbrug;connecttimeout=5;readtimeout=20;writetimeout=20;charset=utf8"));
		m_pMySQL->SetConnectionString(sConn);
		if (m_bConnectedToMySQL = (m_pMySQL->OpenConnection() != 0))
		{
			OutputDebugString(_T("connected to MySQL\n"));
		}
		else
		{
			OutputDebugString(_T("not connected to MySQL\n"));
		}

		CString sMessage;
		int nInserted = 0;
		for (int iteration = 0; iteration < m_iGenerateNMZ; iteration++)
		{
			mz = new MZ();
			mz->generateMZ(4, 6, m_bVerbose);
			//mz->printMZString();
			//mz->printVisual(true);
			//mz->printIntersections();
			//mz->printRoutesAndStats();

			if (m_bConnectedToMySQL)
			{
				if (m_bVerbose)
				{
					sMessage.Format(_T("\niteration: %i / %i\n"), iteration, m_iGenerateNMZ);
					OutputDebugString(sMessage);
				}
				std::cout << "iteration: " << iteration << " / " << m_iGenerateNMZ << " coverage: " << (int) mz->getRouteCoverage() << " nInserted: " << nInserted << "\n";
				if (mz->getMinIntersections() > 0)
				{
					if (mz->getRouteCoverage() > 33)
					{
						CString insertStr;
						insertStr.Format(_T("insert into zmz.mz (datetimeid, mzstring, width, height, the_entrance, the_exit, route_coverage, row_utilization, min_intersections, opposing_steps) \
											values (now(), \'%s\', %i, %i, %i, %i, %2.2f, %2.2f, %i, %i);"),
											mz->getMZString(),
											mz->getWidth(),
											mz->getHeight(),
											mz->getEntrance(),
											mz->getExit(),
											mz->getRouteCoverage(),
											mz->getRowUtilization(),
											mz->getMinIntersections(),
											mz->getOpposingSteps());

						try
						{
							m_pMySQL->QueryWithoutResult(insertStr);
							nInserted++;
						}
						catch (XMySQL_Exception &e)
						{
							sMessage.Format(_T("MySQL Error %d (%s): %s\n"), e.GetErrNo(), CString(e.GetSqlState()), CString(e.GetError()));
							OutputDebugString(sMessage);
							sMessage.Format(_T("The query that gives the error: %s"), insertStr);
							OutputDebugString(sMessage);
						}
					}
				}
				else
				{
					if (m_bVerbose)
						OutputDebugString(_T("Could not store the generated MZ, because min_intersections was 0\n"));
				}
			}
			else
			{
				OutputDebugString(_T("Could not store MZ to MySQL ...\n"));
			}
			delete mz;
		}
		sMessage.Format(_T("\nInserted MZ's: %i\n"), nInserted);
		OutputDebugString(sMessage);
		std::cout << "nInserted " << nInserted << "\n";
		//printf("%i\n", nInserted);
	}
	return 1;
}