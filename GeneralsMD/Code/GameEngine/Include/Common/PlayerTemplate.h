/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: PlayerTemplate.h ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Westwood Studios Pacific.
//
//                       Confidential Information
//                Copyright (C) 2001 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
// Project:    RTS3
//
// File name:  PlayerTemplate.h
//
// Created:    Steven Johnson, October 2001
//
// Desc:			 @todo
//
//-----------------------------------------------------------------------------

#pragma once

#ifndef _PLAYERTEMPLATE_H_
#define _PLAYERTEMPLATE_H_

#include "Common/SubsystemInterface.h"
#include "Common/GameMemory.h"
#include "Common/Debug.h"
#include "Common/Handicap.h"
#include "Common/Money.h"
#include "Common/Science.h"
#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/NameKeyGenerator.h"
#include "Common/STLTypedefs.h"


// ----------------------------------------------------------------------------------------------

class INI;
struct FieldParse;
class Image;

// ----------------------------------------------------------------------------------------------

#define MAX_MP_STARTING_UNITS 10

// ----------------------------------------------------------------------------------------------
class PlayerTemplate
{

public:

	PlayerTemplate();

	inline void setNameKey(NameKeyType namekey) { m_nameKey = namekey; }

	inline NameKeyType getNameKey() const { DEBUG_ASSERTCRASH(m_nameKey != NAMEKEY_INVALID, ("bad namekey")); return m_nameKey; }
	inline AsciiString getName() const { return KEYNAME(m_nameKey); }

	inline UnicodeString getDisplayName() const { return m_displayName; }

	inline AsciiString getSide() const { return m_side; }
	inline AsciiString getBaseSide() const  { return m_baseSide; }

	/// return the tech tree for the player.
	inline const Handicap *getHandicap() const { return &m_handicap; }

	/// return the money for the player.
	inline const Money *getMoney() const { return &m_money; }

	inline const RGBColor* getPreferredColor() const { return &m_preferredColor; }

	inline AsciiString getStartingBuilding( void ) const { return m_startingBuilding; }
	AsciiString getStartingUnit( Int i ) const;

	inline const ProductionChangeMap& getProductionCostChanges() const { return m_productionCostChanges; }
	inline const ProductionChangeMap& getProductionTimeChanges() const { return m_productionTimeChanges; }
	inline const ProductionVeterancyMap& getProductionVeterancyLevels() const { return m_productionVeterancyLevels; }
	inline Bool isObserver() const { return m_observer; }
	inline Bool isPlayableSide() const { return m_playableSide; }

	inline AsciiString getScoreScreen (void ) const { return m_scoreScreenImage;	}
	inline AsciiString getLoadScreen (void ) const { return m_loadScreenImage;	}
	inline AsciiString getBeaconTemplate( void ) const { return m_beaconTemplate; }

	const Image *getHeadWaterMarkImage( void ) const;
	const Image *getFlagWaterMarkImage( void ) const;
	const Image *getEnabledImage( void ) const;
	//const Image *getDisabledImage( void ) const;
	//const Image *getHiliteImage( void ) const;
	//const Image *getPushedImage( void ) const;
	const Image *getSideIconImage( void ) const;
	const Image *getGeneralImage( void ) const;
	inline const AsciiString getTooltip() const { return m_tooltip; }
	inline const AsciiString getGeneralFeatures( void ) const { return m_strGeneralFeatures; }

	AsciiString getMedallionNormal() const { return m_strMedallionNormal; }
	AsciiString getMedallionHilite() const { return m_strMedallionHilite; }
	AsciiString getMedallionSelected() const { return m_strMedallionSelected; }

	const ScienceVec& getIntrinsicSciences() const { return m_intrinsicSciences; }
	Int getIntrinsicSciencePurchasePoints() const { return m_intrinsicSPP; }
	AsciiString getPurchaseScienceCommandSetRank1( void ) const {return m_purchaseScienceCommandSetRank1;	}
	AsciiString getPurchaseScienceCommandSetRank3( void ) const {return m_purchaseScienceCommandSetRank3;	}
	AsciiString getPurchaseScienceCommandSetRank8( void ) const {return m_purchaseScienceCommandSetRank8;	}

	AsciiString getSpecialPowerShortcutCommandSet( void ) const {return m_specialPowerShortcutCommandSet;	}
	AsciiString getSpecialPowerShortcutWinName( void ) const {return m_specialPowerShortcutWinName;	}
	Int getSpecialPowerShortcutButtonCount( void ) const {return m_specialPowerShortcutButtonCount;	}

