#include <stdio.h>
#include <pthread.h>
#include "tracking.h"
#include "math_utils.h"
#include "pose.h"
#include "utils.h"
#include "pref_global.h"

/**************************/
/* private Static members */
/**************************/

//static struct bloblist_type filtered_bloblist;
//static struct blob_type filtered_blobs[3];
//static bool first_frame = true;
static bool recenter = false;
static float cam_distance = 1000.0f;

pose_t ltr_int_orig_pose;

/*******************************/
/* private function prototypes */
/*******************************/
/*static float expfilt(float x, 
              float y_minus_1,
              float filtfactor);

static void expfilt_vec(float x[3], 
              float y_minus_1[3],
              float filtfactor,
              float res[3]);
*/
/************************/
/* function definitions */
/************************/

bool ltr_int_check_pose()
{
  struct reflector_model_type rm;
  if(ltr_int_model_changed(true)){
    if(ltr_int_get_model_setup(&rm) == false){
      ltr_int_log_message("Can't get pose setup!\n");
      return false;
    }
    ltr_int_log_message("Initializing model!\n");
    ltr_int_pose_init(rm);
  }
  return true;
}

static bool tracking_initialized = false;
static dbg_flag_type tracking_dbg_flag = DBG_CHECK;
static dbg_flag_type raw_dbg_flag = DBG_CHECK;
static int orientation = 0;
static bool behind = false;

bool ltr_int_init_tracking()
{
  if(tracking_dbg_flag == DBG_CHECK){
    tracking_dbg_flag = ltr_int_get_dbg_flag('t');
    raw_dbg_flag = ltr_int_get_dbg_flag('r');
  }

  orientation = ltr_int_get_orientation();
  if(orientation & 8){
    behind = true;
  }else{
    behind = false;
  }
  
  if(ltr_int_check_pose() == false){
    ltr_int_log_message("Can't get pose setup!\n");
    return false;
  }
  ltr_int_init_axes();
//  filtered_bloblist.num_blobs = 3;
//  filtered_bloblist.blobs = filtered_blobs;
//  first_frame = true;
  ltr_int_log_message("Tracking initialized!\n");
  tracking_initialized = true;
  return true;
}

int ltr_int_recenter_tracking()
{
  recenter = true;
  return 0;
}


static pthread_mutex_t pose_mutex = PTHREAD_MUTEX_INITIALIZER;
static double angles[3] = {0.0f, 0.0f, 0.0f};
static double translations[3] = {0.0f, 0.0f, 0.0f};

static void filter_frame(struct frame_type *frame)
{
  static float memory_x[3] = {0.0f, 0.0f, 0.0f};
  static float memory_y[3] = {0.0f, 0.0f, 0.0f};
  
  unsigned int i;
  for(i = 0; i < frame->bloblist.num_blobs; ++i){
    if(ltr_int_is_finite(frame->bloblist.blobs[i].x)){
      frame->bloblist.blobs[i].x = 
        ltr_int_nonlinfilt(frame->bloblist.blobs[i].x, memory_x[i], 2.0);
    }
    if(ltr_int_is_finite(frame->bloblist.blobs[i].y)){
      frame->bloblist.blobs[i].y = 
        ltr_int_nonlinfilt(frame->bloblist.blobs[i].y, memory_y[i], 2.0);
    }
  }
}


static void ltr_int_rotate_camera(float *x, float *y, int orientation)
{
  float tmp_x;
  float tmp_y;
  if(orientation & 4){
    tmp_x = *y;
    tmp_y = *x;
  }else{
    tmp_x = *x;
    tmp_y = *y;
  }
  *x = (orientation & 1) ? -tmp_x : tmp_x;
  *y = (orientation & 2) ? -tmp_y : tmp_y;
}


static void ltr_int_remove_camera_rotation(struct bloblist_type bl)
{
  unsigned int i;
  for(i = 0; i < bl.num_blobs; ++i){
    ltr_int_rotate_camera(&(bl.blobs[i].x), &(bl.blobs[i].y), orientation);
  }
}

