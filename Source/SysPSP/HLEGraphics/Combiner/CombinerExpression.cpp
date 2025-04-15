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

#include "SysPSP/HLEGraphics/Combiner/CombinerExpression.h"
#include "Utility/Stream.h"
#include <ostream>

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



int CCombinerInput::Compare( const CCombinerOperand & other ) const
{
	int		type_diff( GetType() - other.GetType() );
	if( type_diff != 0 )
		return type_diff;

	const CCombinerInput & rhs( static_cast< const CCombinerInput & >( other ) );

	return int( mInput ) - int( rhs.mInput );
}


COutputStream &	CCombinerInput::Stream( COutputStream & stream ) const
{
	return stream << GetCombinerInputName( mInput );
}



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


CCombinerSum::CCombinerSum()
:	CCombinerOperand( CT_SUM ) {}


CCombinerSum::CCombinerSum( std::unique_ptr<CCombinerOperand> operand )
:	CCombinerOperand( CT_SUM )
{
	if(operand)
	{
		Add(std::move(operand) );
	}
}


CCombinerSum::CCombinerSum( const CCombinerSum & rhs )
:	CCombinerOperand( CT_SUM )
{
	for( const auto& node :rhs.mOperands)
	{
		mOperands.emplace_back(node.Operand->Clone(), node.Negate);
	}
}

CCombinerSum::~CCombinerSum() {}


int CCombinerSum::Compare( const CCombinerOperand & other ) const
{
	int	type_diff =  GetType() - other.GetType();
	if( type_diff != 0 )
		return type_diff;

		const auto* rhs = dynamic_cast< const CCombinerSum* >(&other);

		int size_diff = mOperands.size() - rhs->mOperands.size();
	if( size_diff != 0 )
		return size_diff;

	for( size_t i = 0; i < mOperands.size(); ++i )
	{
		// Compare signs first
		const bool lhs_neg = mOperands[i].Negate;
		const bool rhs_neg = rhs->mOperands[i].Negate;

		if (lhs_neg != rhs_neg)
			return lhs_neg ? -1 : 1;

		int diff =  mOperands[i].Operand->Compare( *rhs->mOperands[ i ].Operand );
		if( diff != 0 )
			return diff;
	}

	return 0; // Equal
} 


void CCombinerSum::Add(std::unique_ptr<CCombinerOperand> operand)
{
	if (operand->IsInput(CI_0))
	{
		// Ignore zero input
		return;
	}
	else if (operand->IsSum())
	{
		// Recursively add all children
		CCombinerSum* sum = dynamic_cast<CCombinerSum*>(operand.get());
		DAEDALUS_ASSERT(sum != nullptr, "Expected CCombinerSum");

		for (const auto& entry : sum->mOperands)
		{
			auto simplified = entry.Operand->SimplifyAndReduce();

			if (entry.Negate)
				Sub(std::move(simplified));
			else
				Add(std::move(simplified));
		}
	}
	else
	{
		mOperands.emplace_back(std::move(operand), false);
	}
}


void CCombinerSum::Sub( std::unique_ptr<CCombinerOperand> operand )
{
	if( operand->IsInput( CI_0 ) )
	{
		// Ignore zero input
		return;
	}
	else if( operand->IsSum() )
	{
		// Recursively add all children
		CCombinerSum* sum = dynamic_cast<CCombinerSum*>(operand.get());

		for (const auto& entry : sum->mOperands)
		{
			auto simplified = entry.Operand->SimplifyAndReduce();

			if (entry.Negate)
				Add(std::move(simplified));
			else
				Sub(std::move(simplified));
		}
	}
	else
	{
		mOperands.emplace_back(std::move(operand), true);
	}
}


