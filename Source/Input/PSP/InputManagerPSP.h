#pragma once

class IInputManager : public CInputManager
{
	public:
		IInputManager();
		virtual ~IInputManager();

		virtual bool						Initialise();
		virtual void						Finalise()					{}

		virtual void						GetState( OSContPad pPad[4] );

		virtual u32							GetNumConfigurations() const;
		virtual const char *				GetConfigurationName( u32 configuration_idx ) const;
		virtual const char *				GetConfigurationDescription( u32 configuration_idx ) const;
		virtual void						SetConfiguration( u32 configuration_idx );

		virtual u32							GetConfigurationFromName( const char * name ) const;

	private:
		void								SwapJoyStick(OSContPad *pPad, SceCtrlData *pad);
		void								LoadControllerConfigs( const std::filesystem::path& p_dir );
		CControllerConfig *					BuildDefaultConfig();
		CControllerConfig *					BuildControllerConfig( const std::filesystem::path& filename );

	private:
		CControllerConfig *					mpControllerConfig;
		std::vector<CControllerConfig*>		mControllerConfigs;
};
