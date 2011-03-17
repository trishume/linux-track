#include <linux/types.h>
#include <webcam_driver.h>
#include <utils.h>
#include <string.h>

#include <cal.h>
#include <pref.h>
#include <pref_int.h>

#include <stdio.h>
#include <unistd.h>

struct camera_control_block ccb;

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  char **webcams = NULL;
  if(ltr_int_enum_webcams(&webcams) > 0){
    int i = 0;
    while(webcams[i] != NULL){
      printf("Webcam with id:'%s'\n", webcams[i]);
      webcam_formats fmts;
      ltr_int_enum_webcam_formats(webcams[i], &fmts);
      int j;
      for(j = 0; j < fmts.entries; ++j){
	printf("%s: %d x %d @ %f\n", fmts.fmt_strings[fmts.formats[j].i],
	       fmts.formats[j].w, fmts.formats[j].h,
	       (float)fmts.formats[j].fps_den / fmts.formats[j].fps_num);
      }

      ltr_int_enum_webcam_formats_cleanup(&fmts);
      
      ++i;
    }
    
    ltr_int_array_cleanup(&webcams);
  }
  
  return 0;
  
  if(!ltr_int_read_prefs(NULL, true)){
    ltr_int_log_message("Couldn't load preferences!\n");
    return -1;
  }
  
  
  ccb.device.category = webcam;
  ccb.device.device_id = "Live! Cam Optia";
//  ccb.device.device_id = "USB2.0 1.3M UVC WebCam ";
  if(ltr_int_tracker_init(&ccb)!= 0)
  {
    printf("Problem initializing webcam!\n");
    return 1;
  };
  printf("Init successfull! Res %d x %d\n", ccb.pixel_width, ccb.pixel_height);
  struct frame_type ft;
  ft.bloblist.blobs = ltr_int_my_malloc(sizeof(struct blob_type) * 3);
  ft.bloblist.num_blobs = 3;
  ft.bitmap = NULL;
  
  int i;
  
  printf("Reading frames: ");
  for(i = 0; i< 5; ++i){
    if(ltr_int_tracker_get_frame(&ccb, &ft) == 0){
      printf("."); 
    }else{
      printf("Problem getting frame\n");
    }
  }
  printf("\n");
  printf("Pausing for a second...\n");
  if(ltr_int_tracker_suspend(&ccb) != 0){
    printf("Problem suspending!\n");
  }
  sleep(5);
  printf("Starting again...\n");
  if(ltr_int_tracker_wakeup(&ccb) != 0){
    printf("Problem waking up!\n");
  }
  printf("Reading frames: ");
  for(i = 0; i< 50; ++i){
    if(ltr_int_tracker_get_frame(&ccb, &ft) == 0){
      printf("."); 
    }else{
      printf("Problem getting frame\n");
    }
  }
  printf("\n");
  printf("Shutting down...\n");
  ltr_int_tracker_shutdown(&ccb);
  ltr_int_frame_free(&ccb, &ft);
  ltr_int_free_prefs();
  printf("Webcam closed!\n");
  return 0;
}
