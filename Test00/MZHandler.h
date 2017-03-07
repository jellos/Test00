#pragma once

class MZHandler
{

private:

	bool		m_bConnectedToMySQL;
	bool		m_bVerbose;

public:

	MZHandler();
	~MZHandler();
	int			Go();
};