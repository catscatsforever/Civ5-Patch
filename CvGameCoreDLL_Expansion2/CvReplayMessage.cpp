/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include ".\cvreplaymessage.h"
#include "CvEnumSerialization.h"

// include this after all other headers!
#include "LintFree.h"

#ifdef REPLAY_MESSAGE_EXTENDED
//------------------------------------------------------------------------------
unsigned int CvReplayMessage::Version()
{
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	return BUMP_SAVE_VERSION_REPLAYMESSAGE;
# else
	return 2;
# endif
}
//------------------------------------------------------------------------------
CvReplayMessage::CvReplayMessage()
	: m_iTurn(-1)
	, m_iTimeMilliseconds(0)
	, m_eType(NO_REPLAY_MESSAGE)
	, m_ePlayer(NO_PLAYER)
{
}
//------------------------------------------------------------------------------
CvReplayMessage::CvReplayMessage(int iTurn, ReplayMessageTypes eType, PlayerTypes ePlayer) :
	m_iTurn(iTurn),
	m_iTimeMilliseconds(static_cast<int>(GC.getGame().getTimeElapsed() * 1000)),
	m_ePlayer(ePlayer),
	m_eType(eType)
{
}
#else
//------------------------------------------------------------------------------
unsigned int CvReplayMessage::Version()
{
	return 2;
}
//------------------------------------------------------------------------------
CvReplayMessage::CvReplayMessage()
	: m_iTurn(-1)
	, m_eType(NO_REPLAY_MESSAGE)
	, m_ePlayer(NO_PLAYER)
{
}
//------------------------------------------------------------------------------
CvReplayMessage::CvReplayMessage(int iTurn, ReplayMessageTypes eType, PlayerTypes ePlayer) :
	m_iTurn(iTurn),
	m_ePlayer(ePlayer),
	m_eType(eType)
{
}
#endif
//------------------------------------------------------------------------------
CvReplayMessage::~CvReplayMessage()
{
}
//------------------------------------------------------------------------------
void CvReplayMessage::setTurn(int iTurn)
{
	m_iTurn = iTurn;
}
//------------------------------------------------------------------------------
int CvReplayMessage::getTurn() const
{
	return m_iTurn;
}
//------------------------------------------------------------------------------
void CvReplayMessage::setType(ReplayMessageTypes eType)
{
	m_eType = eType;
}
//------------------------------------------------------------------------------
ReplayMessageTypes CvReplayMessage::getType() const
{
	return m_eType;
}
//------------------------------------------------------------------------------
void CvReplayMessage::setPlayer(PlayerTypes ePlayer)
{
	m_ePlayer = ePlayer;
}
//------------------------------------------------------------------------------
PlayerTypes CvReplayMessage::getPlayer() const
{
	return m_ePlayer;
}
//------------------------------------------------------------------------------
void CvReplayMessage::setText(const CvString& strText)
{
	m_strText = strText;
}
//------------------------------------------------------------------------------
const CvString& CvReplayMessage::getText() const
{
	return m_strText;
}
//------------------------------------------------------------------------------
void CvReplayMessage::addPlot(int iPlotX, int iPlotY)
{
	short sPlotX = (short)iPlotX;
	short sPlotY = (short)iPlotY;

	for(PlotPositionList::iterator it = m_Plots.begin(); it != m_Plots.end(); ++it)
	{
		const PlotPosition& position = (*it);
		if(position.first == sPlotX && position.second == sPlotY)
			return;
	}

	m_Plots.push_back(PlotPosition(sPlotX, sPlotY));
}
//------------------------------------------------------------------------------
bool CvReplayMessage::getPlot(unsigned int idx, int& iPlotX, int& iPlotY) const
{
	if(idx < m_Plots.size())
	{
		const PlotPosition& position = m_Plots[idx];
		iPlotX = (int)position.first;
		iPlotY = (int)position.second;
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------
unsigned int CvReplayMessage::getNumPlots() const
{
	return m_Plots.size();
}
//------------------------------------------------------------------------------
void CvReplayMessage::clearPlots()
{
	m_Plots.clear();
}
#ifdef REPLAY_MESSAGE_EXTENDED
void CvReplayMessage::setTimestamp(float fTime)
{
	m_iTimeMilliseconds = static_cast<int>(fTime);
}
int CvReplayMessage::getTimestamp() const
{
	return m_iTimeMilliseconds;
}
#endif
//------------------------------------------------------------------------------
void CvReplayMessage::read(FDataStream& kStream, unsigned int uiVersion)
{
	UNREFERENCED_PARAMETER(uiVersion);

	kStream >> m_iTurn;
	kStream >> m_eType;

	int nPlots = -1;
	kStream >> nPlots;
	if(nPlots > 0)
	{
		m_Plots.reserve(nPlots);
		for(int i = 0; i < nPlots; ++i)
		{
			short sPlotX, sPlotY;
			kStream >> sPlotX;
			kStream >> sPlotY;
			m_Plots.push_back(PlotPosition(sPlotX, sPlotY));
		}
	}
#ifdef REPLAY_MESSAGE_EXTENDED
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	if (uiVersion >= BUMP_SAVE_VERSION_REPLAYMESSAGE)
	{
# endif
		kStream >> m_iTimeMilliseconds;
# ifdef SAVE_BACKWARDS_COMPATIBILITY
	}
	else
	{
		m_iTimeMilliseconds = 0;
	}
# endif
#endif

	kStream >> m_ePlayer;
	kStream >> m_strText;
}
//------------------------------------------------------------------------------
void CvReplayMessage::write(FDataStream& kStream) const
{
	kStream << m_iTurn;
	kStream << m_eType;

	kStream << (int)m_Plots.size();
	for(PlotPositionList::const_iterator it = m_Plots.begin(); it != m_Plots.end(); ++it)
	{
		kStream << (*it).first;
		kStream << (*it).second;
	}
#ifdef REPLAY_MESSAGE_EXTENDED
	kStream << m_iTimeMilliseconds;
#endif

	kStream << m_ePlayer;
	kStream << m_strText;
}
//------------------------------------------------------------------------------
