#ifndef RUNLOOP__H
#define RUNLOOP__H

typedef int (*tracker_init_fun)(struct camera_control_block *ccb);
typedef int (*tracker_pause_fun)();
typedef int (*tracker_get_frame_fun)(struct camera_control_block *ccb, struct frame_type *frame);
typedef int (*tracker_resume_fun)();
typedef int (*tracker_close_fun)();

typedef struct {
  tracker_init_fun tracker_init;
  tracker_pause_fun tracker_pause;
  tracker_get_frame_fun tracker_get_frame;
  tracker_resume_fun tracker_resume;
  tracker_close_fun tracker_close;
} tracker_interface;

extern tracker_interface trck_iface;

#endif