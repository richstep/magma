/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */
#define SERVICE303
#define SERVICE303_TASK_C

#include <stdio.h>

#include "log.h"
#include "intertask_interface.h"
#include "timer.h"
#include "common_defs.h"
#include "service303.h"
#include "bstrlib.h"
#include "intertask_interface_types.h"
#include "itti_types.h"
#include "timer_messages_types.h"
#include "itti_free_defined_msg.h"

static void service303_server_exit(void);
static void service303_message_exit(void);

static long service303_epc_stats_timer_id;
task_zmq_ctx_t service303_server_task_zmq_ctx;
task_zmq_ctx_t service303_message_task_zmq_ctx;

static int handle_service303_server_message(zloop_t* loop, zsock_t* reader, void* arg)
{
  zframe_t* msg_frame = zframe_recv(reader);
  assert(msg_frame);
  MessageDef* received_message_p = (MessageDef*) zframe_data(msg_frame);

  switch (ITTI_MSG_ID(received_message_p)) {
    case TERMINATE_MESSAGE:
      itti_free_msg_content(received_message_p);
      zframe_destroy(&msg_frame);
      service303_server_exit();
      break;
    default: {
      OAILOG_DEBUG(
        LOG_UTIL,
        "Unkwnon message ID %d: %s\n",
        ITTI_MSG_ID(received_message_p),
        ITTI_MSG_NAME(received_message_p));
    } break;
  }

  itti_free_msg_content(received_message_p);
  zframe_destroy(&msg_frame);
  return 0;
}

static void* service303_server_thread(__attribute__((unused)) void* args)
{
  service303_data_t *service303_data = (service303_data_t *) args;

  start_service303_server(service303_data->name, service303_data->version);

  itti_mark_task_ready(TASK_SERVICE303_SERVER);
  init_task_context(
      TASK_SERVICE303_SERVER,
      (task_id_t []) {},
      0,
      handle_service303_server_message,
      &service303_server_task_zmq_ctx);

  zloop_start(service303_server_task_zmq_ctx.event_loop);
  service303_server_exit();
  return NULL;
}

static int handle_service_message (zloop_t* loop, zsock_t* reader, void* arg)
{
  zframe_t* msg_frame = zframe_recv(reader);
  assert(msg_frame);
  MessageDef* received_message_p = (MessageDef*) zframe_data(msg_frame);

  switch (ITTI_MSG_ID(received_message_p)) {
    case TIMER_HAS_EXPIRED: {
      /*
      * Check statistic timer
      */
      if (!timer_exists(
            received_message_p->ittiMsg.timer_has_expired.timer_id)) {
        break;
      }
      if (
        received_message_p->ittiMsg.timer_has_expired.timer_id ==
        service303_epc_stats_timer_id) {
        service303_statistics_read();
      }
      timer_handle_expired(
        received_message_p->ittiMsg.timer_has_expired.timer_id);
      break;
    }
    case APPLICATION_HEALTHY_MSG: {
      service303_set_application_health(APP_HEALTHY);
    } break;
    case APPLICATION_UNHEALTHY_MSG: {
      service303_set_application_health(APP_UNHEALTHY);
    } break;
    case TERMINATE_MESSAGE:
      zframe_destroy(&msg_frame);
      service303_message_exit();
      break;
    default: {
      OAILOG_DEBUG(
        LOG_UTIL,
        "Unkwnon message ID %d: %s\n",
        ITTI_MSG_ID(received_message_p),
        ITTI_MSG_NAME(received_message_p));
    } break;
  }

  zframe_destroy(&msg_frame);
  return 0;
}

static void* service303_thread(void* args)
{
  bstring pkg_name = bfromcstr(SERVICE303_MME_PACKAGE_NAME);
  service303_data_t *service303_data = (service303_data_t *) args;

  itti_mark_task_ready(TASK_SERVICE303);
  init_task_context(
      TASK_SERVICE303,
      (task_id_t []) {},
      0,
      handle_service_message,
      &service303_message_task_zmq_ctx);


  if (bstricmp(service303_data->name, pkg_name) == 0) {
    /* NOTE : Above check for MME package is added since SPGW does not support stats at present
     * TODO : Whenever SPGW implements stats,remove the above "if" check so that timer is started
     * in SPGW also and SPGW stats can also be read as part of timer expiry handling
     */

    /*
     * Check if this thread is started by MME service if so start a timer
     * to trigger reading the mme stats so that it cen be sent to server
     * for display
     * Start periodic timer
     */
    if (
      timer_setup(
        EPC_STATS_TIMER_VALUE,
        0,
        TASK_SERVICE303,
        INSTANCE_DEFAULT,
        TIMER_PERIODIC,
        NULL,
        0,
        &service303_epc_stats_timer_id) < 0) {
      OAILOG_ALERT(
        LOG_UTIL,
        " TASK SERVICE303_MESSAGE for EPC: Periodic Stat Timer Start: ERROR\n");
      service303_epc_stats_timer_id = 0;
    }
  }

  bdestroy(pkg_name);

  zloop_start(service303_message_task_zmq_ctx.event_loop);
  service303_message_exit();
  return NULL;
}

int service303_init(service303_data_t *service303_data)
{
  OAILOG_DEBUG(LOG_UTIL, "Initializing Service303 task interface\n");

  if (
    itti_create_task(
      TASK_SERVICE303_SERVER, &service303_server_thread, service303_data) < 0) {
    perror("pthread_create");
    OAILOG_ALERT(LOG_UTIL, "Initializing Service303 server: ERROR\n");
    return RETURNerror;
  }

  if (
    itti_create_task(
      TASK_SERVICE303, &service303_thread, service303_data) < 0) {
    perror("pthread_create");
    OAILOG_ALERT(
      LOG_UTIL, "Initializing Service303 message interface: ERROR\n");
    return RETURNerror;
  }

  OAILOG_DEBUG(LOG_UTIL, "Initializing Service303 task interface: DONE\n");
  return RETURNok;
}

static void service303_server_exit(void)
{
  destroy_task_context(&service303_server_task_zmq_ctx);
  OAI_FPRINTF_INFO("TASK_SERVICE303_SERVER terminated\n");
  pthread_exit(NULL);
}

static void service303_message_exit(void)
{
  destroy_task_context(&service303_message_task_zmq_ctx);
  timer_remove(service303_epc_stats_timer_id, NULL);
  OAI_FPRINTF_INFO("TASK_SERVICE303 terminated\n");
  pthread_exit(NULL);
}
