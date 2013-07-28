#include "ui/ui_local.h"
#include "ui_engine.h"

// cURL headers
#include <curl/curl.h>
#include <curl/curl_md5.h>
#include <curl/mprintf.h>

// cJSON headers
#include <json/cJSON.h>

// minizip headers
#define MAXFILENAME (256)
#include "minizip/unzip.h"

#ifdef WIN32
	#include <direct.h>
	#define USEWIN32IOAPI
	#include "minizip/iowin32.h"
#elif MAC_PORT
	#include "macosx/jp_mac.h"
#endif

#ifndef OPENJK

#pragma comment( lib, "zlib" )
#pragma comment( lib, "minizip" )
#pragma comment( lib, "cURL" )

static CURL *curl_handle;

typedef struct MemoryStruct_s {
	char *memory;
	size_t size;
} MemoryStruct_t;

static size_t JAPP_CURL_WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	MemoryStruct_t *mem = (MemoryStruct_t *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if ( !mem->memory )
		trap_Error("JA++ autoupdate failed: not enough memory (realloc:NULL)\n");

	memcpy( &(mem->memory[mem->size]), contents, realsize );
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

CURLcode JAPP_CURL_Fetch( const char *url, MemoryStruct_t *outChunk )
{
	CURLcode ret = CURLE_OK;
	curl_handle = curl_easy_init(); //init the curl session
	curl_easy_setopt(curl_handle, CURLOPT_URL, url); //specify URL to get
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, JAPP_CURL_WriteMemoryCallback); //send all data to this function
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, outChunk); //we pass our 'chunk' struct to the callback function
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0"); //some servers don't like requests that are made without a user-agent field, so we provide one
	ret = curl_easy_perform(curl_handle); //get it!
	curl_easy_cleanup(curl_handle); //cleanup curl stuff
	curl_handle = NULL;
	return ret;
}

/*
{
	"version" : 5,
	"versionlong" : "JA++ 0.3 build 1",
	"files" : [{
			"file" : "japp_assets.pk3",
			"md5" : "32d714c50b405722f170e993176bbb59",
			"path" : "0/3/b1/japp_assets.pk3"
		}, {
			"file" : "japp_bins.pk3",
			"md5" : "3201bd56f2b9693f4b2dc735ef852405",
			"path" : "0/3/b1/japp_bins.pk3"
		}, {
			"file" : "japp_animations.pk3",
			"md5" : "3c7b3a69f0a9a9ab30126b0c3db62725",
			"path" : "0/3/b1/japp_animations.pk3"
		}, {
			"file" : "japp_lua.pk3",
			"md5" : "54636be64e729ed6ed39ff57553d2ed0",
			"path" : "0/3/b1/japp_lua.pk3",
		}],
	"changelog" : "* Certain textures (weapon icons, etc) will be loaded at native resolution disregarding r_picmip\n- Removed clguntele. All similar commands will be implemented in Lua.\n* Any post-processing errors will disable the post-processing pass to avoid console spam.\n* Added a framework for rendering multiple views, akin to spectating multiple clients\n+ Unknown female skins now show as Jan instead of Kyle\n* Drastically improved post-processing. More options for bloom, faster rendering, on-the-fly reloading.\n[...] too many changes to list\n\n"
}
*/

