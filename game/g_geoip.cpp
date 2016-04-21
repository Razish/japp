#include "g_local.h"
#include <unordered_map>
#include <future>
#include <mutex>
extern "C" {
	#include "maxmind/maxminddb.h"
}
namespace GeoIP {
	static MMDB_s *handle;

#ifdef _WIN32
	// temporary files - store them in a circular buffer.
	// We're pretty much guaranteed to not need more than 8 temp files at a time.
	static int		fs_temporaryFileWriteIdx = 0;
	static char		fs_temporaryFileNames[8][MAX_OSPATH];
#endif

	// caller must free() tempFilePath is return value is true
	// extension is only used on linux and mac but should be in the form ".dat"
	bool WriteToTemporaryFile( const void *data, size_t dataLength, char **tempFilePath, const char *extension ) {
	#if defined(_WIN32)
		DWORD error;
		TCHAR tempPath[MAX_PATH];
		DWORD tempPathResult = GetTempPath( MAX_PATH, tempPath );
		if ( tempPathResult ) {
			TCHAR tempFileName[MAX_PATH];
			UINT tempFileNameResult = GetTempFileName( tempPath, "maxmind", 0, tempFileName );
			if ( tempFileNameResult ) {
				HANDLE file = CreateFile(
					tempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0
				);
				if ( file != INVALID_HANDLE_VALUE ) {
					DWORD bytesWritten = 0;
					if ( WriteFile( file, data, dataLength, &bytesWritten, NULL ) ) {
						int deletesRemaining = ARRAY_LEN( fs_temporaryFileNames );

						CloseHandle( file );

						while ( deletesRemaining > 0 && fs_temporaryFileNames[fs_temporaryFileWriteIdx][0] != '\0' ) {
							// Delete old temporary file as we need to
							if ( DeleteFile( fs_temporaryFileNames[fs_temporaryFileWriteIdx] ) ) {
								break;
							}

							error = GetLastError();
							trap->Print(
								"%s failed for '%s'. Win32 error code: 0x%08x\n",
								JAPP_FUNCTION, fs_temporaryFileNames[fs_temporaryFileWriteIdx], error
							);

							// Failed to delete, possibly because DLL was still in use. This can
							// happen when running a listen server and you continually restart
							// the map. The game DLL is reloaded, but cgame and ui DLLs are not.
							fs_temporaryFileWriteIdx =
								(fs_temporaryFileWriteIdx + 1) % ARRAY_LEN(fs_temporaryFileNames);
							deletesRemaining--;
						}

						// If this happened, then all slots are used and we some how have 8 DLLs
						// loaded at once?!
						assert( deletesRemaining > 0 );

						Q_strncpyz( fs_temporaryFileNames[fs_temporaryFileWriteIdx],
									tempFileName, sizeof(fs_temporaryFileNames[0]) );
						fs_temporaryFileWriteIdx =
							(fs_temporaryFileWriteIdx + 1) % ARRAY_LEN(fs_temporaryFileNames);

						if ( tempFilePath ) {
							size_t fileNameLen = strlen( tempFileName );
							*tempFilePath = (char *)malloc( fileNameLen + 1 );
							Q_strncpyz( *tempFilePath, tempFileName, fileNameLen + 1 );
						}

						return true;
					}
					else {
						error = GetLastError();
						trap->Print( "%s failed to write '%s'. Win32 error code: 0x%08x\n",
							JAPP_FUNCTION, tempFileName, error
						);
					}
				}
				else {
					error = GetLastError();
					trap->Print( "%s failed to create '%s'. Win32 error code: 0x%08x\n",
						JAPP_FUNCTION, tempFileName, error
					);
				}
			}
			else {
				error = GetLastError();
				trap->Print( "%s failed to generate temporary file name. Win32 error code: 0x%08x\n",
					JAPP_FUNCTION, error
				);
			}
		}
		else
		{
			error = GetLastError();
			trap->Print( "%s failed to get temporary file folder. Win32 error code: 0x%08x\n",
				JAPP_FUNCTION, error
			);
		}
	#elif defined(__linux__) || defined(MACOS_X)
		// grab the best path for the temporary file
	#if 0
		const char *tmpDir = getenv( "TMPDIR" );
		if ( !tmpDir || !tmpDir[0] ) {
			tmpDir = "/tmp";
		}
	#else
		const char *tmpDir = ".";
	#endif
		char absPath[MAX_OSPATH];
		Com_sprintf( absPath, sizeof(absPath), "%s/maxmind-XXXXXX%s", tmpDir, extension );

		// create the temporary file, this function will replace the Xs
		errno = 0;
		int filedes = mkstemps( absPath, strlen(extension) );
		unlink( absPath ); // so the temporary file is deleted when closed/program exits

		if ( filedes < 1 ) {
			trap->Print( "failed to create temporary file: %s\n", absPath, strerror( errno ) );
			return true;
		}
		else {
			trap->Print( "created temporary file: %s\n", absPath );
		}

		// write to the temporary file
		errno = 0;
		if ( write( filedes, data, dataLength ) == -1 ) {
			trap->Print( "failed to write to temporary file %s: %s\n", absPath, strerror( errno ) );
			return true;
		}

		if ( tempFilePath ) {
			size_t fileNameLen = strlen( absPath );
			*tempFilePath = (char *)malloc( fileNameLen + 1 );
			Q_strncpyz( *tempFilePath, absPath, fileNameLen + 1 );
		}
	#endif

		return false;
	}

