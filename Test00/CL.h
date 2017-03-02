#include "stdafx.h"
#include <atlstr.h>

class CL
{

private:

	char		m_char;			//character in the range 0-9 and :;<=>?
	char		m_1fchar;		//character in the range 0-9 and abcdef
	
public:

	CL(const char c);
	~CL();

	bool		hasTopWall();
	bool		hasRightWall();
	bool		hasBottomWall();
	bool		hasLeftWall();
	int			addRandomWall();
	void		addWallAtSide(int s);
	int			getNumberOfWalls();
	void		removeWallAt(int s);
	char		internalTohumanReadableChar(const char c);
	char		humanReadableToInternalChar(const char c);
	char		getHumanReadableChar();
};