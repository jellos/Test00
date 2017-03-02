#include "stdafx.h"
#include "MZ.h"
#include <iostream>

#define BIG_NUMBER 100

MZ::MZ()
{
	m_width = 0;
	m_widthVisual = 0;
	m_height = 0;
	m_heightVisual = 0;
	m_totalCells = 0;
}

MZ::~MZ()
{
}

bool MZ::setMZ(int width, int height, int entrance, int exit, const CString &s)
{
	m_width = width;
	m_height = height;
	m_entrance = entrance;
	m_exit = exit;
	m_totalCells = width * height;
	m_widthVisual = (width * 2) + 1;
	m_heightVisual = height + 1;
	mzString = s;
	m_bAllCellsReachable = false;

	//shiftDown(13);

	if (!checkMZ())
		return false;

	initializeCLMatrix();			// fills m_CLMatrix

	if (!checkStuff())
		return false;

	updateVectors();
	calculateMazeAndRouteStats();

	return true;
}

void MZ::shiftDown(int offset)
{
	offset = (offset % m_height);
	CString lastRow = mzString.Mid(m_totalCells - (offset * m_width), (offset * m_width));
	mzString.Delete(m_totalCells - (offset * m_width), (offset * m_width));
	lastRow.Append(mzString);
	mzString = lastRow;
	m_entrance = (m_entrance + (offset * m_width)) % m_totalCells;
	m_exit = (m_exit + (offset * m_width)) % m_totalCells;
}

void MZ::generateMZ(int width, int height, bool bVerbose)
{
	CString sDebug;
	m_width = width;
	m_height = height;
	m_totalCells = width * height;
	m_widthVisual = (width * 2) + 1;
	m_heightVisual = height + 1;
	m_nWalls = 0;

	for (int i = 0; i < m_totalCells; i++)
		mzString.Append(_T("0"));

	m_bAllCellsReachable = false;

	initializeCLMatrix();
	addRandomEntrance();
	addRandomExit();
	updateVectors(bVerbose);

	m_nTries = m_totalCells / 2;
	m_nMaxWalls = 1;
	bool suc, removed;
	int cell, side;

	for (int i = 0; i < m_nTries; i++)
	{
		removed = false;
		suc = addRandomWall(cell, side);
		if (!m_bAllCellsReachable)
		{
			removeWallAt(cell, side);
			removed = true;
		}
		if (bVerbose)
		{
			sDebug.Format(_T("m_nMaxWalls = %i addRandomWall (cell %i side %i) succeeded? %i removed? %i\n"), m_nMaxWalls, cell, side, suc, removed);
			OutputDebugString(sDebug);
		}
	}

	m_nMaxWalls = 2;

	for (int i = 0; i < m_nTries; i++)
	{
		removed = false;
		suc = addRandomWall(cell, side);
		if (!m_bAllCellsReachable)
		{
			removeWallAt(cell, side);
			removed = true;
		}
		if (bVerbose)
		{
			sDebug.Format(_T("m_nMaxWalls = %i addRandomWall (cell %i side %i) succeeded? %i removed? %i\n"), m_nMaxWalls, cell, side, suc, removed);
			OutputDebugString(sDebug);
		}
	}

	m_nMaxWalls = 3;

	for (int i = 0; i < m_nTries; i++)
	{
		removed = false;
		suc = addRandomWall(cell, side);
		if (!m_bAllCellsReachable)
		{
			removeWallAt(cell, side);
			removed = true;
		}
		if (bVerbose)
		{
			sDebug.Format(_T("m_nMaxWalls = %i addRandomWall (cell %i side %i) succeeded? %i removed? %i\n"), m_nMaxWalls, cell, side, suc, removed);
			OutputDebugString(sDebug);
		}
	}

	if (m_minIntersections == 0)
		fillWalllessCells(bVerbose);

	calculateMazeAndRouteStats();
}

void MZ::calculateMazeAndRouteStats()
{
	m_iRowsUtilized = 0;
	m_nOpposingSteps = 0;

	m_vRowHistogram.clear();
	
	for (int i = 0; i < m_height; i++)
		m_vRowHistogram.push_back(0);
	
	for (int i = 0; i < (int)m_vBestRoute.size(); i++)
		m_vRowHistogram.at(getRowNumber(m_vBestRoute.at(i)))++;
	
	for (int i = 0; i < (int)m_vRowHistogram.size(); i++)
		m_iRowsUtilized += (m_vRowHistogram.at(i) > 0 ? 1 : 0);

	int previousCell;
	for (int i = 1; i < (int)m_vBestRoute.size(); i++)
	{
		previousCell = m_vBestRoute.at(i - 1);
		m_nOpposingSteps += (isLeftOf(previousCell, m_vBestRoute.at(i)) ? 1 : 0);
	}

	m_fBestRouteCoverage = ((float) m_vBestRoute.size() / (float) m_totalCells) * 100.f;
	m_fBestRouteRowsUtilized = ((float) m_iRowsUtilized / (float) m_height) * 100.f;
}

