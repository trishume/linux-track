#include "ltr_srv_comm.h"
#include "ltr_srv_master.h"
#include <stdio.h>
#include <wait.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
#include <ipc_utils.h>
#include <ltlib.h>
#include <utils.h>
#include <tracking.h>
#include <pref.h>
#include <pref_global.h>

static struct mmap_s mmm;
static int master_uplink = -1;
static int master_downlink = -1;
static int stop_slave_reader_thread = false;
static int slave_reader_thread_finished = true;
static pthread_t reader_tid;
static char *profile_name = NULL;
static ltr_axes_t axes;
static bool master_works = false;
static bool master_restart_retries = 3;

typedef enum {MR_OK, MR_FAIL, MR_OFTEN} mr_res_t;

static int ltr_int_open_slave_fifo(int master_uplink, const char *name_template, int max_fifos)
{
  ltr_int_log_message("Registering slave fifo...\n");
  char *data_fifo_name = NULL;
  //Open the data passing fifo and pass it to the master...
  int fifo_number = -1;
  int master_downlink = ltr_int_open_unique_fifo(&data_fifo_name, &fifo_number, name_template, max_fifos);
  ltr_int_log_message("Trying to open unique fifo %s => %d\n", data_fifo_name, master_downlink);
  free(data_fifo_name);
  if(master_downlink <= 0){
    return -1;
  }
  if(ltr_int_send_message_w_str(master_uplink, CMD_NEW_FIFO, fifo_number, profile_name) == 0){
    return master_downlink;
  }else{
    close(master_downlink);
    return -1;
  }
}

static bool ltr_int_start_master(int *master_uplink, int *master_downlink)
{
  bool is_child;
  master_works = false;
  if(ltr_int_gui_lock(false)){ //don't try to start master if gui is running...
    int fifo = ltr_int_open_fifo_exclusive(ltr_int_master_fifo_name());
    if(fifo > 0){
      close(fifo);
      printf("Master is not running, start it\n"); 
      char *args[] = {"srv", NULL};
      args[0] = ltr_int_get_app_path("/ltr_server1");
      ltr_int_fork_child(args, &is_child);
      int status;
      //Disable the wait when not daemonizing master!!!
      wait(&status);
      //At this point master is either running or exited (depending on the state of fifo)
      free(args[0]);
    }
  }
  *master_uplink = ltr_int_open_fifo_for_writing(ltr_int_master_fifo_name(), true);
  if(*master_uplink <= 0){
    printf("Couldn't open fifo to master!\n");
    return false;
  }
  if((*master_downlink = ltr_int_open_slave_fifo(*master_uplink, ltr_int_slave_fifo_name(), 
                                                 ltr_int_max_slave_fifos())) <= 0){
    printf("Couldn't pass master our fifo!\n");
    close(*master_uplink);
    *master_uplink = -1;
    *master_downlink = -1;
    return false;
  }
  return true;
}

static mr_res_t ltr_int_try_start_master(int *master_uplink, int *master_downlink)
{
  if(master_restart_retries > 0){
    --master_restart_retries;
    return (ltr_int_start_master(master_uplink, master_downlink)) ? MR_OK : MR_FAIL;
  }
  return MR_OFTEN;
}


static void *ltr_int_slave_reader_thread(void *param)
{
  (void) param;
  int received_frames = 0;
  master_works = false;
  if((master_uplink == -1) || (master_downlink == -1)){
    slave_reader_thread_finished = true;
    ltr_int_log_message("Tried to run the slave reader thread(ul = %d, dl = %d)!\n", 
                        master_uplink, master_downlink);
    return NULL;
  }else{
    slave_reader_thread_finished = false;
  }
  struct pollfd downlink_poll= {
    .fd = master_downlink,
    .events = POLLIN,
    .revents = 0
  };
  
  while(!stop_slave_reader_thread){
    downlink_poll.events = POLLIN;
    int fds = poll(&downlink_poll, 1, 1000);
    if(fds < 0){
      perror("poll");
      continue;
    }else if(fds == 0){
      continue;
    }
    if(downlink_poll.revents & POLLHUP){
      break;
    }
    message_t msg;
    struct ltr_comm *com;
    pose_t unfiltered;
    if(ltr_int_fifo_receive(master_downlink, &msg, sizeof(message_t)) != 0){
      ltr_int_log_message("Slave reader problem!\n");
    }
    switch(msg.cmd){
      case CMD_POSE:
        //printf("Have new pose!\n");
        //printf(">>>>%f %f %f\n", msg.pose.yaw, msg.pose.pitch, msg.pose.tz);
        ltr_int_postprocess_axes(axes, &(msg.pose), &unfiltered);
        
        com = mmm.data;
        ltr_int_lockSemaphore(mmm.sem);
        com->heading = msg.pose.yaw;
        com->pitch = msg.pose.pitch;
        com->roll = msg.pose.roll;
        com->tx = msg.pose.tx;
        com->ty = msg.pose.ty;
        com->tz = msg.pose.tz;
        com->counter = msg.pose.counter;
        com->state = msg.pose.status;
        com->preparing_start = false;
        ltr_int_unlockSemaphore(mmm.sem);
        ++received_frames;
        if(received_frames > 20){
          master_works = true;
        }
        break;
      case CMD_PARAM:
        //printf("Changing %s of %s to %f!!!\n", ltr_int_axis_param_get_desc(msg.param.param_id), 
        //  ltr_int_axis_get_desc(msg.param.axis_id), msg.param.flt_val);
        if(msg.param.axis_id == MISC){
          if(msg.param.param_id == MISC_ALTER){
            ltr_int_set_use_alter(msg.param.flt_val > 0.5f);
          }else if(msg.param.param_id == MISC_ALIGN){
            ltr_int_set_tr_align(msg.param.flt_val > 0.5f);
          }else if(msg.param.param_id == MISC_LEGR){
            ltr_int_set_use_oldrot(msg.param.flt_val > 0.5f);
          }
        }else if(msg.param.param_id == AXIS_ENABLED){
          ltr_int_set_axis_bool_param(axes, msg.param.axis_id, msg.param.param_id, msg.param.flt_val > 0.5f);
        }else{
          ltr_int_set_axis_param(axes, msg.param.axis_id, msg.param.param_id, msg.param.flt_val);
        }
        break;
      default:
        ltr_int_log_message("Slave received unexpected message %d!\n", msg.cmd);
        break;
    }
    //printf("Received: '%s'\n", msg.str);
  }
  slave_reader_thread_finished = true;
  return NULL;
}


