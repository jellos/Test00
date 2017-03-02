#pragma once

class MZHandler
{

private:

	bool		m_bConnectedToMySQL;
	bool		m_bVerbose;
	int			m_iGenerateNMZ;

public:

	MZHandler();
	~MZHandler();
	int			Go(bool bGenerateNew);
};