void MZ::addRandomEntrance()
{
	int r = rand() % m_height;
	r *= m_width;
	r += m_width - 1;
	m_entrance = r;

	for (int i = m_width - 1; i < m_totalCells; i += m_width)
	{
		if (i != r)
		{
			m_CLMatrix.at(i).addWallAtSide(1);
			m_nWalls++;
		}
	}
}

void MZ::addRandomExit()
{
	int r = rand() % m_height;
	r *= m_width;
	m_exit = r;
	
	for (int i = 0; i < m_totalCells; i += m_width)
	{
		if (i != r)
		{
			m_CLMatrix.at(i).addWallAtSide(3);
			m_nWalls++;
		}
	}
}

void MZ::fillWalllessCells(bool bVerbose)
{
	CString sDebug;
	
	for (int i = 0; i < (int) m_vIntersectionProblems.size(); i++)
	{
		m_CLMatrix.at(m_vIntersectionProblems.at(i)).addWallAtSide(1);
		m_CLMatrix.at(m_vIntersectionProblems.at(i) + 1).addWallAtSide(3);
		if (bVerbose)
		{
			sDebug.Format(_T("\nFixing wallless cell at %i\n"), m_vIntersectionProblems.at(i));
			OutputDebugString(sDebug);
		}
	}
	updateVectors();
}

void MZ::removeWallAt(int cell, int side)
{
	int neighBour;
	int mirroredWall = (side + 2) % 4;
	
	if (side == 0)
	{
		neighBour = (cell - m_width + m_totalCells) % m_totalCells;
	}
	else if ((cell + 1) % m_width != 0 && side == 1)
	{
		neighBour = cell + 1;
	}
	else if (side == 2)
	{
		neighBour = (cell + m_width) % m_totalCells;
	}
	else if (cell % m_width != 0 && side == 3)
	{
		neighBour = cell - 1;
	}
	m_CLMatrix.at(cell).removeWallAt(side);
	m_CLMatrix.at(neighBour).removeWallAt(mirroredWall);
	m_nWalls--;
	updateVectors();
}

bool MZ::addRandomWall(int &cell, int &side)
{
	bool succeeded = false;
	int r = rand() % m_totalCells;
	cell = r;
	int addedWall = -1;
	int neighBour = -1;
	int nWalls = m_CLMatrix.at(r).getNumberOfWalls();
	int nWallsNeighbour;

	if (nWalls < m_nMaxWalls)
	{
		addedWall = m_CLMatrix.at(r).addRandomWall();
		side = addedWall;
		if ((isRightEdge(r) && addedWall == 1) || (isLeftEdge(r) && addedWall == 3))
		{
			m_CLMatrix.at(r).removeWallAt(addedWall);
			return false;
		}		
		m_nWalls++;
	}
		
	if (addedWall == 0)
	{
		neighBour = (r - m_width + m_totalCells) % m_totalCells;
	}
	else if ((r + 1) % m_width != 0 && addedWall == 1)
	{
		neighBour = r + 1;
	}
	else if (addedWall == 2)
	{
		neighBour = (r + m_width) % m_totalCells;
	}
	else if (r % m_width != 0 && addedWall == 3)
	{
		neighBour = r - 1;
	}

	if (neighBour >= 0)
	{
		succeeded = true;
		int mirroredWall = (addedWall + 2) % 4;
		m_CLMatrix.at(neighBour).addWallAtSide(mirroredWall);
		nWallsNeighbour = m_CLMatrix.at(neighBour).getNumberOfWalls();
		if (nWallsNeighbour > m_nMaxWalls)
		{
			m_CLMatrix.at(r).removeWallAt(addedWall);
			m_CLMatrix.at(neighBour).removeWallAt(mirroredWall);
			m_nWalls--;
			succeeded = false;
		}
	}
	side = addedWall;
	updateVectors();
	return succeeded;
}

