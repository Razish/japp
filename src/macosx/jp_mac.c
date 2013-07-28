#include <string.h>
#ifdef UI_EXPORTS
#include <curl/curl.h>
#include <curl/curl_md5.h>
#include <curl/warnless.h>
#include <openssl/md5.h>

#include <CoreFoundation/CoreFoundation.h>
#endif

#define CURL_MASK_UINT  0xFFFFFFFF

char *strrev(char *str)
{
	char *p1, *p2;
	
	if (! str || ! *str)
		return str;
	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
	{
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}

#ifdef UI_EXPORTS
void JAPP_md5it(unsigned char *outbuffer, /* 16 bytes */
                const unsigned char *input,
				const unsigned int len )
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, input, curlx_uztoui(len));
	MD5_Final(outbuffer, &ctx);
}

/*
 ** unsigned size_t to unsigned int
 */

unsigned int curlx_uztoui(size_t uznum)
{
	return (unsigned int)(uznum & (size_t) CURL_MASK_UINT);
}

int whichVersionDigDl( char *version )
{
	CFBundleRef mainBundle;
	CFStringRef mainBundleExecutableName, mainBundleVersion;
	CFStringRef digitalDownloadString = CFSTR("Jedi Academy MP (NoCD)");
	CFStringRef versionString = CFSTR("1.0.3e");//*need* this version
	
	CFComparisonResult result;
	
	mainBundle = CFBundleGetMainBundle();
	mainBundleExecutableName = CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleExecutableKey);
	mainBundleVersion = CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleVersionKey);
	
	CFStringGetCString(mainBundleVersion, version, 8, kCFStringEncodingASCII);

	result = CFStringCompareWithOptions(versionString, mainBundleVersion, CFRangeMake(0,CFStringGetLength(versionString)), kCFCompareEqualTo);
	
	if (result != kCFCompareEqualTo) {
		return 0;//wrong version
	}

	result = CFStringCompareWithOptions(digitalDownloadString, mainBundleExecutableName, CFRangeMake(0,CFStringGetLength(digitalDownloadString)), kCFCompareEqualTo);
	
	if (result == kCFCompareEqualTo) {
        return 1;//digital download version
    } else {
        return 2;//disc version
    }
}
#endif