	bool Init( void ) {
		if ( handle ) {
			return true;
		}
		handle = new MMDB_s{};
		int status = -1;

		const char *sPath = "GeoLite2-Country.mmdb";
		trap->Print( "Loading %s\n", sPath );
		fileHandle_t f = NULL_FILE;
		unsigned int len = trap->FS_Open( sPath, &f, FS_READ );

		// no file
		if ( !f ) {
			return false;
		}

		// empty file
		if ( !len || len == -1 ) {
			trap->FS_Close( f );
			return false;
		}

		// alloc memory for buffer
		char *buf = (char *)malloc( len + 1 );
		if ( !buf ) {
			return false;
		}

		trap->FS_Read( buf, len, f );
		trap->FS_Close( f );
		buf[len] = '\0';

		const char *extension = nullptr;
		for ( const char *p = sPath + strlen(sPath); p != sPath; p-- ) {
			if ( *p == '.' ) {
				extension = p;
				break;
			}
		}
		char *tmpFilePath = nullptr;
		if ( WriteToTemporaryFile( buf, len, &tmpFilePath, extension ) ) {
			trap->Print( "Failed to create temporary file\n" );
			free( buf );
			return false;
		}

		trap->Print( "loading from temporary file %s\n", tmpFilePath );
		status = MMDB_open( tmpFilePath, MMDB_MODE_MMAP, handle );
		if ( status != MMDB_SUCCESS ) {
			trap->Print( "Error occured while initialising MaxMind GeoIP: \"%s\"\n", MMDB_strerror( status ) );
			delete handle;
			handle = nullptr;

			free( tmpFilePath );
			free( buf );
			return false;
		}

		free( tmpFilePath );
		free( buf );
		return true;
	}

	void ShutDown( void ) {
		MMDB_close( handle );
		delete handle;
		handle = nullptr;
	}

	void Worker( GeoIPData *data ) {
		int error = -1, gai_error = -1;
		MMDB_lookup_result_s result =  MMDB_lookup_string( handle, data->getIp().c_str(), &gai_error, &error );
		if ( error != MMDB_SUCCESS || gai_error != 0 ) {
			std::string *str = data->getData();
			*str = MMDB_strerror( error );
			data->setStatus( 0 ); // error
			return;
		}
		if ( result.found_entry ) {
			MMDB_entry_s entry = result.entry;
			MMDB_entry_data_s entry_data;
			if ( (error = MMDB_get_value( &entry, &entry_data, "names", "en", NULL )) != MMDB_SUCCESS ) {
				std::string *str = data->getData();
				*str = MMDB_strerror( error );
				data->setStatus( 0 ); // error
				return;
			}
			if ( entry_data.has_data ) {
				std::string *str = data->getData();
				*str = entry_data.utf8_string;
				data->setStatus( 1 );
				return;
			}
			else {
				*data->getData() = "Unknown";
				data->setStatus( 1 );
				return;
			}
		}
		else {
			*data->getData() = "Unknown";
			data->setStatus( 1 );
			return;
		}
	}

	GeoIPData *GetIPInfo( const std::string ip ) {
		if ( handle ) {
			GeoIPData *data = new GeoIPData( ip );
			std::future<void> result = std::async( Worker, data );
			return data;
		}
		else {
			return nullptr;
		}
	}
}