void MZ::updateVectors(bool bVerbose)
{
	updateVisual();
	updateIntersections();
	updateWallCount();

	if (bVerbose)
		printVisual();
	
	updateRoutes();
	if (bVerbose)
		printRoutesAndStats();

	updateMZString();
	if (bVerbose)
		printMZString();
}

void MZ::updateMZString()
{
	mzString.Empty();
	for (int i = 0; i < m_totalCells; i++)
		mzString += m_CLMatrix.at(i).getHumanReadableChar();
}

void MZ::updateRoutes()
{
	int nRoutes;
	int currentCell = m_entrance;
	bool bBestRouteFound = false;
	m_nVisited = 0;
	m_vRoutes.clear();
	m_vVisited.clear();
	m_vBestRoute.clear();

	for (int i = 0; i < m_totalCells; i++)
		m_vVisited.push_back(i == m_entrance);

	std::vector<int> start, tRoute, rRoute, bRoute, lRoute;

	start.push_back(m_entrance);
	m_vRoutes.push_back(start);

	while (true)
	{
		nRoutes = m_vRoutes.size();
		std::vector<int> bErase;
		for (int i = 0; i < nRoutes; i++)
		{
			bool bEr = false;
			tRoute = m_vRoutes.at(i);
			rRoute = m_vRoutes.at(i);
			bRoute = m_vRoutes.at(i);
			lRoute = m_vRoutes.at(i);

			currentCell = m_vRoutes.at(i).back();

			if (!m_CLMatrix.at(currentCell).hasTopWall())
			{
				int nbTop = neighBourTop(currentCell);
				if (!m_vVisited.at(nbTop))
				{
					tRoute.push_back(nbTop);
					m_vRoutes.push_back(tRoute);
					m_vVisited.at(nbTop) = true;
					if (!bEr)
					{
						bErase.push_back(i);
						bEr = true;
					}
					if (!bBestRouteFound && nbTop == m_exit)
					{
						bBestRouteFound = true;
						m_vBestRoute = tRoute;
					}
				}
			}
			if (!m_CLMatrix.at(currentCell).hasRightWall() && currentCell != m_entrance)
			{
				int nbRight = neighBourRight(currentCell);
				if (!m_vVisited.at(nbRight))
				{
					rRoute.push_back(nbRight);
					m_vRoutes.push_back(rRoute);
					m_vVisited.at(nbRight) = true;
					if (!bEr)
					{
						bErase.push_back(i);
						bEr = true;
					}
					if (!bBestRouteFound && nbRight == m_exit)
					{
						bBestRouteFound = true;
						m_vBestRoute = rRoute;
					}
				}
			}
			if (!m_CLMatrix.at(currentCell).hasBottomWall())
			{
				int nbBottom = neighBourBottom(currentCell);
				if (!m_vVisited.at(nbBottom))
				{
					bRoute.push_back(nbBottom);
					m_vRoutes.push_back(bRoute);
					m_vVisited.at(nbBottom) = true;
					if (!bEr)
					{
						bErase.push_back(i);
						bEr = true;
					}
					if (!bBestRouteFound && nbBottom == m_exit)
					{
						bBestRouteFound = true;
						m_vBestRoute = bRoute;
					}
				}
			}
			if (!m_CLMatrix.at(currentCell).hasLeftWall() && currentCell != m_exit)
			{
				int nbLeft = neighBourLeft(currentCell);
				if (!m_vVisited.at(nbLeft))
				{
					lRoute.push_back(nbLeft);
					m_vRoutes.push_back(lRoute);
					m_vVisited.at(nbLeft) = true;
					if (!bEr)
					{
						bErase.push_back(i);
						bEr = true;
					}
					if (!bBestRouteFound && nbLeft == m_exit)
					{
						bBestRouteFound = true;
						m_vBestRoute = lRoute;
					}
				}
			}			
		}
		for (int e = (int) bErase.size() - 1; e >= 0; e--)
		{
			m_vRoutes.erase(m_vRoutes.begin() + bErase.at(e));
		}
		if (bErase.size() == 0)
			break;
	}

	for (int i = 0; i < m_totalCells; i++)
		m_nVisited += m_vVisited.at(i) ? 1 : 0;

	m_bAllCellsReachable = m_nVisited == m_totalCells;
}

void MZ::initializeCLMatrix()
{
	m_CLMatrix.clear();
	for (int i = 0; i < m_totalCells; i++)
	{
		CL* cell = new CL((const char)mzString.GetAt(i));
		m_CLMatrix.push_back(*cell);
		delete cell;
	}
}

