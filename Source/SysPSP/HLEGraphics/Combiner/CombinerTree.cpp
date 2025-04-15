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
#include "SysPSP/HLEGraphics/Combiner/BlendConstant.h"
#include "SysPSP/HLEGraphics/Combiner/CombinerTree.h"
#include "SysPSP/HLEGraphics/Combiner/CombinerExpression.h"
#include "SysPSP/HLEGraphics/Combiner/RenderSettings.h"

#include "HLEGraphics/RDP.h"
#include "Utility/Stream.h"


namespace
{

const ECombinerInput	CombinerInput32[ 32 ] =
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_COMBINED_ALPHA,
	CI_TEXEL0_ALPHA,	CI_TEXEL1_ALPHA,	CI_PRIMITIVE_ALPHA,	CI_SHADE_ALPHA,	CI_ENV_ALPHA,		CI_LOD_FRACTION,	CI_PRIM_LOD_FRACTION,	CI_K5,

	CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,		CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,				CI_UNKNOWN,
	CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,		CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,				CI_0,
};

const ECombinerInput	CombinerInput16[ 16 ] =
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_COMBINED_ALPHA,
	CI_TEXEL0_ALPHA,	CI_TEXEL1_ALPHA,	CI_PRIMITIVE_ALPHA,	CI_SHADE_ALPHA,	CI_ENV_ALPHA,		CI_LOD_FRACTION,	CI_PRIM_LOD_FRACTION,	CI_0,
};

const ECombinerInput	CombinerInput8[ 8 ] =
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_0,
};

const ECombinerInput	CombinerInputAlphaC1_8[ 8 ] =
{
	CI_LOD_FRACTION,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_0,
};

const ECombinerInput	CombinerInputAlphaC2_8[ 8 ] =
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_0,
};


enum EBuildConstantExpressionOptions
{
	BCE_ALLOW_SHADE,
	BCE_DISALLOW_SHADE,
};

static const CBlendConstantExpression* BuildConstantExpression(const CCombinerOperand* operand, EBuildConstantExpressionOptions options)
{
	if (operand->IsInput())
	{
		auto input = dynamic_cast<const CCombinerInput*>(operand);
		if (!input) return nullptr;

		switch (input->GetInput())
		{
		case CI_PRIMITIVE:         return new CBlendConstantExpressionValue(BC_PRIMITIVE);
		case CI_ENV:               return new CBlendConstantExpressionValue(BC_ENVIRONMENT);
		case CI_PRIMITIVE_ALPHA:   return new CBlendConstantExpressionValue(BC_PRIMITIVE_ALPHA);
		case CI_ENV_ALPHA:         return new CBlendConstantExpressionValue(BC_ENVIRONMENT_ALPHA);
		case CI_1:                 return new CBlendConstantExpressionValue(BC_1);
		case CI_0:                 return new CBlendConstantExpressionValue(BC_0);
		case CI_SHADE:
			if (options == BCE_ALLOW_SHADE)
				return new CBlendConstantExpressionValue(BC_SHADE);
			else
				return nullptr;
		default:
			return nullptr;
		}
	}
	else if (operand->IsSum())
	{
		const CCombinerSum* sum = dynamic_cast<const CCombinerSum*>(operand);
		if (!sum) return nullptr;

		const CBlendConstantExpression* sum_expr = nullptr;

		for (u32 i = 0; i < sum->GetNumOperands(); ++i)
		{
			const std::unique_ptr<CCombinerOperand>& sum_term = sum->GetOperand(i);
			const CBlendConstantExpression* lhs = sum_expr;
			const CBlendConstantExpression* rhs = BuildConstantExpression(sum_term.get(), options);

			if (!rhs)
			{
				delete sum_expr;
				return nullptr;
			}

			if (sum->IsTermNegated(i))
			{
				if (!lhs)
					lhs = new CBlendConstantExpressionValue(BC_0);

				sum_expr = new CBlendConstantExpressionSub(lhs, rhs);
			}
			else
			{
				sum_expr = lhs ? new CBlendConstantExpressionAdd(lhs, rhs) : rhs;
			}
		}

		return sum_expr;
	}
	else if (operand->IsProduct())
	{
		const CCombinerProduct* product = dynamic_cast<const CCombinerProduct*>(operand);
		if (!product) return nullptr;

		const CBlendConstantExpression* product_expr = nullptr;

		for (u32 i = 0; i < product->GetNumOperands(); ++i)
		{
			const std::unique_ptr<CCombinerOperand>& product_term = product->GetOperand(i);
			const CBlendConstantExpression* lhs = product_expr;
			const CBlendConstantExpression* rhs = BuildConstantExpression(product_term.get(), options);

			if (!rhs)
			{
				delete product_expr;
				return nullptr;
			}

			product_expr = lhs ? new CBlendConstantExpressionMul(lhs, rhs) : rhs;
		}

		return product_expr;
	}

	return nullptr;
}

}