	AsciiString getLoadScreenMusic( void ) const {return m_loadScreenMusic;	}
	AsciiString getScoreScreenMusic() const { return m_scoreScreenMusic; }

  inline Bool isOldFaction( void ) const { return m_oldFaction; }

	static const FieldParse* getFieldParse();


protected:

	// for parsing from INI
	static void parseStartMoney( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseProductionCostChange( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseProductionTimeChange( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseProductionVeterancyLevel( INI* ini, void *instance, void *store, const void* /*userData*/ );

private:

	NameKeyType			m_nameKey;
	UnicodeString		m_displayName;
	AsciiString			m_side, m_baseSide;
	Handicap				m_handicap;						///< initial baseline for Player capabilities
	Money						m_money;							///< starting credits, if any
	RGBColor				m_preferredColor;			///< our preferred starting color
	AsciiString			m_startingBuilding;		///< template name of our multiplayer start building (con yard, i'd expect)
	AsciiString			m_startingUnits[MAX_MP_STARTING_UNITS];		///< template names of our multiplayer start units
	ProductionChangeMap			m_productionCostChanges;
	ProductionChangeMap			m_productionTimeChanges;
	ProductionVeterancyMap	m_productionVeterancyLevels;
	ScienceVec				m_intrinsicSciences;	///< sciences that this Player will always know
	AsciiString				m_purchaseScienceCommandSetRank1;		///< the command set we'll use when we want to purchase sciences
	AsciiString				m_purchaseScienceCommandSetRank3;		///< the command set we'll use when we want to purchase sciences
	AsciiString				m_purchaseScienceCommandSetRank8;		///< the command set we'll use when we want to purchase sciences

	AsciiString				m_specialPowerShortcutCommandSet;		///< the command set we'll use when we want to show the shortcut to the special powers
	AsciiString				m_specialPowerShortcutWinName;			///< The name of the window we'll be using for the shortcut bar
	Int								m_specialPowerShortcutButtonCount;	///< The number of buttons located on the shortcut bar
	AsciiString				m_loadScreenMusic;									///< the load screen music we want to play
	AsciiString				m_scoreScreenMusic;									///< the score screen music we want to play
	AsciiString				m_tooltip;								///< The tooltip describing this player template
	Bool							m_observer;
	Bool							m_playableSide;
  Bool              m_oldFaction;                       ///< Faction existed in the original Generals

	Int								m_intrinsicSPP;

	AsciiString			m_scoreScreenImage;			///< Image that will be shown on the score screen
	AsciiString			m_loadScreenImage;

	AsciiString			m_headWaterMark;				///< Image that will be the background on the observer control bar
	AsciiString			m_flagWaterMark;				///< Imabe that will be the background on the observer control bar
	AsciiString			m_enabledImage;					///< enable button image
	//AsciiString			m_disabledImage;				///< disabled button image
	//AsciiString			m_hiliteImage;					///< hilite button image
	//AsciiString			m_pushedImage;					///< pushed button image
	AsciiString			m_sideIconImage;				///< The little icon we show on game info screens for the sides
	AsciiString			m_generalImage;				///< The icon overlayed on the rank image in My Persona

	AsciiString			m_beaconTemplate;				///< ThingTemplate name for beacons

	AsciiString m_strGeneralFeatures;		///< used in loadscreens
	AsciiString m_strMedallionNormal;
	AsciiString m_strMedallionHilite;
	AsciiString m_strMedallionSelected;
};

// ----------------------------------------------------------------------------------------------
/**
	This is a singleton class that maintains the list of PlayerTemplates.
*/
class PlayerTemplateStore : public SubsystemInterface
{
public:

	PlayerTemplateStore();
	~PlayerTemplateStore();

	virtual void init();
	virtual void reset();
	virtual void update();

	static void parsePlayerTemplateDefinition( INI* ini );

	const PlayerTemplate* getNthPlayerTemplate(Int i) const;
	const PlayerTemplate* findPlayerTemplate(NameKeyType namekey) const;
	inline Int getPlayerTemplateCount() const { return m_playerTemplates.size(); }
	Int getTemplateNumByName(AsciiString name) const;

	// This function will fill outStringList with all the sides found in all the templates
	void getAllSideStrings(AsciiStringList *outStringList);

private:

	typedef std::vector<PlayerTemplate> PlayerTemplateVector;

	PlayerTemplateVector m_playerTemplates;
};

// ----------------------------------------------------------------------------------------------
extern PlayerTemplateStore *ThePlayerTemplateStore;	///< singleton instance of PlayerTemplateStore

#endif // _PLAYERTEMPLATE_H_