static int update_pose_1pt(struct frame_type *frame)
{
  static float c_x = 0.0f;
  static float c_y = 0.0f;
  static float c_z = 0.0f;
  bool recentering = false;
  
  ltr_int_check_pose();
  
  if(tracking_dbg_flag == DBG_ON){
    unsigned int i;
    for(i = 0; i < frame->bloblist.num_blobs; ++i){
      ltr_int_log_message("*DBG_t* %d: %g %g %d\n", i, frame->bloblist.blobs[i].x, frame->bloblist.blobs[i].y,
                          frame->bloblist.blobs[i].score);
    }
  }
  
  if(ltr_int_is_finite(frame->bloblist.blobs[0].x) && ltr_int_is_finite(frame->bloblist.blobs[0].y)
      && (frame->bloblist.num_blobs > 0)){
  }else{
    return -1;
  }

  ltr_int_remove_camera_rotation(frame->bloblist);

  if(recenter == true){
    recenter = false;
    recentering = true;
  } 
  
  if(recentering){
    c_x = frame->bloblist.blobs[0].x;
    c_y = frame->bloblist.blobs[0].y;
    c_z = cam_distance * sqrtf((float)frame->bloblist.blobs[0].score);
    printf("Recentering! c_z = %f\n", c_z);
  }
//printf("cz = %f, z = %f\n", c_z, sqrtf((float)frame->bloblist.blobs[0].score));
  pthread_mutex_lock(&pose_mutex);
  angles[0] = c_y - frame->bloblist.blobs[0].y;
  angles[1] = frame->bloblist.blobs[0].x - c_x;
  angles[2] = 0.0f;
  translations[0] = 0.0f;
  translations[1] = 0.0f;
  if(ltr_int_is_face() && (frame->bloblist.blobs[0].score > 0)){
      translations[2] = c_z / sqrtf((float)frame->bloblist.blobs[0].score) - cam_distance;
  }else{
    translations[2] = 0.0f;
  }
  
  if(behind){
    angles[0] *= -1;
  }
  
  pthread_mutex_unlock(&pose_mutex);
/*
  ltr_int_orig_pose.pitch = angles[0];
  ltr_int_orig_pose.heading = angles[1];
  ltr_int_orig_pose.roll = 0;
  ltr_int_orig_pose.tx = 0;
  ltr_int_orig_pose.ty = 0;
  ltr_int_orig_pose.tz = translations[2];
  
  static double filtered[3] = {0.0f, 0.0f, 0.0f};
  float filterfactor=1.0;
  ltr_int_get_filter_factor(&filterfactor);
  double filter_factors[3] = {filterfactor, filterfactor, filterfactor * 10};
  double values[] = {angles[0], angles[1], translations[2]};
  ltr_int_nonlinfilt_vec(values, filtered, filter_factors, filtered);
  angles[0] = clamp_angle(ltr_int_val_on_axis(PITCH, filtered[0]));
  angles[1] = clamp_angle(ltr_int_val_on_axis(PITCH, filtered[1]));
  translations[2] = ltr_int_val_on_axis(TZ, filtered[2]);
*/
  return 0;
}


static int update_pose_3pt(struct frame_type *frame)
{
  bool recentering = false;
  
  ltr_int_check_pose();
  
  if(frame->bloblist.num_blobs != 3){
    return -1;
  }
  if(ltr_int_is_finite(frame->bloblist.blobs[0].x) && ltr_int_is_finite(frame->bloblist.blobs[0].y) &&
      ltr_int_is_finite(frame->bloblist.blobs[1].x) && ltr_int_is_finite(frame->bloblist.blobs[1].y) &&
      ltr_int_is_finite(frame->bloblist.blobs[2].x) && ltr_int_is_finite(frame->bloblist.blobs[2].y)){
  }else{
    return -1;
  }
  if(tracking_dbg_flag == DBG_ON){
    unsigned int i;
    for(i = 0; i < frame->bloblist.num_blobs; ++i){
      ltr_int_log_message("*DBG_t* %d: %g %g %d\n", i, frame->bloblist.blobs[i].x, frame->bloblist.blobs[i].y,
                          frame->bloblist.blobs[i].score);
    }
  }
  
  if(recenter == true){
    recenter = false;
    recentering = true;
  } 
  ltr_int_remove_camera_rotation(frame->bloblist);
  ltr_int_pose_sort_blobs(frame->bloblist);

  int res = 0;
  pose_t t;
  ltr_int_pose_process_blobs(frame->bloblist, &t, recentering);
  pthread_mutex_lock(&pose_mutex);
  angles[0] = t.pitch;
  angles[1] = t.yaw;
  angles[2] = t.roll;
  translations[0] = t.tx;
  translations[1] = t.ty;
  translations[2] = t.tz;

  if(behind){
    angles[0] *= -1;
    //angles[1] *= -1;
    angles[2] *= -1;
    translations[0] *= -1;
    translations[2] *= -1;
  }
  pthread_mutex_unlock(&pose_mutex);
  if(raw_dbg_flag == DBG_ON){
    printf("*DBG_r* yaw: %g pitch: %g roll: %g\n", angles[0], angles[1], angles[2]);
    ltr_int_log_message("*DBG_r* x: %g y: %g z: %g\n", 
                        translations[0], translations[1], translations[2]);
  }
  
  return res;
}