CCombinerTree::CCombinerTree(u64 mux, bool two_cycles)
	: mMux(mux)
	, mCycle1(nullptr)
	, mCycle1A(nullptr)
	, mCycle2(nullptr)
	, mCycle2A(nullptr)
{
	RDP_Combine m;
	m.mux = mux;

	// First cycle
	mCycle1  = BuildCycle1(
		CombinerInput16[m.aRGB0],
		CombinerInput16[m.bRGB0],
		CombinerInput32[m.cRGB0],
		CombinerInput8[m.dRGB0]
	);

	mCycle1A = BuildCycle1(
		CombinerInputAlphaC1_8[m.aA0],
		CombinerInputAlphaC1_8[m.bA0],
		CombinerInputAlphaC1_8[m.cA0],
		CombinerInputAlphaC1_8[m.dA0]
	);

	// Optional second cycle
	if (two_cycles)
	{
		mCycle2 = BuildCycle2(
			CombinerInput16[m.aRGB1],
			CombinerInput16[m.bRGB1],
			CombinerInput32[m.cRGB1],
			CombinerInput8[m.dRGB1],
			mCycle1
		);

		mCycle2A = BuildCycle2(
			CombinerInputAlphaC2_8[m.aA1],
			CombinerInputAlphaC2_8[m.bA1],
			CombinerInputAlphaC2_8[m.cA1],
			CombinerInputAlphaC2_8[m.dA1],
			mCycle1A
		);

		mBlendStates = GenerateBlendStates(mCycle2, mCycle2A);
	}
	else
	{
		mBlendStates = GenerateBlendStates(mCycle1, mCycle1A);
	}
}


CCombinerTree::~CCombinerTree()
{
}


std::unique_ptr<CCombinerOperand> CCombinerTree::BuildCycle1(ECombinerInput a, ECombinerInput b, ECombinerInput c, ECombinerInput d)
{
	auto input_a = std::make_unique<CCombinerInput>(a);
	auto input_b = std::make_unique<CCombinerInput>(b);
	auto input_c = std::make_unique<CCombinerInput>(c);
	auto input_d = std::make_unique<CCombinerInput>(d);

	return Build(std::move(input_a), std::move(input_b), std::move(input_c), std::move(input_d));
}


std::unique_ptr<CCombinerOperand> CCombinerTree::BuildCycle2(
	ECombinerInput a, ECombinerInput b, ECombinerInput c, ECombinerInput d,
	const std::unique_ptr<CCombinerOperand>& cycle_1_output
)
{
	auto input_a = (a == CI_COMBINED) ? cycle_1_output->Clone() : std::make_unique<CCombinerInput>(a);
	auto input_b = (b == CI_COMBINED) ? cycle_1_output->Clone() : std::make_unique<CCombinerInput>(b);
	auto input_c = (c == CI_COMBINED) ? cycle_1_output->Clone() : std::make_unique<CCombinerInput>(c);
	auto input_d = (d == CI_COMBINED) ? cycle_1_output->Clone() : std::make_unique<CCombinerInput>(d);

	return Build(std::move(input_a), std::move(input_b), std::move(input_c), std::move(input_d));
}
//*****************************************************************************
//	Build an expression of the form output = (A-B)*C + D, and simplify.
//*****************************************************************************
std::unique_ptr<CCombinerOperand> CCombinerTree::Build(
	std::unique_ptr<CCombinerOperand> a,
	std::unique_ptr<CCombinerOperand> b,
	std::unique_ptr<CCombinerOperand> c,
	std::unique_ptr<CCombinerOperand> d)
{
	auto sum = std::make_unique<CCombinerSum>();
	sum->Add(std::move(a));
	sum->Sub(std::move(b));

	auto product = std::make_unique<CCombinerProduct>(std::unique_ptr<CCombinerOperand>(std::move(sum)));
	product->Mul(std::move(c));
	auto output = std::make_unique<CCombinerSum>(std::unique_ptr<CCombinerOperand>(std::move(product)));
	output->Add(std::move(d));

	return Simplify(static_cast<std::unique_ptr<CCombinerOperand>>(std::move(output)));
}


