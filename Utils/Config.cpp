#include "Config.hpp"
#include "../Utils/base64.h"
#include "../Libraries/json.h"
#include <fstream>
#include <iomanip>
#include "../SDK/CVariables.hpp"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

#include <shlobj.h>
#include <sstream>

BOOL ConfigManager::DirectoryExists( LPCTSTR szPath ) {
	DWORD dwAttrib = GetFileAttributes( szPath );

	return ( dwAttrib != INVALID_FILE_ATTRIBUTES &&
		( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
}

std::vector<std::string> ConfigManager::GetConfigs( ) {
	static bool created_config = true;
	if( created_config ) {
		namespace fs = std::experimental::filesystem;
		fs::path full_path( fs::current_path( ) );
		std::wstring str = full_path.wstring( ) + XorStr( L"\\immortal" );

		CreateDirectoryW( str.c_str( ), nullptr );
		str += XorStr( L"\\config" );
		CreateDirectoryW( str.c_str( ), nullptr );

		created_config = false;
	}

	std::string config_extension = XorStr( ".imm" );
	std::vector<std::string> names;

	WIN32_FIND_DATAA find_data;
	HANDLE preset_file = FindFirstFileA( ( XorStr( "immortal\\config\\*" ) + config_extension ).c_str( ), &find_data );

	// the following will need to be redone or replaced!
	if( preset_file != INVALID_HANDLE_VALUE ) {
		do {
			if( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				continue;

			std::string s = find_data.cFileName;
			int pos = s.find( XorStr( ".imm" ) );

			// so this line WILL cause random crashes on inject, very nice!
			// removed for testing.
			//s.erase( s.begin( ) + pos, s.begin( ) + pos + 4 );

			names.push_back( s.c_str( ) );
		} while( FindNextFileA( preset_file, &find_data ) );

		FindClose( preset_file );
	}

	return names;
}

void ConfigManager::LoadConfig( std::string configname ) {
	std::ifstream input_file = std::ifstream( ( XorStr( "immortal\\config\\" ) + configname /*+ XorStr( ".imm" )*/ ).c_str( ) );
	if( !input_file.good( ) )
		return;

	try {
		std::stringstream content{ };

		content << input_file.rdbuf( );

		if( content.str( ).empty( ) )
			return;

		auto decoded_string = base64::decode( content.str( ) );
		// this a vader config nigga?
		if( decoded_string[ 0 ] != '[' ||
			decoded_string[ 1 ] != 's' ||
			decoded_string[ 2 ] != 'o' ||
			decoded_string[ 3 ] != 'n' ||
			decoded_string[ 4 ] != 't' ||
			decoded_string[ 5 ] != ']' ||
			decoded_string[ 6 ] != ' ' ||
			decoded_string[ 7 ] != '-' ||
			decoded_string[ 8 ] != ' ' )
			return;

		// yes it is nigga
		auto parsed_config = nlohmann::json::parse( decoded_string.erase( 0, 8 ) );

		g_Vars.m_json = parsed_config;
		input_file.close( );
	}
	catch( ... ) {
		input_file.close( );
		return;
	}

	for( auto& child : g_Vars.m_children ) {
		child->Load( g_Vars.m_json[ child->GetName( ) ] );
	}
}

void ConfigManager::SaveConfig( std::string configname ) {
	std::ofstream o( ( XorStr( "immortal\\config\\" ) + configname /*+ XorStr( ".imm" )*/ ).c_str( ) );
	if( !o.is_open( ) )
		return;

	g_Vars.m_json.clear( );
	for( auto& child : g_Vars.m_children ) {
		child->Save( );

		auto json = child->GetJson( );
		g_Vars.m_json[ child->GetName( ) ] = ( json );
	}

	o.clear( );
	auto str = base64::encode( ( std::string( XorStr( "[sont] - " ) ).append( g_Vars.m_json.dump( -1, '~', true ) ) ).c_str( ) );
	o << str;
	o.close( );

	g_Vars.m_json.clear( );
}

void ConfigManager::RemoveConfig( std::string configname ) {
	std::remove( ( XorStr( "immortal\\config\\" ) + configname /*+ XorStr( ".imm" )*/ ).c_str( ) );
}

void ConfigManager::CreateConfig( std::string configname ) {
	std::ofstream o( ( XorStr( "immortal\\config\\" ) + configname /*+ XorStr( ".imm" )*/ ).c_str( ) );
}

void ConfigManager::ResetConfig( ) {
	for( auto& child : g_Vars.m_children ) {
		child->Load( g_Vars.m_json_default_cfg[ child->GetName( ) ] );
	}
}

void ConfigManager::OpenConfigFolder( ) {
	namespace fs = std::experimental::filesystem;
	fs::path full_path( fs::current_path( ) );

	std::wstring str = full_path.wstring( ) + XorStr( L"\\immortal\\config" );

	PIDLIST_ABSOLUTE pidl;
	if( SUCCEEDED( SHParseDisplayName( str.c_str( ), 0, &pidl, 0, 0 ) ) ) {
		// we don't want to actually select anything in the folder, so we pass an empty
		// PIDL in the array. if you want to select one or more items in the opened
		// folder you'd need to build the PIDL array appropriately
		ITEMIDLIST idNull = { 0 };
		LPCITEMIDLIST pidlNull[ 1 ] = { &idNull };
		SHOpenFolderAndSelectItems( pidl, 1, pidlNull, 0 );

		// LIFEEEEHAAAACK BITCH!!! (◣_◢)
		using ILFree_t = void( __stdcall* )( LPITEMIDLIST );
		static ILFree_t ILFree_fn = ( ILFree_t ) GetProcAddress( GetModuleHandleA( XorStr( "SHELL32" ) ), XorStr( "ILFree" ) );
		ILFree_fn( pidl );
	}
}