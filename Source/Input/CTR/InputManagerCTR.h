#pragma once
#include "Input/InputManager.h"


class CButtonMapping
{
	public:
		virtual ~CButtonMapping() {}
		virtual bool	Evaluate( u32 in_buttons ) const = 0;
};

class CButtonMappingMask : public CButtonMapping
{
	public:
		CButtonMappingMask( u32 mask ) : mMask( mask ) {}

		virtual bool	Evaluate( u32 in_buttons ) const		{ return ( in_buttons & mMask ) != 0; }

	private:
		u32					mMask;
};

class CButtonMappingNegate : public CButtonMapping
{
	public:
		CButtonMappingNegate( CButtonMapping * p_mapping )
			:	mpMapping( p_mapping )
		{
		}

		~CButtonMappingNegate()
		{
			delete mpMapping;
		}

		virtual bool	Evaluate( u32 in_buttons ) const
		{
			return !mpMapping->Evaluate( in_buttons );
		}

	private:
		CButtonMapping *	mpMapping;
};

class CButtonMappingAnd : public CButtonMapping
{
	public:
		CButtonMappingAnd( CButtonMapping * p_a, CButtonMapping * p_b )
			:	mpMappingA( p_a )
			,	mpMappingB( p_b )
		{
		}

		~CButtonMappingAnd()
		{
			delete mpMappingA;
			delete mpMappingB;
		}

		virtual bool	Evaluate( u32 in_buttons ) const
		{
			return mpMappingA->Evaluate( in_buttons ) &&
				   mpMappingB->Evaluate( in_buttons );
		}

	private:
		CButtonMapping *	mpMappingA;
		CButtonMapping *	mpMappingB;
};


class CButtonMappingOr : public CButtonMapping
{
	public:
		CButtonMappingOr( CButtonMapping * p_a, CButtonMapping * p_b )
			:	mpMappingA( p_a )
			,	mpMappingB( p_b )
		{
		}

		~CButtonMappingOr()
		{
			delete mpMappingA;
			delete mpMappingB;
		}

		virtual bool	Evaluate( u32 in_buttons ) const
		{
			return mpMappingA->Evaluate( in_buttons ) ||
				   mpMappingB->Evaluate( in_buttons );
		}

	private:
		CButtonMapping *	mpMappingA;
		CButtonMapping *	mpMappingB;
};

enum EN64Button
{
	N64Button_A = 0,
	N64Button_B,
	N64Button_Z,
	N64Button_L,
	N64Button_R,
	N64Button_Up,
	N64Button_Down,
	N64Button_Left,
	N64Button_Right,
	N64Button_CUp,
	N64Button_CDown,
	N64Button_CLeft,
	N64Button_CRight,
	N64Button_Start,
	NUM_N64_BUTTONS
};

const u32 gN64ButtonMasks[NUM_N64_BUTTONS] =
{
	A_BUTTON,		//N64Button_A = 0,
	B_BUTTON,		//N64Button_B,
	Z_TRIG,			//N64Button_Z,
	L_TRIG,			//N64Button_L,
	R_TRIG,			//N64Button_R,
	U_JPAD,			//N64Button_Up,
	D_JPAD,			//N64Button_Down,
	L_JPAD,			//N64Button_Left,
	R_JPAD,			//N64Button_Right,
	U_CBUTTONS,		//N64Button_CUp,
	D_CBUTTONS,		//N64Button_CDown,
	L_CBUTTONS,		//N64Button_CLeft,
	R_CBUTTONS,		//N64Button_CRight,
	START_BUTTON	//N64Button_Start,
};

const char * const gN64ButtonNames[NUM_N64_BUTTONS] =
{
	"N64.A",			//N64Button_A = 0,
	"N64.B",			//N64Button_B,
	"N64.Z",			//N64Button_Z,
	"N64.LTrigger",		//N64Button_L,
	"N64.RTrigger",		//N64Button_R,
	"N64.Up",			//N64Button_Up,
	"N64.Down",			//N64Button_Down,
	"N64.Left",			//N64Button_Left,
	"N64.Right",		//N64Button_Right,
	"N64.CUp",			//N64Button_CUp,
	"N64.CDown",		//N64Button_CDown,
	"N64.CLeft",		//N64Button_CLeft,
	"N64.CRight",		//N64Button_CRight,
	"N64.Start"			//N64Button_Start,
};
class CControllerConfig
{
	public:
		CControllerConfig();
		~CControllerConfig();

	public:
		void				SetName( const char * name )		{ mName = name; }
		void				SetDescription( const char * desc )	{ mDescription = desc; }

		const char *		GetName() const						{ return mName.c_str(); }
		const char *		GetDescription() const				{ return mDescription.c_str(); }

		void				SetButtonMapping( EN64Button button, CButtonMapping * p_mapping );
		u32					GetN64ButtonsState( u32 psp_button_mask ) const;

	private:
		std::string			mName;
		std::string			mDescription;

		CButtonMapping *	mButtonMappings[ NUM_N64_BUTTONS ];
};


class IInputManager : public CInputManager
{
	public:
		IInputManager();
		virtual ~IInputManager();

		virtual bool				Initialise();
		virtual void				Finalise();

		virtual void				GetState( OSContPad pPad[4] );

		virtual u32					GetNumConfigurations() const;
		virtual const char *		GetConfigurationName( u32 configuration_idx ) const;
		virtual const char *		GetConfigurationDescription( u32 configuration_idx ) const;
		virtual void				SetConfiguration( u32 configuration_idx );
		virtual u32					GetConfigurationFromName( const char * name ) const;

	private:
		void								LoadControllerConfigs( const std::filesystem::path p_dir );
		CControllerConfig *					BuildDefaultConfig(bool ZSwap = false);
		CControllerConfig *					BuildControllerConfig( const char * filename );

		CControllerConfig *					mpControllerConfig;
		std::vector<CControllerConfig*>		mControllerConfigs;
};

