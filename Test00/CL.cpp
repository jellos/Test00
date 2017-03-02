#include "stdafx.h"
#include "CL.h"
#include <math.h>

/*
0			0000
1			0001
2			0010
3			0011
4			0100
5			0101
6			0110
7			0111
8			1000
9			1001
a	:		1010
b	;		1011
c	<		1100
d	=		1101
e	>		1110
f	?		1111
*/

CL::CL(const char c)
{
	m_1fchar = c;
	m_char = humanReadableToInternalChar(c);
}

CL::~CL()
{}

char CL::internalTohumanReadableChar(const char c)
{
	switch (c)
	{
	case ':':
		return 'a';
	case ';':
		return 'b';
	case '<':
		return 'c';
	case '=':
		return 'd';
	case '>':
		return 'e';
	case '?':
		return 'f';
	default:
		return c;
	}
}

char CL::humanReadableToInternalChar(const char c)
{
	switch (c)
	{
	case 'a':
		return ':';
	case 'b':
		return ';';
	case 'c':
		return '<';
	case 'd':
		return '=';
	case 'e':
		return '>';
	case 'f':
		return '?';
	default:
		return c;
	}
}

bool CL::hasTopWall()
{
	return (m_char >= '8' && m_char <= '?');
}

bool CL::hasRightWall()
{
	return ((m_char >= '4' && m_char <= '7') || (m_char >= '<' && m_char <= '?'));
}

bool CL::hasBottomWall()
{
	return (m_char == '2' || m_char == '3' || m_char == '6' || m_char == '7' || m_char == ':' || m_char == ';' || m_char == '>' || m_char == '?');
}

bool CL::hasLeftWall()
{
	return (m_char == '1' || m_char == '3' || m_char == '5' || m_char == '7' || m_char == '9' || m_char == ';' || m_char == '=' || m_char == '?');
}

int CL::addRandomWall()
{
	int r = rand() % 4;

	if (r == 0 && hasTopWall())
		return addRandomWall();
	if (r == 1 && hasRightWall())
		return addRandomWall();
	if (r == 2 && hasBottomWall())
		return addRandomWall();
	if (r == 3 && hasLeftWall())
		return addRandomWall();

	//CString sDebug;
	//sDebug = _T("m_char was ");
	//sDebug.AppendChar(m_char);
	m_char += (int) pow((double)2, (3 - r));
	//sDebug.Append(_T(" and has become "));
	//sDebug.AppendChar(m_char);
	//sDebug.Append(_T("\n"));
	//OutputDebugString(sDebug);

	m_1fchar = internalTohumanReadableChar(m_char);
	return r;
}

void CL::addWallAtSide(int s)
{
	switch (s)
	{
	case 0:
		m_char += 8;
		break;
	case 1:
		m_char += 4;
		break;
	case 2:
		m_char += 2;
		break;
	case 3:
		m_char += 1;
		break;
	}
	m_1fchar = internalTohumanReadableChar(m_char);
}

void CL::removeWallAt(int s)
{
	switch (s)
	{
	case 0:
		m_char -= 8;
		break;
	case 1:
		m_char -= 4;
		break;
	case 2:
		m_char -= 2;
		break;
	case 3:
		m_char -= 1;
		break;
	}
	m_1fchar = internalTohumanReadableChar(m_char);
}

int CL::getNumberOfWalls()
{
	if (m_char == '0')
		return 0;
	else if (m_char == '1' || m_char == '2' || m_char == '4' || m_char == '8')
		return 1;
	else if (m_char == '3' || m_char == '5' || m_char == '6' || m_char == '9' || m_char == ':' || m_char == '<')
		return 2;
	else if (m_char == '7' || m_char == ';' || m_char == '=' || m_char == '>')
		return 3;
	else if (m_char == '?')
		return 4;

	return 5;
}

char CL::getHumanReadableChar()
{
	return m_1fchar;
}