COutputStream &	CCombinerTree::Stream( COutputStream & stream ) const
{
	stream << "RGB:   "; mCycle2->Stream( stream ); stream << "\n";
	stream << "Alpha: "; mCycle2A->Stream( stream ); stream << "\n";
	return stream;
}


CBlendStates *	CCombinerTree::GenerateBlendStates( const std::unique_ptr<CCombinerOperand>& colour_operand, const std::unique_ptr<CCombinerOperand>& alpha_operand ) const
{
	auto states =  new CBlendStates;

	states->SetAlphaSettings( GenerateAlphaRenderSettings( alpha_operand ) );

	GenerateRenderSettings( states, colour_operand );

	return states;
}



namespace
{
void	ApplyAlphaModulateTerm( CAlphaRenderSettings * settings, const std::unique_ptr<CCombinerOperand>& operand )
{
	if( operand->IsInput() )
	{
		const CCombinerInput* input = dynamic_cast<const CCombinerInput*>(operand.get());;

		switch( input->GetInput() )
		{
		case CI_TEXEL0:
			settings->AddTermTexel0();
			break;
		case CI_TEXEL1:
			settings->AddTermTexel1();
			break;
		case CI_SHADE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_SHADE ) );
			break;
		case CI_PRIMITIVE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE ) );
			break;
		case CI_ENV:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT ) );
			break;
		case CI_PRIMITIVE_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE_ALPHA ) );
			break;
		case CI_ENV_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT_ALPHA ) );
			break;
		case CI_0:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_0 ) );
			break;
		case CI_1:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_1 ) );
			break;

		default:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			printf( "Unhandled Alpha Input: %s\n", GetCombinerInputName( input->GetInput() ) );
#endif
			settings->SetInexact();
			break;
		}
	}
	else
	{
		//
		//	Try to reduce to a constant term, and add that
		//
		const CBlendConstantExpression *	constant_expression( BuildConstantExpression( operand.get(), BCE_ALLOW_SHADE ) );
		if( constant_expression != NULL )
		{
			settings->AddTermConstant( constant_expression );
		}
		else
		{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			COutputStringStream	str;
			operand->Stream( str );
			printf( "\n********************************\n" );
			printf( "Unhandled alpha - not a simple term: %s\n", str.c_str() );
			printf( "********************************\n\n" );
#endif
			settings->SetInexact();
		}
	}
}


}


CAlphaRenderSettings * CCombinerTree::GenerateAlphaRenderSettings( const std::unique_ptr<CCombinerOperand>& operand ) const
{
	COutputStringStream			str;
	operand->Stream( str );

	auto settings =  new CAlphaRenderSettings( str.c_str() );

	if(operand->IsProduct())
	{
		const CCombinerProduct* product = dynamic_cast<const CCombinerProduct*>(operand.get());
		for( u32 i = 0; i < product->GetNumOperands(); ++i )
		{
			ApplyAlphaModulateTerm( settings, product->GetOperand( i ) );
		}
	}
	else
	{
		ApplyAlphaModulateTerm( settings, operand );
	}

	settings->Finalise();

	return settings;
}


namespace
{
void	ApplyModulateTerm( CRenderSettingsModulate * settings, const std::unique_ptr<CCombinerOperand>& operand )
{
	if( operand->IsInput() )
	{
		const CCombinerInput* input = dynamic_cast<const CCombinerInput*>(operand.get());

		switch( input->GetInput() )
		{
		case CI_TEXEL0:
			settings->AddTermTexel0();
			break;
		case CI_TEXEL1:
			settings->AddTermTexel1();
			break;
		case CI_SHADE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_SHADE ) );
			break;
		case CI_PRIMITIVE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE ) );
			break;
		case CI_ENV:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT ) );
			break;
		case CI_PRIMITIVE_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE_ALPHA ) );
			break;
		case CI_ENV_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT_ALPHA ) );
			break;
		case CI_0:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_0 ) );
			break;
		case CI_1:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_1 ) );
			break;

		default:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			printf( "Unhandled Input: %s\n", GetCombinerInputName( input->GetInput() ) );