bool ltr_int_postprocess_axes(pose_t *pose)
{
//  printf(">>%f %f %f  %f %f %f\n", pose->pitch, pose->heading, pose->roll, pose->tx, pose->ty, pose->tz);
  static bool init = true;
  if(init){
    ltr_int_init_axes();
    init = false;
  }
  static float filterfactor=1.0;
  ltr_int_get_filter_factor(&filterfactor);
  static double filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static double filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  double filter_factors_angles[3] = {filterfactor, filterfactor, filterfactor};
  double filter_factors_translations[3] = {filterfactor, filterfactor, filterfactor * 10};
  double raw_angles[3] = {pose->pitch, pose->yaw, pose->roll};
  ltr_int_nonlinfilt_vec(raw_angles, filtered_angles, filter_factors_angles, filtered_angles);
  
  pose->pitch = clamp_angle(ltr_int_val_on_axis(PITCH, filtered_angles[0]));
  pose->yaw = clamp_angle(ltr_int_val_on_axis(YAW, filtered_angles[1]));
  pose->roll = clamp_angle(ltr_int_val_on_axis(ROLL, filtered_angles[2]));
//  printf("Pitch: %g   Yaw: %g  Roll: %g\n", pose->pitch, pose->heading, pose->roll);
  
  double rotated[3];
  double transform[3][3];
  double displacement[3] = {pose->tx, pose->ty, pose->tz};
//  ltr_int_euler_to_matrix(pitch / 180.0 * M_PI, yaw / 180.0 * M_PI, 
//                          roll / 180.0 * M_PI, transform);
  ltr_int_euler_to_matrix(pose->pitch / 180.0 * M_PI, pose->yaw / 180.0 * M_PI, 
                          pose->roll / 180.0 * M_PI, transform);
  ltr_int_matrix_times_vec(transform, displacement, rotated);
//  ltr_int_print_matrix(transform, "trf");
//  ltr_int_print_vec(displacement, "mv");
//  ltr_int_print_vec(rotated, "rotated");
  ltr_int_nonlinfilt_vec(rotated, filtered_translations, filter_factors_translations, 
        filtered_translations);
  //ltr_int_orig_pose.tx = rotated[0];
  //ltr_int_orig_pose.ty = rotated[1];
  //ltr_int_orig_pose.tz = rotated[2];
  pose->tx = ltr_int_val_on_axis(TX, filtered_translations[0]);
  pose->ty = ltr_int_val_on_axis(TY, filtered_translations[1]);
  pose->tz = ltr_int_val_on_axis(TZ, filtered_translations[2]);
//  ltr_int_print_vec(displacement, "tr");
//  printf(">>>%f %f %f  %f %f %f\n", pose->pitch, pose->heading, pose->roll, pose->tx, pose->ty, pose->tz);
  return true;
}


static unsigned int counter_d;

int ltr_int_update_pose(struct frame_type *frame)
{
  counter_d = frame->counter;
  filter_frame(frame);
  if(ltr_int_is_single_point()){
    return update_pose_1pt(frame);
  }else{
    return update_pose_3pt(frame);
  }
}

int ltr_int_tracking_get_camera(float *heading,
                      float *pitch,
                      float *roll,
                      float *tx,
                      float *ty,
                      float *tz,
                      unsigned int *counter)
{
  if(!tracking_initialized){
    ltr_int_init_tracking();
  }
  
  pthread_mutex_lock(&pose_mutex);
  *pitch = angles[0];
  *heading = angles[1];
  *roll = angles[2];
  *tx = translations[0];
  *ty = translations[1];
  *tz = translations[2];
  
  *counter = counter_d;
  pthread_mutex_unlock(&pose_mutex);
  return 0;
}



double ltr_int_nonlinfilt(double x, 
              double y_minus_1,
              double filterfactor) 
{
  double y;
  if(!ltr_int_is_finite(x)){
    return y_minus_1;
  }
  double delta = x - y_minus_1;
  y = y_minus_1 + delta * (fabsf(delta)/(fabsf(delta) + filterfactor));

  return y;
}

void ltr_int_nonlinfilt_vec(double x[3], 
              double y_minus_1[3],
              double filterfactor[3],
              double res[3]) 
{
  res[0] = ltr_int_nonlinfilt(x[0], y_minus_1[0], filterfactor[0]);
  res[1] = ltr_int_nonlinfilt(x[1], y_minus_1[1], filterfactor[1]);
  res[2] = ltr_int_nonlinfilt(x[2], y_minus_1[2], filterfactor[2]);
}