bool MZ::checkStuff()
{
	if (!checkEdgeCompatibility())
	{
		OutputDebugString(_T("MZ edge compatibility error(s)! exiting ...\n"));
		return false;
	}

	/*
	if (!checkOneEntrance())
	{
		OutputDebugString(_T("There is not exactly one entrance! exiting ...\n"));
		return false;
	}

	if (!checkOneExit())
	{
		OutputDebugString(_T("There is not exactly one exit! exiting ...\n"));
		return false;
	}
	*/
	return true;
}

void MZ::updateWallCount()
{
	int count;
	m_CLWallCount.clear();
	for (int i = 0; i < m_totalCells; i++)
	{
		count = 0;
		count += m_CLMatrix.at(i).hasTopWall() ? 1 : 0;
		count += m_CLMatrix.at(i).hasRightWall() ? 1 : 0;
		count += m_CLMatrix.at(i).hasBottomWall() ? 1 : 0;
		count += m_CLMatrix.at(i).hasLeftWall() ? 1 : 0;
		m_CLWallCount.push_back(count);
	}
}

void MZ::updateVisual()
{
	int visual[BIG_NUMBER][BIG_NUMBER] = {0};
	m_CLMatrixVisual.clear();

	for (int i = 0; i < m_totalCells; i++)
	{
		int x_odd = (2 * i + 1) % (m_width * 2);
		int x_even = (2 * i) % (m_width * 2);
		int y = i / m_width;
		if (m_CLMatrix.at(i).hasTopWall())
			visual[x_odd][y] = 1;
		
		if (isBottomRow(i) && m_CLMatrix.at(i).hasBottomWall())
			visual[x_odd][y + 1] = 1;
		
		if (m_CLMatrix.at(i).hasLeftWall())
			visual[x_even][y + 1] = 1;
		
		if (isRightEdge(i) && m_CLMatrix.at(i).hasRightWall())
			visual[x_even + 2][y + 1] = 1;
	}

	for (int y = 0; y < m_heightVisual; y++)
	{
		for (int x = 0; x < m_widthVisual; x++)
		{
			bool b = visual[x][y] == 1 ? true : false;
			m_CLMatrixVisual.push_back(b);
		}
	}
}

void MZ::updateIntersections()
{
	CString sDebug;
	m_CLIntersections.clear();
	int nIntersections;
	int visualIndex;
	int totalCells = m_heightVisual * m_widthVisual;
	int leftNei, rightNei, topNei, bottomNei;

	m_minIntersections = 5;

	for (int y = 0; y < m_height; y++)
	{
		for (int x = 0; x < m_width + 1; x++)
		{
			nIntersections = 0;
			visualIndex = 2 * x + (y * m_widthVisual);
			leftNei = visualIndex - 1;
			rightNei = visualIndex + 1;
			topNei = (visualIndex - m_widthVisual + totalCells) % totalCells;
			bottomNei = (visualIndex + m_widthVisual) % totalCells;

			if (x != 0) //x is no leftmost column
				nIntersections += m_CLMatrixVisual.at(leftNei) ? 1 : 0;
			if (x != m_width) //x is no rightmost column
				nIntersections += m_CLMatrixVisual.at(rightNei) ? 1 : 0;

			nIntersections += m_CLMatrixVisual.at(bottomNei) ? 1 : 0;
			if (y == 0) //y is top row
				nIntersections += m_CLMatrixVisual.at(topNei) ? 1 : 0;
			nIntersections += m_CLMatrixVisual.at(visualIndex) ? 1 : 0;
			
			m_CLIntersections.push_back(nIntersections);
			m_minIntersections = min(m_minIntersections, nIntersections);
		}
	}

	m_vIntersectionProblems.clear();
	int cellID;
	for (int i = 0; i < (int)m_CLIntersections.size(); i++)
	{		
		if (m_CLIntersections.at(i) == 0)
		{
			cellID = ((i / (m_width + 1)) - 1) * m_width + ((i % (m_width + 1)) - 1);
			if (cellID >= 0)
			{
				//sDebug.Format(_T("\nintersectionproblem i = %i cellID = %i\n"), i, cellID);
				//OutputDebugString(sDebug);
				m_vIntersectionProblems.push_back(cellID);
			}
		}
	}
}

void MZ::printWallCount()
{
	CString outPut;
	OutputDebugString(_T("\nWall Count:\n"));
	for (int i = 0; i < m_totalCells; i++)
	{
		outPut.AppendFormat(_T("%i"), m_CLWallCount.at(i));
		if (i % m_width == (m_width - 1))
			outPut.Append(_T("\n"));
	}
	OutputDebugString(outPut);
}

