/*
 *  Ogglib - a mini library to add Ogg Vorbis support to allegro programs
 *           Based on Peter Wang's quick hack
 *           Distributed under the BSD-like license used by Ogg Vorbis.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int load_vorbis_stream_offset (FILE *fp);
extern int load_vorbis_stream (const char *filename);
extern void unload_vorbis_stream (void);
extern int poll_vorbis_stream (void);

#ifdef __cplusplus
}
#endif
