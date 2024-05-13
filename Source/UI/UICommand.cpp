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


#include "Base/Types.h"
#include "DrawTextUtilities.h"
#include "UICommand.h"
#include "UIContext.h"


u32		CUICommand::GetHeight(std::shared_ptr<CUIContext> context ) const
{
	return context->GetFontHeight() + 2;
}

void	CUICommand::Draw(std::shared_ptr<CUIContext> context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const
{
	c32	colour;
	if( !IsSelectable())
	{
		colour = DrawTextUtilities::TextWhiteDisabled;
	}
	else if( selected )
	{
		colour = context->GetSelectedTextColour();
	}
	else
	{
		colour = context->GetDefaultTextColour();
	}

	context->DrawTextAlign( min_x, max_x, halign, y + context->GetFontHeight(), mName.c_str(), colour );
}