#define UPDATER_TYPE 1
int do_extract(unzFile uf, int opt_extract_without_path, int opt_overwrite, const char *password);
void CheckFiles( char *jsonText )
{
	char *path = ENG_FS_BuildOSPath( ENG_Cvar_FindVar( "fs_basepath" )->string, ENG_Cvar_FindVar( "fs_game" )->string, "" );
	cJSON *root=NULL, *files=NULL;
	int latestVersion=1, currentVersion=0, filesCount=0;
	char versionlong[64]={0};
	char *changelog=NULL;//[2048]={0};
	const char *tmp=NULL;
	cvar_t *japp_updateURL = ENG_Cvar_FindVar( "japp_updateURL" );
	FILE *f = NULL;
	#if UPDATER_TYPE == 2
		qboolean updated = qfalse;
		int i=0;
		cvar_t *japp_update = ENG_Cvar_FindVar( "japp_update" );
	#endif

	root = cJSON_Parse( jsonText );
	if ( !root )
	{
		Com_Printf( "Error: Could not parse update info\n" );
		return;
	}

	latestVersion = cJSON_ToInteger( cJSON_GetObjectItem( root, "version" ) );
	if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( root, "versionlong" ) )) )
		Q_strncpyz( versionlong, tmp, sizeof( versionlong ) );
	#if 0
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( root, "changelog" ) )) )
			Q_strncpyz( changelog, tmp, sizeof( changelog ) );
	#else
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( root, "changelog" ) )) )
			changelog = strdup( tmp );
	#endif
	files = cJSON_GetObjectItem( root, "files" );
	filesCount = cJSON_GetArraySize( files );

	if ( (f=fopen( va( "%s/version.txt", path ), "r" )) )
	{//Read the current version number if it exists
		char buf[16]={0};
		currentVersion = atoi( fgets( buf, sizeof( buf ), f ) );
		fclose( f );
		f = NULL;
	}
	else
		currentVersion = 0; // initial state = outdated

	if ( latestVersion <= currentVersion )
	{
		Com_Printf( "^5Your JA++ version is up to date =D ("JAPP_CLIENT_VERSION_CLEAN")\n" );
		uiLocal.updateStatus = JAPP_UPDATE_UPTODATE;

		if ( changelog )
			free( changelog );
		cJSON_Delete( root );
		return;
	}

	#if UPDATER_TYPE == 1
		Com_Printf( "^5%s is now available. Please visit http://%s to download it\n", versionlong, japp_updateURL->string );
		Com_Printf( "^7Changelog:\n\n%s\n", changelog );
		uiLocal.updateStatus = JAPP_UPDATE_OUTDATED;

		if ( changelog )
			free( changelog );
		cJSON_Delete( root );
	#elif UPDATER_TYPE == 2 //old autoupdater, tends to fuck up when overwriting files that are loaded resulting in various fatal errors
		else if ( !japp_autoUpdate->integer )
		{
			Com_Printf( "^5%s is now available. Please visit http://%s or enable japp_autoUpdate to download it\n", versionlong, japp_autoUpdateURL->string );
			Com_Printf( "^7Changelog:\n\n%s\n", changelog );
			uiLocal.updateStatus = JAPP_UPDATE_OUTDATED;
			if ( changelog )
				free( changelog );
			return;
		}

		Com_Printf( "^5JA++ update available! ^7Upgrading from "JAPP_CLIENT_VERSION_CLEAN" to %s\n^7Changelog:\n\n%s\n", versionlong, changelog );
		for ( i=0; i<filesCount; i++ )
		{
			cJSON *item = cJSON_GetArrayItem( files, i );
			char itemPath[MAX_QPATH*4]={0}, itemMD5[33]={0}, itemFile[MAX_QPATH*4]={0}, itemExtra[64]={0};
			char currentFile[MAX_QPATH*4]={0}, remoteFile[MAX_QPATH*4]={0};
			qboolean unzip = qfalse;
			unsigned char digest[16]={0}, *contents=NULL;
			long len = 0;
			char digestAscii[33] = { 0 };
			int j;

			if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "file" ) )) )
				Q_strncpyz( itemFile, tmp, sizeof( itemFile ) );
			if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "md5" ) )) )
				Q_strncpyz( itemMD5, tmp, sizeof( itemMD5 ) );
			if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "path" ) )) )
				Q_strncpyz( itemPath, tmp, sizeof( itemPath ) );
			if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "extra" ) )) )
				Q_strncpyz( itemExtra, tmp, sizeof( itemExtra ) );

			if ( !strcmp( itemExtra, "unzip" ) )
				unzip = qtrue;

			Com_sprintf( currentFile, sizeof( currentFile ), "%s/%s", path, itemFile );
			Com_sprintf( remoteFile, sizeof( remoteFile ), "http://%s%s", japp_autoUpdateURL->string, itemPath );
			if ( (f=fopen( currentFile, "rb" ) ) )
			{//File exists, check MD5
				fseek( f, 0, SEEK_END ); len = ftell( f ); rewind( f ); // get length
				contents = (unsigned char *)malloc( len ); memset( contents, 0, len );
				fread( contents, 1, len, f );
				fclose( f );
				f = NULL;

				JAPP_md5it( &digest[0], contents, len );
				free( contents );

				for ( j=0; j<16; j++ )
					curl_msnprintf( (char *)&digestAscii[j*2], 3, "%02x", digest[j] );
				if ( strcmp( digestAscii, itemMD5 ) )
				{//MD5 mismatch, update!
					CURLcode ret = CURLE_OK;
					MemoryStruct_t chunk = { 0 };
					chunk.memory = (char *)malloc(1);  /* will be grown as needed */

					Com_Printf( "Updating %s\n", itemFile );
					#if 1
						if ( (ret=JAPP_CURL_Fetch( remoteFile, &chunk )) )
							Com_Printf( "Error updating %s: %s\n", itemFile, curl_easy_strerror( ret ) );

						if ( chunk.size && (f=fopen( currentFile, "wb+" )) )
						{
							fwrite( chunk.memory, 1, chunk.size, f );
							fclose( f );
							f = NULL;
							if ( unzip )
							{
							    char filename_try[MAXFILENAME+16] = "";
								unzFile uf=NULL;
								char oldWD[_MAX_PATH]={0};
								getcwd( oldWD, sizeof( oldWD ) );
								chdir( path );

								if (itemFile!=NULL)
								{
									#ifdef USEWIN32IOAPI
										zlib_filefunc_def ffunc;
									#endif
									strncpy(filename_try, itemFile,MAXFILENAME-1);
									/* strncpy doesnt append the trailing NULL, of the string is too long. */
									filename_try[ MAXFILENAME ] = '\0';

									#ifdef USEWIN32IOAPI
										fill_win32_filefunc(&ffunc);
										uf = unzOpen2(itemFile,&ffunc);
									#else
										uf = unzOpen(itemFile);
									#endif
									if (uf==NULL)
									{
										strcat(filename_try,".zip");
										uf = unzOpen(filename_try);
									}
								}

								if (uf==NULL)
								{
									Com_Printf("Cannot open %s or %s.zip\n",itemFile,itemFile);
									chdir( oldWD );
									continue;
								}
								Com_Printf("Extracting %s\n",filename_try);

								do_extract(uf, 0, 1, NULL);
								unzClose(uf);
								chdir( oldWD );
							}
						}
					#endif
					if ( chunk.memory )
						free( chunk.memory );
					updated = qtrue;
				}
			}
			else
			{//File doesn't exist, download!
				CURLcode ret = CURLE_OK;
				MemoryStruct_t chunk = { 0 };
				chunk.memory = (char *)malloc( 1 );  /* will be grown as needed */

				Com_Printf( "Downloading %s\n", itemFile );
				#if 1
					if ( (ret=JAPP_CURL_Fetch( remoteFile, &chunk )) )
						Com_Printf( "Error downloading %s: %s\n", itemFile, curl_easy_strerror( ret ) );

					if ( chunk.size && (f=fopen( currentFile, "wb+" )) )
					{
						fwrite( chunk.memory, 1, chunk.size, f );
						fclose( f );
						f = NULL;
						if ( unzip )
						{
						    char filename_try[MAXFILENAME+16] = "";
							unzFile uf=NULL;
							char oldWD[_MAX_PATH]={0};
							getcwd( oldWD, sizeof( oldWD ) );
							chdir( path );

							if (itemFile!=NULL)
							{
								#ifdef USEWIN32IOAPI
									zlib_filefunc_def ffunc;
								#endif
								strncpy(filename_try, itemFile,MAXFILENAME-1);
								/* strncpy doesnt append the trailing NULL, of the string is too long. */
								filename_try[ MAXFILENAME ] = '\0';

								#ifdef USEWIN32IOAPI
									fill_win32_filefunc(&ffunc);
									uf = unzOpen2(itemFile,&ffunc);
								#else
									uf = unzOpen(itemFile);
								#endif
								if (uf==NULL)
								{
									strcat(filename_try,".zip");
									uf = unzOpen(filename_try);
								}
							}

							if (uf==NULL)
							{
								Com_Printf("Cannot open %s or %s.zip\n",itemFile,itemFile);
								chdir( oldWD );
								continue;
							}
							Com_Printf("Extracting %s\n",filename_try);

							do_extract(uf, 0, 1, NULL);
							unzClose(uf);
							chdir( oldWD );
						}
					}
				#endif
				if ( chunk.memory )
					free( chunk.memory );
				updated = qtrue;
			}
		}

		if ( changelog )
			free( changelog );
		cJSON_Delete( root );

		if ( updated )
		{//Update our local version number
			uiLocal.updateStatus = JAPP_UPDATE_UPDATED;
			if ( (f=fopen( va( "%s/version.txt", path ), "w" )) )
			{
				fprintf( f, "%i", latestVersion );
				fclose( f );
				f = NULL;
			}
		}
	#endif
}