#endif
			settings->SetInexact();
			break;
		}
	}
	else
	{
		//
		//	Try to reduce to a constant term, and add that
		//
		const auto constant_expression = BuildConstantExpression( operand.get(), BCE_ALLOW_SHADE );
		if( constant_expression != NULL )
		{
			settings->AddTermConstant( constant_expression );
		}
		else
		{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			COutputStringStream	str;
			operand->Stream( str );
			printf( "\n********************************\n" );
			printf( "Unhandled rgb - not a simple term: %s\n", str.c_str() );
			printf( "********************************\n\n" );
#endif
			settings->SetInexact();
		}
	}
}


}

void	CCombinerTree::GenerateRenderSettings( CBlendStates * states, const std::unique_ptr<CCombinerOperand>& operand ) const
{
	if(operand->IsInput())
	{
		COutputStringStream	str;
		operand->Stream( str );

		auto settings =  new CRenderSettingsModulate( str.c_str() );

		ApplyModulateTerm( settings, operand );

		settings->Finalise();

		states->AddColourSettings( settings );
	}
	else if(operand->IsSum())
	{
		const CCombinerSum* sum = dynamic_cast<const CCombinerSum*>(operand.get());

		for( u32 i = 0; i < sum->GetNumOperands(); ++i )
		{
			const auto&	sum_term = sum->GetOperand( i );

			// Recurse
			if( sum->IsTermNegated( i ) )
			{
				printf( "Negative term!!\n" );
				COutputStringStream	str;
				str << "- ";
				sum->Stream( str );
				states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
			}
			else
			{
				GenerateRenderSettings( states, sum_term );
			}
		}
	}
	else if(operand->IsProduct())
	{
		const auto product = dynamic_cast<const CCombinerProduct*>(operand.get());

		COutputStringStream	str;
		product->Stream( str );

		auto settings =  new CRenderSettingsModulate( str.c_str() );

		for( u32 i = 0; i < product->GetNumOperands(); ++i )
		{
			ApplyModulateTerm( settings, product->GetOperand( i ) );
		}

		settings->Finalise();

		states->AddColourSettings( settings );
	}
	else if(operand->IsBlend())
	{
		const CCombinerBlend* blend = static_cast<const CCombinerBlend*>(operand.get());

		const CCombinerOperand* operand_a = blend->GetInputA();
		const CCombinerOperand* operand_b = blend->GetInputB();
		const CCombinerOperand* operand_f = blend->GetInputF();

		COutputStringStream	str;
		blend->Stream( str );
		bool	handled( false );

		if( operand_f->IsInput( CI_TEXEL0 ) )
		{
			
			const auto expr_a = BuildConstantExpression( operand_a, BCE_ALLOW_SHADE );
			const auto expr_b = BuildConstantExpression( operand_b, BCE_DISALLOW_SHADE );
			if( expr_a && expr_b)
			{
				states->AddColourSettings( new CRenderSettingsBlend( str.c_str(), expr_a, expr_b ) );
				handled = true;
			}
		}
		else
		{
			const auto expr_a = BuildConstantExpression( operand_a, BCE_ALLOW_SHADE );
			const auto expr_b = BuildConstantExpression( operand_b, BCE_ALLOW_SHADE );
			const auto expr_f = BuildConstantExpression( operand_f, BCE_ALLOW_SHADE );

			if( expr_a && expr_b && expr_f )
			{
				const auto expr_blend = new CBlendConstantExpressionBlend( expr_a, expr_b, expr_f );
				auto settings =  new CRenderSettingsModulate( str.c_str() );

				settings->AddTermConstant( expr_blend );
				settings->Finalise();
				states->AddColourSettings( settings );
				handled = true;
			}
		}

		if( !handled )
		{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			printf( "CANNOT BLEND!\n" );
#endif
			states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
		}

	}
	else
	{
		COutputStringStream	str;
		operand->Stream( str );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		printf( "\n********************************\n" );
		printf( "Unhandled - inner operand is not an input/product/sum: %s\n", str.c_str() );
		printf( "********************************\n\n" );
#endif
		states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
	}

}


std::unique_ptr<CCombinerOperand> CCombinerTree::Simplify( std::unique_ptr<CCombinerOperand> operand )
{
	bool	did_something;
	do
	{
		auto new_tree = operand->SimplifyAndReduce();

		did_something = !new_tree->IsEqual( *operand );
		
		operand = std::move(new_tree);
	}
	while( did_something );

	return operand;
}
