extern struct video_window vwin;
extern struct video_picture vpic;
extern struct video_buffer vbuf;
extern struct video_capability vcap;
extern struct video_audio vaudio;
extern struct video_tuner vt;
extern unsigned long vfrequency;
extern unsigned long vbase_freq;
extern int vchannel;
extern int grabber_format;
extern int capture_running;
extern int capture_hidden;
extern int tv_fd;
extern int kill_on_overlap;
extern int clean_display;
extern int chroma_key;
extern int fixed_size;
extern int need_colour_cube;
extern int need_greymap;	
extern int use_shm;
extern int xsize, ysize;
extern int xmin, ymin;
extern int xmax, ymax;

extern unsigned long channel_compute(int frequency);
extern char *probe_tv_set(int unit);
extern int open_tv_card(int unit);
extern void close_tv_card(int handle);
extern int video_ll_mapping(struct video_buffer *vb);
extern void set_tv_frequency(long freq);
extern void set_tv_picture(struct video_picture *vp);

extern void make_about_box(int x, int y);

extern void preferences_page(void);
extern void colour_setting(GtkWidget *w);
extern void frequency_setting(void);
