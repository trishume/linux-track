#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <image_process.h>
#include <cal.h>
#include <ipc_utils.h>
#include "buffer.h"
#include <com_proc.h>

static pthread_cond_t state_cv = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t state_mx = PTHREAD_MUTEX_INITIALIZER;
static bool new_frame_flag = false;
static pthread_t processing_thread;
static bool end_flag = false;

static struct mmap_s *mmm;

static int width = -1;
static int height = -1;
static int reader = 0;
static int writer = 0;

static void *processingThreadFun(void *param)
{
  (void) param;
  
  while(!end_flag){
    pthread_mutex_lock(&state_mx);
    while(!new_frame_flag){
      pthread_cond_wait(&state_cv, &state_mx);
    }
    new_frame_flag = false;
    pthread_mutex_unlock(&state_mx);
//    printf("Processing frame!\n");
    if(isEmpty(reader)){
//      printf("No new buffer!\n");
    }else{
//      printf("Processing buffer %d @ %p\n", reader, getCurrentBuffer(reader));
      image img = {
	.bitmap = getCurrentBuffer(reader),
	.w = width,
	.h = height,
	.ratio = 1.0f
      };
      struct blob_type blobs_array[3] = {
	{0.0f, 0.0f, -1},
	{0.0f, 0.0f, -1},
	{0.0f, 0.0f, -1}
      };
      struct bloblist_type bloblist = {
	.num_blobs = 3,
	.blobs = blobs_array
      };
      ltr_int_to_stripes(&img);
      if(ltr_int_stripes_to_blobs(3, &bloblist, ltr_int_getMinBlob(mmm), ltr_int_getMaxBlob(mmm), &img) == 0){
	ltr_int_setBlobs(mmm, blobs_array, bloblist.num_blobs);
        if(!ltr_int_getFrameFlag(mmm)){
//	  printf("Copying buffer!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	  memcpy(ltr_int_getFramePtr(mmm), img.bitmap, width * height);
	  ltr_int_setFrameFlag(mmm);
	}
      }
      bufferRead(&reader);
    }
  }
  return NULL;
}

bool startProcessing(int w, int h, int buffers, struct mmap_s *mmm_p)
{
  mmm = mmm_p;
  new_frame_flag = 0;
  end_flag = false;
  width = w;
  height = h;
  if(!createBuffers(buffers, w * h)){
//    printf("Problem creating buffers!\n");
    return false;
  }
  ltr_int_prepare_for_processing(w, h);
//  printf("Starting processing thread!\n");
  return 0 == pthread_create(&processing_thread, NULL, processingThreadFun, NULL);
}

void endProcessing()
{
//  printf("Signaling end to the processing thread!\n");
  end_flag = true;
}

bool newFrame(unsigned char *ptr)
{
  if(!isEmpty(writer)){
//    printf("No empty buffer!\n");
    return false;
  }
  
  unsigned char *dest = getCurrentBuffer(writer);
//  printf("Writing buffer %d @ %p\n", writer, dest);
  unsigned char thr = (unsigned char)ltr_int_getThreshold(mmm);
  size_t i;
  for(i = 0; i < (size_t) width * height; ++i){
    dest[i] = (*ptr >= thr) ? *ptr : 0;
    ptr += 2;
  }
  bufferWritten(&writer);
  
//  printf("Signaling new frame!\n");
  pthread_mutex_lock(&state_mx);
  new_frame_flag = true;
  pthread_cond_broadcast(&state_cv);
  pthread_mutex_unlock(&state_mx);
  return true;
}