void UpdateJAPP( void )
{
	char remoteFile[MAX_QPATH]={0};
	cvar_t	*japp_update = ENG_Cvar_FindVar( "japp_update" ),
			*japp_updateURL = ENG_Cvar_FindVar( "japp_updateURL" );
	CURLcode ret = CURLE_OK;
	MemoryStruct_t chunk = { 0 };
	char buf[MAX_TOKEN_CHARS] = {0};
	trap_Argv( 0, buf, sizeof( buf ) );
	
	#if UPDATER_TYPE == 1
		if ( Q_stricmp( buf, "update" ) && !japp_update->integer )
			return;
	#endif

	chunk.memory = (char *)malloc(1);  /* will be grown as needed */
	Com_sprintf( remoteFile, sizeof( remoteFile ), "http://%supdate", japp_updateURL->string );

	if ( (ret = JAPP_CURL_Fetch( remoteFile, &chunk )) )
		Com_Printf( "There was a problem contacting the update server (%s)\n", curl_easy_strerror( ret ) );

	//Successfully fetched the JSON object
	else if ( chunk.size )
		CheckFiles( chunk.memory );

	if ( chunk.memory )
		free( chunk.memory );
}

void JAPP_CURL_Init( void )
{
	ENG_Cvar_Get( "japp_updateURL",	"japp.jkhub.org/downloads/cl/",		CVAR_ARCHIVE );
	ENG_Cvar_Get( "japp_update",	"1",								CVAR_ARCHIVE );

	curl_global_init(CURL_GLOBAL_ALL);
}

void JAPP_CURL_Shutdown( void )
{
	curl_global_cleanup();
	curl_handle = NULL;
}

#endif // !OPENJK