void MZ::printIntersections()
{
	CString outPut;
	OutputDebugString(_T("\nIntersections:\n"));
	for (int y = 0; y < m_height; y++)
	{
		outPut.Empty();
		for (int x = 0; x < m_width + 1; x++)
		{
			outPut.AppendFormat(_T("%i"), m_CLIntersections.at(x + y * (m_width + 1)));
		}
		outPut.Append(_T("\n"));
		OutputDebugString(outPut);
	}
}

void MZ::printVisual(bool doubly)
{
	CString outPut;
	/*
	for (int y = 0; y < m_heightVisual; y++)
	{
		outPut.Empty();
		for (int x = 0; x < m_widthVisual; x++)
		{
			outPut.Append(m_CLMatrixVisual.at(y * m_widthVisual + x) ? _T("1") : _T("0"));
		}
		outPut.Append(_T("\n"));
		OutputDebugString(outPut);
	}
	*/
	CString currChar;
	for (int y = 0; y < m_heightVisual; y++)
	{
		outPut.Empty();
		for (int x = 0; x < m_widthVisual; x++)
		{
			currChar = x % 2 == 1 ? _T("_") : _T("|");
			outPut.Append(m_CLMatrixVisual.at(y * m_widthVisual + x) ? currChar : _T(" "));
		}
		outPut.Append(_T("\n"));
		OutputDebugString(outPut);
	}
	if (doubly)
	{
		for (int y = 1; y < m_heightVisual; y++)
		{
			outPut.Empty();
			for (int x = 0; x < m_widthVisual; x++)
			{
				currChar = x % 2 == 1 ? _T("_") : _T("|");
				outPut.Append(m_CLMatrixVisual.at(y * m_widthVisual + x) ? currChar : _T(" "));
			}
			outPut.Append(_T("\n"));
			OutputDebugString(outPut);
		}
	}
}

void MZ::printRoutesAndStats()
{
	CString outPut;
	OutputDebugString(_T("\nRoutes:\n"));
	for (int i = 0; i < (int) m_vRoutes.size(); i++)
	{
		for (int j = 0; j < (int) m_vRoutes.at(i).size(); j++)
		{
			outPut.Format(_T("%i "), m_vRoutes.at(i).at(j));
			OutputDebugString(outPut);
		}
		outPut.Format(_T("\n"));
		OutputDebugString(outPut);
	}
	outPut.Format(_T("\nm_entrance: %i\t\t\tm_exit: %i\nm_nVisited: %i/%i\t\tm_bAllCellsReachable: %i\t\tm_nWalls: %i\t\tm_minIntersections: %i\n"), 
		m_entrance, m_exit, m_nVisited, m_totalCells, m_bAllCellsReachable, m_nWalls, m_minIntersections);
	OutputDebugString(outPut);

	outPut.Format(_T("\nBest Route:\n"));
	OutputDebugString(outPut);
	for (int j = 0; j < (int) m_vBestRoute.size(); j++)
	{
		outPut.Format(_T("%i "), m_vBestRoute.at(j));
		OutputDebugString(outPut);
	}
	outPut.Format(_T("\n"));
	OutputDebugString(outPut);

	outPut.Format(_T("Best Route Length: %i / %i = %2.2f %%\n"), m_vBestRoute.size(), m_totalCells, m_fBestRouteCoverage);
	OutputDebugString(outPut);
	
	//outPut.Format(_T("Route Histogram:\n"));
	//OutputDebugString(outPut);
	//for (int i = 0; i < (int)m_vRowHistogram.size(); i++)
	//{
	//	outPut.Format(_T("%i "), m_vRowHistogram.at(i));
	//	OutputDebugString(outPut);
	//}
	//OutputDebugString(_T("\n"));
		
	outPut.Format(_T("Rows utilized: %i / %i = %2.2f %%\n"), m_iRowsUtilized, m_height, m_fBestRouteRowsUtilized);
	OutputDebugString(outPut);

	outPut.Format(_T("m_nOpposingSteps: %i\n"), m_nOpposingSteps);
	OutputDebugString(outPut);

	if (m_vIntersectionProblems.size() > 0)
	{
		OutputDebugString(_T("Intersection problems at: \n"));
		for (int i = 0; i < (int)m_vIntersectionProblems.size(); i++)
		{
			outPut.Format(_T("%i "), m_vIntersectionProblems.at(i));
			OutputDebugString(outPut);
		}
		OutputDebugString(_T("\n"));
	}
	else
		OutputDebugString(_T("No Intersection problems at all\n"));
}

