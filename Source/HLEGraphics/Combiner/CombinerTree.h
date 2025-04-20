/*
Copyright (C) 2007 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#ifndef HLEGRAPHICS_COMBINER_COMBINERTREE_H_
#define HLEGRAPHICS_COMBINER_COMBINERTREE_H_

#include "Base/Types.h"
#include "CombinerInput.h"
#include <memory>


class CAlphaRenderSettings;
class CBlendStates;
class CCombinerOperand;
class COutputStream;

//*****************************************************************************
//
//*****************************************************************************
class CCombinerTree
{
public:
	CCombinerTree( u64 mux, bool two_cycles );
	~CCombinerTree();

	COutputStream &				Stream( COutputStream & stream ) const;

	const CBlendStates *		GetBlendStates() const				{ return mBlendStates; }

private:
	CBlendStates *				GenerateBlendStates( const CCombinerOperand * colour_operand, const CCombinerOperand * alpha_operand ) const;

	CAlphaRenderSettings *		GenerateAlphaRenderSettings(  const CCombinerOperand * operand ) const;
	void						GenerateRenderSettings( CBlendStates * states, const CCombinerOperand * operand ) const;


	static std::unique_ptr<CCombinerOperand> Simplify( std::unique_ptr<CCombinerOperand> operand );


	static std::unique_ptr<CCombinerOperand>	BuildCycle1( ECombinerInput a, ECombinerInput b, ECombinerInput c, ECombinerInput d );
	static std::unique_ptr<CCombinerOperand>	BuildCycle2( ECombinerInput a, ECombinerInput b, ECombinerInput c, ECombinerInput d, std::unique_ptr<CCombinerOperand> cycle_1_output );

	static std::unique_ptr<CCombinerOperand> Build( std::unique_ptr<CCombinerOperand> a, std::unique_ptr<CCombinerOperand>  b, std::unique_ptr<CCombinerOperand>  c, std::unique_ptr<CCombinerOperand>  d );

private:
	u64							mMux;
	std::unique_ptr<CCombinerOperand>				mCycle1;
	std::unique_ptr<CCombinerOperand>				mCycle1A;
	std::unique_ptr<CCombinerOperand>				mCycle2;
	std::unique_ptr<CCombinerOperand>				mCycle2A;

	CBlendStates *				mBlendStates;
};

#endif // HLEGRAPHICS_COMBINER_COMBINERTREE_H_