static void ltr_int_slave_main_loop()
{
  //Prepare to process client requests
  struct ltr_comm *com = mmm.data;
  ltr_cmd cmd = NOP_CMD;
  bool recenter = false;
  bool quit_flag = false;
  bool reader_started = false;
  while(!quit_flag){
    if((com->cmd != NOP_CMD) || com->recenter){
      ltr_int_lockSemaphore(mmm.sem);
      cmd = (ltr_cmd)com->cmd;
      com->cmd = NOP_CMD;
      recenter = com->recenter;
      com->recenter = false;
      ltr_int_unlockSemaphore(mmm.sem);
    }
    switch(cmd){
      case PAUSE_CMD:
        ltr_int_send_message(master_uplink, CMD_PAUSE, 0);
        break;
      case RUN_CMD:
        ltr_int_send_message(master_uplink, CMD_WAKEUP, 0);
        break;
      case STOP_CMD:
        quit_flag = true;
        break;
      default:
        break;
    }
    cmd = NOP_CMD;
    if(recenter){
      ltr_int_log_message("Slave sending master recenter request!\n");
      ltr_int_send_message(master_uplink, CMD_RECENTER, 0);
    }
    recenter = false;
    //Check whether parent lives 
    //  (if not, we got orphaned and got adopted by init (pid 1))
    if(getppid() == 1){
      ltr_int_log_message("Parent died!\n");
      break;
    }
    if(slave_reader_thread_finished){
      //the connection to master was lost - most probably master quitted or crashed
      if(reader_started){
        pthread_join(reader_tid, NULL);
        close(master_downlink);
        close(master_uplink);
        master_uplink = master_downlink = -1;
      }
      if((master_works) || !(ltr_int_gui_lock(false))){
        master_restart_retries = 3;
      }
      mr_res_t mr_res = ltr_int_try_start_master(&master_uplink, &master_downlink);
      if(mr_res == MR_OFTEN){
        break; //Can't get hold of master on the other side after 3 tries... 
      }else if(mr_res == MR_OK){
        //restart reader thread
        stop_slave_reader_thread = false;
        slave_reader_thread_finished = false; //to avoid getting here second time
                                              // if the thread doesn't come up quick enough
        if(pthread_create(&reader_tid, NULL, ltr_int_slave_reader_thread, NULL) == 0){
          reader_started = true;
        }
      }
    }
    usleep(100000);
  }
}

//main slave function

bool ltr_int_slave(const char *c_profile, const char *c_com_file)
{
  profile_name = ltr_int_my_strdup(c_profile);
  if(!ltr_int_read_prefs(NULL, false)){
    ltr_int_log_message("Couldn't load preferences!\n");
    return false;
  }
  ltr_int_init_axes(&axes, profile_name);
  //Prepare client comm channel
  char *com_file = ltr_int_my_strdup(c_com_file);
  if(!ltr_int_mmap_file(com_file, sizeof(struct ltr_comm), &mmm)){
    ltr_int_log_message("Couldn't mmap file!!!\n");
    return false;
  }
  free(com_file);
  
  ltr_int_slave_main_loop();
  
  stop_slave_reader_thread = true;
  pthread_join(reader_tid, NULL);

  if(master_downlink > 0){
    close(master_downlink);
  }
  if(master_uplink > 0){
    close(master_uplink);
  }
  ltr_int_unmap_file(&mmm);
  //finish prefs
  ltr_int_close_axes(&axes);
  ltr_int_free_prefs();
  free(profile_name);
  ltr_int_gui_lock_clean();
  return true;
}


