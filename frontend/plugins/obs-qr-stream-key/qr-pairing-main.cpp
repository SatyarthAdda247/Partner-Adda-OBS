#include <obs-module.h>
#include <obs-frontend-api.h>

#include "qr-pairing-plugin.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-qr-stream-key", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS Adda QR Stream Key pairing plugin";
}

bool obs_module_load(void)
{
	return true;
}

void obs_module_unload(void)
{
	QRPairingPlugin::instance()->shutdown();
}

void obs_module_post_load(void)
{
	obs_frontend_add_event_callback(
		[](enum obs_frontend_event event, void *) {
			if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING)
				QRPairingPlugin::instance()->init();
		},
		nullptr);
}
