#include "g_local.h"
#include <unordered_map>
#include <future>
#include <mutex>
extern "C" {
	#include "maxmind\maxminddb.h"
}
namespace GeoIP {
	static MMDB_s handle;
	bool Init(void) {
		int status = -1;
		if ((status = MMDB_open("japlus/GeoLite2-Country.mmdb", MMDB_MODE_MMAP, &handle)) != MMDB_SUCCESS) {
			trap->Print("Error occured while initialising MaxMind GeoIP: \"%s\"\n", MMDB_strerror(status));
			return false;
		}
		return true;
	}

	void ShutDown(void) {
		MMDB_close(&handle);
	}

	GeoIPData *GetIPInfo(const std::string ip) {
		GeoIPData *data = new GeoIPData(ip);
		std::future<void> result = std::async(std::launch::async, [](GeoIPData *data) {
			int error = -1, gai_error = -1;
			MMDB_lookup_result_s result =  MMDB_lookup_string(&handle, data->getIp().c_str(), &gai_error, &error);
			if (error != MMDB_SUCCESS || gai_error != 0) {
				std::string *str = data->getData();
				*str = MMDB_strerror(error);
				data->setStatus(0); // error
				return;
			}
			if (result.found_entry) {
				MMDB_entry_s entry = result.entry;
				MMDB_entry_data_s entry_data;
				if ((error = MMDB_get_value(&entry, &entry_data, "names", "en", NULL)) != MMDB_SUCCESS) {
					std::string *str = data->getData();
					*str = MMDB_strerror(error);
					data->setStatus(0); // error
					return;
				}
				if (entry_data.has_data) {
					std::string *str = data->getData();
					*str = entry_data.utf8_string;
					data->setStatus(1);
					return;
				}
				else {
					*data->getData() = "Unknown";
					data->setStatus(1);
					return;
				}
			}
			else {
				*data->getData() = "Unknown";
				data->setStatus(1);
				return;
			}
		}, data);
		return data;
	}
}