#include <gst/interfaces/mixer.h>
#include <gst/interfaces/mixertrack.h>
#include <gst/interfaces/mixeroptions.h>

#include <glib.h>
#include <glib/gi18n.h>

gint get_page_num (GstMixerTrack *track)
{
	/* GstMixerOptions derives from GstMixerTrack */
	if (GST_IS_MIXER_OPTIONS (track)) {
		return 3;
	} else {
		/* present tracks without channels as toggle switches */
		if (track->num_channels == 0)
			return 2;
		else {
			/* is it possible to have a track that does input and output? */
			g_assert (! (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_INPUT)
			            && GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_OUTPUT)));

			if (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_INPUT))
				return 1;
			else if (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_OUTPUT))
				return 0;
		}
	}
	
	g_assert_not_reached ();
}

gchar *get_page_description (gint n)
{
	/* needs i18n work */
	switch (n) {
	case 0:
		return _("Playback");
	case 1:
		return _("Capture");
	case 2:
		return _("Switch");
	case 3:
		return _("Option");
	}

	g_assert_not_reached ();
}
