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


#include "CombinerExpression.h"

#include "Utility/Stream.h"
#include <memory>

//*****************************************************************************
//
//*****************************************************************************
static const char * const gCombinerInputNames[] =
{
	"Combined",		// CI_COMBINED,
	"Texel0",		// CI_TEXEL0,
	"Texel1",		// CI_TEXEL1,
	"Prim",			// CI_PRIMITIVE,
	"Shade",		// CI_SHADE,
	"Env",			// CI_ENV,
	"CombinedA",	// CI_COMBINED_ALPHA,
	"Texel0A",		// CI_TEXEL0_ALPHA,
	"Texel1A",		// CI_TEXEL1_ALPHA,
	"PrimA",		// CI_PRIMITIVE_ALPHA,
	"ShadeA",		// CI_SHADE_ALPHA,
	"EnvA",			// CI_ENV_ALPHA,
	"LodFrac",		// CI_LOD_FRACTION,
	"PrimLodFrac",	// CI_PRIM_LOD_FRACTION,
	"K5",			// CI_K5,
	"1",			// CI_1,
	"0",			// CI_0,
	"?",			// CI_UNKNOWN,
};

const char * GetCombinerInputName( ECombinerInput input )
{
	return gCombinerInputNames[ input ];
}


//*****************************************************************************
//
//*****************************************************************************
int CCombinerInput::Compare( const CCombinerOperand & other ) const
{
	int		type_diff( GetType() - other.GetType() );
	if( type_diff != 0 )
		return type_diff;

	const CCombinerInput & rhs( static_cast< const CCombinerInput & >( other ) );

	return int( mInput ) - int( rhs.mInput );
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream &	CCombinerInput::Stream( COutputStream & stream ) const
{
	return stream << GetCombinerInputName( mInput );
}


//*****************************************************************************
//
//*****************************************************************************
int CCombinerBlend::Compare( const CCombinerOperand & other ) const
{
	int		type_diff( GetType() - other.GetType() );
	if( type_diff != 0 )
		return type_diff;

	const CCombinerBlend & rhs( static_cast< const CCombinerBlend & >( other ) );

	int	input_diff;

	input_diff = mInputA->Compare( *rhs.mInputA );
	if( input_diff != 0 )
		return input_diff;

	input_diff = mInputB->Compare( *rhs.mInputB );
	if( input_diff != 0 )
		return input_diff;

	input_diff = mInputF->Compare( *rhs.mInputF );
	if( input_diff != 0 )
		return input_diff;

	// Equal
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream &	CCombinerBlend::Stream( COutputStream & stream ) const
{
	stream << "blend(";
	mInputA->Stream( stream );
	stream << ",";
	mInputB->Stream( stream );
	stream << ",";
	mInputF->Stream( stream );
	stream << ")";
	return stream;
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerSum::CCombinerSum()
:	CCombinerOperand( CT_SUM )
{

}

//*****************************************************************************
//
//*****************************************************************************
CCombinerSum::CCombinerSum( std::unique_ptr<CCombinerOperand> operand )
:	CCombinerOperand( CT_SUM )
{
	if( operand)
	{
		Add( std::move(operand) );
	}
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerSum::CCombinerSum( const CCombinerSum & rhs )
:	CCombinerOperand( CT_SUM )
{
	for ( const auto& node : rhs.mOperands)
	{
		mOperands.emplace_back(node.Operand->Clone(), node.Negate);
	}
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerSum::~CCombinerSum()
{
	mOperands.clear();
}

//*****************************************************************************
//
//*****************************************************************************
int CCombinerSum::Compare( const CCombinerOperand & other ) const
{
	int		type_diff( GetType() - other.GetType() );
	if( type_diff != 0 )
		return type_diff;

	const CCombinerSum & rhs( static_cast< const CCombinerSum & >( other ) );
	int size_diff( mOperands.size() - rhs.mOperands.size() );
	if( size_diff != 0 )
		return size_diff;

	for( u32 i = 0; i < mOperands.size(); ++i )
	{
		// Compare signs first
		if( mOperands[ i ].Negate && !rhs.mOperands[ i ].Negate )
			return -1;
		else if ( !mOperands[ i ].Negate && rhs.mOperands[ i ].Negate )
			return 1;

		int diff( mOperands[ i ].Operand->Compare( *rhs.mOperands[ i ].Operand ) );
		if( diff != 0 )
		{
			return diff;
		}
	}

	// Equal
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
void CCombinerSum::Add( std::unique_ptr<CCombinerOperand> operand )
{
	if( operand->IsInput( CI_0 ) )
	{
		return;
	}
	else if( operand->IsSum() )
	{
		// Recursively add all children
		auto*	sum =static_cast< CCombinerSum * >( operand.get() );
		for( u32 i = 0; i < sum->mOperands.size(); ++i )
		{
			if( sum->mOperands[ i ].Negate )
			{
				Sub( sum->mOperands[ i ].Operand->SimplifyAndReduce() );
			}
			else
			{
				Add( sum->mOperands[ i ].Operand->SimplifyAndReduce() );
			}
		}

	}
	else
	{
		mOperands.emplace_back( Node( std::move(operand), false ) );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CCombinerSum::Sub( std::unique_ptr<CCombinerOperand> operand )
{
	if( operand->IsInput( CI_0 ) )
	{
		return;
	}
	else if( operand->IsSum() )
	{
		// Recursively add all children
		auto *sum =  static_cast< CCombinerSum * >( operand.get() );
		for( u32 i = 0; i < sum->mOperands.size(); ++i )
		{
			if( sum->mOperands[ i ].Negate )
			{
				Add( sum->mOperands[ i ].Operand->SimplifyAndReduce() );			// Note we Add, not Sub
			}
			else
			{
				Sub( sum->mOperands[ i ].Operand->SimplifyAndReduce() );			// Note we Sub, not Add
			}
		}
	}
	else
	{
		mOperands.emplace_back(std::move(operand), true);

	}
}

//*****************************************************************************
//
//*****************************************************************************
// Try to reduce this operand to a blend. If it fails, returns NULL
std::unique_ptr<CCombinerOperand> CCombinerSum::ReduceToBlend() const
{
	// We're looking for expressions of the form (A + (f * (B - A)))
	if (mOperands.size() == 2)
	{
		if (!mOperands[0].Negate &&
			!mOperands[1].Negate && mOperands[1].Operand->IsProduct())
		{
			const CCombinerOperand* input_a = mOperands[0].Operand.get();
			const auto* product = static_cast<const CCombinerProduct*>(mOperands[1].Operand.get());

			if (product->GetNumOperands() == 2)
			{
				const CCombinerOperand* factor = product->GetOperand(0); // f
				const CCombinerOperand* diff = product->GetOperand(1);   // B - A

				if (diff->IsSum())
				{
					const auto* diff_sum = static_cast<const CCombinerSum*>(diff);

					if (diff_sum->mOperands.size() == 2)
					{
						if (!diff_sum->mOperands[0].Negate &&
							diff_sum->mOperands[1].Negate &&
							diff_sum->mOperands[1].Operand->IsEqual(*input_a))
						{
							const CCombinerOperand* input_b = diff_sum->mOperands[0].Operand.get();

							return std::make_unique<CCombinerBlend>(
								input_a->SimplifyAndReduce(),
								input_b->SimplifyAndReduce(),
								factor->SimplifyAndReduce()
							);
						}
					}
				}
			}
		}
	}

	return nullptr;
}

//*****************************************************************************
//
//*****************************************************************************
std::unique_ptr<CCombinerOperand> CCombinerSum::SimplifyAndReduce() const
{
	// If we consist of a single element, hoist that up
	if( mOperands.size() == 1 && mOperands[ 0 ].Negate == false )		// XXXX
	{
		return mOperands[ 0 ].Operand->SimplifyAndReduce();
	}

	auto	blend =  ReduceToBlend();
	if( blend)
	{
		return blend;
	}

	auto new_add( std::make_unique<CCombinerSum>() );

	for( std::vector< Node >::const_iterator it = mOperands.begin(); it != mOperands.end(); ++it )
	{
		if( it->Negate )
			new_add->Sub( it->Operand->SimplifyAndReduce() );
		else
			new_add->Add( it->Operand->SimplifyAndReduce() );
	}

	new_add->SortOperands();

	return new_add;
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream &	CCombinerSum::Stream( COutputStream & stream ) const
{
	stream << "( ";
	for( u32 i = 0; i < mOperands.size(); ++i )
	{
		if( i != 0 )
		{
			stream << (mOperands[ i ].Negate ? " - " : " + ");
		}
		else
		{
			if( mOperands[ i ].Negate )
				stream << "-";
		}
		mOperands[ i ].Operand->Stream( stream );
	}
	stream << " )";
	return stream;
}


//*****************************************************************************
//
//*****************************************************************************
CCombinerProduct::CCombinerProduct()
:	CCombinerOperand( CT_PRODUCT )
{

}

//*****************************************************************************
//
//*****************************************************************************
CCombinerProduct::CCombinerProduct( std::unique_ptr<CCombinerOperand> operand )
:	CCombinerOperand( CT_PRODUCT )
{
	if( operand)
	{
		Mul( std::move(operand) );
	}
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerProduct::CCombinerProduct(const CCombinerProduct& rhs)
: CCombinerOperand(CT_PRODUCT)
{
	for (const auto& node : rhs.mOperands)
	{
		mOperands.emplace_back(node.Operand->Clone());
	}
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerProduct::~CCombinerProduct()
{
	Clear();
}

//*****************************************************************************
//
//*****************************************************************************
void CCombinerProduct::Clear()
{

	mOperands.clear();
}

//*****************************************************************************
//
//*****************************************************************************
int CCombinerProduct::Compare( const CCombinerOperand & other ) const
{
	int		type_diff( GetType() - other.GetType() );
	if( type_diff != 0 )
		return type_diff;

	const CCombinerProduct & rhs( static_cast< const CCombinerProduct & >( other ) );
	int size_diff( mOperands.size() - rhs.mOperands.size() );
	if( size_diff != 0 )
		return size_diff;

	for( u32 i = 0; i < mOperands.size(); ++i )
	{
		int diff( mOperands[ i ].Operand->Compare( *rhs.mOperands[ i ].Operand ) );
		if( diff != 0 )
		{
			return diff;
		}
	}

	// Equal
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
void CCombinerProduct::Mul(std::unique_ptr<CCombinerOperand> operand)
{
	if (operand->IsInput(CI_0))
	{
		// Clear and insert just 0
		Clear();
		mOperands.emplace_back(std::move(operand));
		return;
	}

	if (operand->IsInput(CI_1))
	{
		// Ignore multiplicative identity
		return;
	}

	if (operand->IsProduct())
	{
		// Steal its children
		auto* product = static_cast<CCombinerProduct*>(operand.get());

		for (auto& node : product->mOperands)
		{
			auto simplified = node.Operand->SimplifyAndReduce();
			Mul(std::move(simplified));
		}

		// `operand` is discarded here automatically
		return;
	}

	// Default case: insert operand as a new term
	mOperands.emplace_back(std::move(operand));
}


//*****************************************************************************
//
//*****************************************************************************
std::unique_ptr<CCombinerOperand> CCombinerProduct::SimplifyAndReduce() const
{
	// If we consist of a single element, hoist that up
	if( mOperands.size() == 1 )
	{
		return mOperands[ 0 ].Operand->SimplifyAndReduce();
	}

	auto	new_mul = std::make_unique<CCombinerProduct>();

	for( std::vector< Node >::const_iterator it = mOperands.begin(); it != mOperands.end(); ++it )
	{
		new_mul->Mul( it->Operand->SimplifyAndReduce() );
	}

	new_mul->SortOperands();

	return new_mul;
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream &		CCombinerProduct::Stream( COutputStream & stream ) const
{
	stream << "( ";
	for( u32 i = 0; i < mOperands.size(); ++i )
	{
		if( i != 0 )
		{
			stream << " * ";
		}
		mOperands[ i ].Operand->Stream( stream );
	}
	stream << " )";
	return stream;
}