// Try to reduce this operand to a blend. If it fails, returns NULL
std::unique_ptr<CCombinerOperand> CCombinerSum::ReduceToBlend() const
{
	// We're looking for expressions of the form (A + (f * (B - A)))
	if (mOperands.size() == 2)
	{
		const auto& op0 = mOperands[0];
		const auto& op1 = mOperands[1];

		if (!op0.Negate && !op1.Negate && op1.Operand->IsProduct())
		{
			const auto& input_a = op0.Operand;

			auto* product = dynamic_cast<CCombinerProduct*>(op1.Operand.get());
			if (product && product->GetNumOperands() == 2)
			{
				auto& factor = product->GetOperand(0);
				auto& diff = product->GetOperand(1);

				if (diff->IsSum())
				{
					auto* diff_sum = dynamic_cast<CCombinerSum*>(diff.get());
					if (diff_sum && diff_sum->mOperands.size() == 2)
					{
						const auto& a = diff_sum->mOperands[0];
						const auto& b = diff_sum->mOperands[1];

						if (!a.Negate && b.Negate && b.Operand->IsEqual(*input_a))
						{
							const auto& input_b = a.Operand;
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


std::unique_ptr<CCombinerOperand> CCombinerSum::SimplifyAndReduce() const
{
	// If we consist of a single non-negated element, just return its simplified version
	if (mOperands.size() == 1 && !mOperands[0].Negate)
	{
		return mOperands[0].Operand->SimplifyAndReduce();
	}

	if (auto blend = ReduceToBlend(); blend != nullptr)
	{
		return blend;
	}

	auto new_add = std::make_unique<CCombinerSum>();

	for (const auto& node : mOperands)
	{
		auto simplified = node.Operand->SimplifyAndReduce();

		if (node.Negate)
			new_add->Sub(std::move(simplified));
		else
			new_add->Add(std::move(simplified));
	}

	new_add->SortOperands();

	return new_add;
}


COutputStream& CCombinerSum::Stream(COutputStream& stream) const
{
	stream << "( ";

	for (size_t i = 0; i < mOperands.size(); ++i)
	{
		const auto& node = mOperands[i];

		if (i != 0)
		{
			stream << (node.Negate ? " - " : " + ");
		}
		else if (node.Negate)
		{
			stream << "-";
		}

		node.Operand->Stream(stream);
	}

	stream << " )";
	return stream;
}

CCombinerProduct::CCombinerProduct()
:	CCombinerOperand( CT_PRODUCT )
{

}


CCombinerProduct::CCombinerProduct( std::unique_ptr<CCombinerOperand> operand )
:	CCombinerOperand( CT_PRODUCT )
{
	if(operand)
	{
		Mul( std::move(operand) );
	}
}


CCombinerProduct::CCombinerProduct(const CCombinerProduct& rhs)
: CCombinerOperand(CT_PRODUCT)
{
	mOperands.reserve(rhs.mOperands.size()); // Optional, for efficiency

	for (const auto& node : rhs.mOperands)
	{
		mOperands.emplace_back(node.Operand->Clone());
	}
}
CCombinerProduct::~CCombinerProduct()
{
	Clear();
}


void CCombinerProduct::Clear()
{
	mOperands.clear();
}

int CCombinerProduct::Compare(const CCombinerOperand& other) const
{
	int type_diff = GetType() - other.GetType();
	if (type_diff != 0)
		return type_diff;

	const auto* rhs = dynamic_cast<const CCombinerProduct*>(&other);
	DAEDALUS_ASSERT(rhs != nullptr, "Invalid cast in CCombinerProduct::Compare");

	int size_diff = static_cast<int>(mOperands.size()) - static_cast<int>(rhs->mOperands.size());
	if (size_diff != 0)
		return size_diff;

	for (size_t i = 0; i < mOperands.size(); ++i)
	{
		int diff = mOperands[i].Operand->Compare(*rhs->mOperands[i].Operand);
		if (diff != 0)
			return diff;
	}

	return 0;
}
void CCombinerProduct::Mul(std::unique_ptr<CCombinerOperand> operand)
{
	if (operand->IsInput(CI_0))
	{
		Clear();
		mOperands.emplace_back(std::move(operand));
	}
	else if (operand->IsInput(CI_1))
	{
		// Identity — do nothing
		return;
	}
	else if (operand->IsProduct())
	{
		CCombinerProduct* product = dynamic_cast<CCombinerProduct*>(operand.get());
		DAEDALUS_ASSERT(product != nullptr, "Operand claims to be a product, but cast failed!");

		for (auto& sub : product->mOperands)
		{
			Mul(sub.Operand->SimplifyAndReduce());
		}
	}
	else
	{
		mOperands.emplace_back(std::move(operand));
	}
}
std::unique_ptr<CCombinerOperand> CCombinerProduct::SimplifyAndReduce() const
{
	// If we consist of a single element, just return its simplified result
	if (mOperands.size() == 1)
	{
		return mOperands[0].Operand->SimplifyAndReduce();
	}

	auto new_mul = std::make_unique<CCombinerProduct>();

	for (const auto& node : mOperands)
	{
		new_mul->Mul(node.Operand->SimplifyAndReduce());
	}

	new_mul->SortOperands();

	return new_mul;
}

COutputStream& CCombinerProduct::Stream(COutputStream& stream) const
{
	stream << "( ";

	for (size_t i = 0; i < mOperands.size(); ++i)
	{
		if (i != 0)
			stream << " * ";

		mOperands[i].Operand->Stream(stream);
	}

	stream << " )";
	return stream;
}