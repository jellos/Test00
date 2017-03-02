#include <atlstr.h>
#include <vector>
#include "CL.h"

class MZ
{

private:

	CString				mzString;
	int					m_width,
						m_height,
						m_totalCells,
						m_widthVisual,
						m_heightVisual,
						m_nTries,
						m_nWalls,
						m_nMaxWalls,
						m_entrance,
						m_exit,
						m_minIntersections,
						m_nVisited,
						m_iRowsUtilized,
						m_nOpposingSteps;

	bool				m_bAllCellsReachable;

	float				m_fBestRouteCoverage,
						m_fBestRouteRowsUtilized;

	std::vector<CL>		m_CLMatrix;
	std::vector<bool>	m_CLMatrixVisual;
	std::vector<int>	m_CLIntersections;
	std::vector<int>	m_CLWallCount;
	std::vector<std::vector<int>> m_vRoutes;
	std::vector<bool>	m_vVisited;
	std::vector<int>	m_vBestRoute;
	std::vector<int>	m_vRowHistogram;
	std::vector<int>	m_vIntersectionProblems;

	void				shiftDown(int offset);
	bool				checkMZ();
	bool				checkStuff();
	void				initializeCLMatrix();
	int					neighBourTop(int cl);
	int					neighBourRight(int cl);
	int					neighBourBottom(int cl);
	int					neighBourLeft(int cl);
	bool				isLeftEdge(int cl);
	bool				isRightEdge(int cl);
	bool				isBottomRow(int cl);
	int					getRowNumber(int cl);
	bool				checkEdgeCompatibility();
	bool				checkOneEntrance();
	bool				checkOneExit();
	void				updateVisual();
	void				updateIntersections();
	void				updateWallCount();
	void				updateVectors(bool bVerbose = false);
	bool				addRandomWall(int &cell, int &side);
	void				removeWallAt(int cell, int side);
	void				addRandomEntrance();
	void				addRandomExit();
	void				fillWalllessCells(bool bVerbose = false);
	void				updateRoutes();
	void				updateMZString();
	void				calculateMazeAndRouteStats();
	bool				isLeftOf(int cell1, int cell2);

public:

	MZ();
	~MZ();

	bool				setMZ(int width, int height, int entrance, int exit, const CString &s);
	void				generateMZ(int width, int height, bool bVerbose = false);
	
	void				printMZString();
	void				printVisual(bool doubly = false);
	void				printIntersections();
	void				printWallCount();
	void				printRoutesAndStats();

	//getters
	CString				getMZString();
	int					getEntrance();
	int					getExit();
	float				getRouteCoverage();
	float				getRowUtilization();
	int					getMinIntersections();
	int					getOpposingSteps();
	int					getWidth();
	int					getHeight();

};