bool MZ::checkOneEntrance()
{
	int nEntrances = 0;
	for (int i = m_width - 1; i < m_totalCells; i += m_width)
	{
		nEntrances += (m_CLMatrix.at(i).hasRightWall() ? 0 : 1);
	}
	return nEntrances == 1;
}

bool MZ::checkOneExit()
{
	int nExits = 0;
	for (int i = 0; i < m_totalCells; i += m_width)
	{
		nExits += (m_CLMatrix.at(i).hasLeftWall() ? 0 : 1);
	}
	return nExits == 1;
}

bool MZ::checkEdgeCompatibility()
{
	CString sDebug;
	bool bEdgesCompatible = true;
	for (int i = 0; i < m_totalCells; i++)
	{
		if (!isLeftEdge(i))
		{
			bEdgesCompatible &= m_CLMatrix.at(i).hasLeftWall() == m_CLMatrix.at(neighBourLeft(i)).hasRightWall();
		}
		if (!isRightEdge(i))
		{
			bEdgesCompatible &= m_CLMatrix.at(i).hasRightWall() == m_CLMatrix.at(neighBourRight(i)).hasLeftWall();
		}
		bEdgesCompatible &= m_CLMatrix.at(i).hasTopWall() == m_CLMatrix.at(neighBourTop(i)).hasBottomWall();
		bEdgesCompatible &= m_CLMatrix.at(i).hasBottomWall() == m_CLMatrix.at(neighBourBottom(i)).hasTopWall();
	}

	return bEdgesCompatible;
}

bool MZ::checkMZ()
{
	if (mzString.GetLength() == 0)
	{
		OutputDebugString(_T("MZ length is 0! exiting ...\n"));
		return false;
	}
	else if (mzString.GetLength() != (m_width * m_height))
	{
		CString sDebug;
		sDebug.Format(_T("MZ dimensions error! MZ String length %s does not match width %i x height %i. exiting ...\n"), mzString, m_width, m_height);
		OutputDebugString(sDebug);
		return false;
	}
	return true;
}

void MZ::printMZString()
{
	std::cout << "\nMazeString:\n";
	std::cout << mzString << "\n\n";
}

int MZ::neighBourBottom(int cl)
{
	return (cl + m_width) % m_totalCells;
}

int MZ::neighBourRight(int cl)
{
	return (isRightEdge(cl) ? -1 : (cl + 1));
}

int MZ::neighBourTop(int cl)
{
	return (cl - m_width + m_totalCells) % m_totalCells;
}

int MZ::neighBourLeft(int cl)
{
	return (isLeftEdge(cl) ? -1 : (cl - 1));
}

bool MZ::isLeftEdge(int cl)
{
	return (cl % m_width) == 0;
}

bool MZ::isRightEdge(int cl)
{
	return ((cl + 1) % m_width) == 0;
}

bool MZ::isBottomRow(int cl)
{
	return cl >= m_totalCells - m_width;
}

int MZ::getRowNumber(int cl)
{
	return cl / m_width;
}

bool MZ::isLeftOf(int cell1, int cell2)
{
	bool bIsLeftOf;
	bIsLeftOf = (getRowNumber(cell1) == getRowNumber(cell2) && cell1 == cell2 - 1);

	//CString outPut;
	//outPut.Format(_T("%i isLeftOf %i ? %i\n"), cell1, cell2, bIsLeftOf);
	//OutputDebugString(outPut);
	return bIsLeftOf;
}

CString MZ::getMZString()
{
	return mzString;
}

int MZ::getEntrance()
{
	return m_entrance;
}

int MZ::getExit()
{
	return m_exit;
}

float MZ::getRouteCoverage()
{
	return m_fBestRouteCoverage;
}

float MZ::getRowUtilization()
{
	return m_fBestRouteRowsUtilized;
}

int MZ::getMinIntersections()
{
	return m_minIntersections;
}

int MZ::getOpposingSteps()
{
	return m_nOpposingSteps;
}

int MZ::getWidth()
{
	return m_width;
}

int MZ::getHeight()
{
	return m_height;
}

/*



0000	0
0001	1
0010	2
0011	3
0100	4
0101	5
0110	6
0111	7
1000	8
1001	9
1010	a
1011	b
1100	c
1101	d
1110	e
1111	f

*/