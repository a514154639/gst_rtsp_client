
#include <gst_rtsp_client.h>

#include <string>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

// gst
#include <gst/gst.h>
#include <gst/gstmemory.h>
#include <gst/gstpad.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/app/gstappsink.h>
#include <gst/gstpipeline.h>
#include <gst/gstcaps.h>

#include <gst/video/gstvideometa.h>
#include <gst/base/gstbasesink.h>
#include <gst/video/video-info.h>
#include <gst/video/video-format.h>
#include <gst/video/video-enumtypes.h>
#include <gst/video/video-tile.h>
#include <gst/base/base-prelude.h>
#include <gst/gstquery.h>

#include <gst/sdp/gstsdpmessage.h>
#include <gobject/gclosure.h>
#include <gst/rtsp/gstrtspmessage.h>
#include <gst/rtsp/gstrtsptransport.h>

// rga
#include <rga/rga.h>
#include <rga/RgaUtils.h>
#include <rga/RockchipRga.h>
#include <rga/im2d.h>

// opencv2
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>

// static GstFlowReturn 
// new_sample (GstElement *sink, GMainLoop *pipline) {
//     GstSample *sample;

//     g_signal_emit_by_name(sink,"pull-sample", &sample);
//     if (sample) {
//         g_print ("*");
//         gst_sample_unref (sample);
//         return GST_FLOW_OK;
//     }

//     return GST_FLOW_ERROR;
// }

// appsink probe
static GstPadProbeReturn
pad_probe (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  struct CustomData *dec = (struct CustomData *) user_data;
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);
  GstCaps *caps;

  (void) pad;

  if (GST_EVENT_TYPE (event) != GST_EVENT_CAPS)
    return GST_PAD_PROBE_OK;

  gst_event_parse_caps (event, &caps);

  if (!caps) {
    GST_ERROR ("caps event without caps");
    return GST_PAD_PROBE_OK;
  }

  if (!gst_video_info_from_caps (&dec->info, caps)) {
    GST_ERROR ("caps event with invalid video caps");
    return GST_PAD_PROBE_OK;
  }

  switch (GST_VIDEO_INFO_FORMAT (&(dec->info))) {
    case GST_VIDEO_FORMAT_I420:
      dec->format = 2;
      break;
    case GST_VIDEO_FORMAT_NV12:
      dec->format = 23;
      break;
    case GST_VIDEO_FORMAT_YUY2:
      dec->format = 4;
      break;
    default:
      GST_ERROR ("unknown format\n");
      return GST_PAD_PROBE_OK;
  }

  dec->isRun = STATUS_CONNECTED;

  return GST_PAD_PROBE_OK;
}

// uridecodebin -> src link tee -> sink 
static void 
on_src_tee_added(GstElement *element, GstPad *pad, gpointer data) {
    // GstPad *sinkpad;
    struct CustomData *decoder = (struct CustomData *)data;

    /* We can now link this pad with the rtsp-decoder sink pad */
    g_print("Dynamic pad created, linking source/demuxer\n");
    GstPad * tee_sinkpad = gst_element_get_static_pad(decoder->tee, "sink");
    gst_pad_link(pad, tee_sinkpad);
    gst_object_unref(tee_sinkpad);
}

static void 
on_src_decodebin_added(GstElement *element, GstPad *pad, gpointer data) {
    struct CustomData *decoder = (struct CustomData *)data;
    
    // decoder->isRun = STATUS_CONNECTED;
    /* We can now link this pad with the rtsp-decoder sink pad */
    g_print("Dynamic pad created, linking source/decodeon_src_decodebin_added\n");
    GstPad * decode_sinkpad = gst_element_get_static_pad(decoder->decode, "sink");
    gst_pad_link(pad, decode_sinkpad);
    gst_object_unref(decode_sinkpad);
}

// appsink query
static GstPadProbeReturn
appsink_query_cb (GstPad * pad G_GNUC_UNUSED, GstPadProbeInfo * info,
    gpointer user_data G_GNUC_UNUSED)
{
  GstQuery *query = (GstQuery *) info->data;

  if (GST_QUERY_TYPE (query) != GST_QUERY_ALLOCATION)
    return GST_PAD_PROBE_OK;

  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return GST_PAD_PROBE_HANDLED;
}

// bus
static gboolean
bus_watch_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  struct CustomData *dec = (struct CustomData *) user_data;
  
  //param not use
  // (void) bus;
  // g_print ( "piple call_bus message m_Id %s %d \n", gst_message_type_get_name(GST_MESSAGE_TYPE (msg)),dec->m_Id);
  

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_UNKNOWN: {
      g_print ( "piple call_bus message GST_MESSAGE_UNKNOWN %d \n",dec->m_Id);
    }
    case GST_MESSAGE_STATE_CHANGED:{
      gchar *dotfilename;
      GstState old_gst_state, cur_gst_state, pending_gst_state;

      /* Only consider state change messages coming from
       * the toplevel element. */
      if (GST_MESSAGE_SRC (msg) != GST_OBJECT (dec->pipeline))
        break;

      gst_message_parse_state_changed (msg, &old_gst_state, &cur_gst_state,
          &pending_gst_state);

      printf ("GStreamer state change:  old: %s  current: %s  pending: %s\n",
          gst_element_state_get_name (old_gst_state),
          gst_element_state_get_name (cur_gst_state),
          gst_element_state_get_name (pending_gst_state)
          );

      dotfilename = g_strdup_printf ("statechange__old-%s__cur-%s__pending-%s",
          gst_element_state_get_name (old_gst_state),
          gst_element_state_get_name (cur_gst_state),
          gst_element_state_get_name (pending_gst_state)
          );
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (dec->pipeline),
          GST_DEBUG_GRAPH_SHOW_ALL, dotfilename);

      // g_print ( "piple call_bus message GST_MESSAGE_STATE_CHANGED %d \n",dec->m_Id);

      g_free (dotfilename);

      break;
    }
    case GST_MESSAGE_REQUEST_STATE:{
      GstState requested_state;
      gst_message_parse_request_state (msg, &requested_state);
      printf ("state change to %s was requested by %s\n",
          gst_element_state_get_name (requested_state),
          GST_MESSAGE_SRC_NAME (msg)
          );
      gst_element_set_state (GST_ELEMENT (dec->pipeline), requested_state);
      break;
    }
    case GST_MESSAGE_LATENCY:{
      printf ("redistributing latency\n");
      gst_bin_recalculate_latency (GST_BIN (dec->pipeline));
      break;
    }
    case GST_MESSAGE_EOS:
      g_print("bus eos \n");
      // g_main_loop_quit (dec->loop);
      // dec->isRun = STATUS_DISCONNECT;
      break;
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_WARNING:
    case GST_MESSAGE_ERROR:{
      g_print ( "piple call_bus message %d \n",dec->m_Id);
      GError *error = NULL;
      gchar *debug_info = NULL;
      gchar const *prefix = "";

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_INFO:
          gst_message_parse_info (msg, &error, &debug_info);
          prefix = "INFO";
          break;
        case GST_MESSAGE_WARNING:
          gst_message_parse_warning (msg, &error, &debug_info);
          prefix = "WARNING";
          break;
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &error, &debug_info);
          prefix = "ERROR";
          break;
        default:
          g_assert_not_reached ();
      }
      g_print ("GStreamer %s: %s; debug info: %s \n", prefix, error->message,
          debug_info);

      g_clear_error (&error);
      g_free (debug_info);

      if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (dec->pipeline),
            GST_DEBUG_GRAPH_SHOW_ALL, "error");
      }
      // TODO: stop mainloop in case of an error
      // g_main_loop_quit(dec->loop);
      // g_print("bus disconnect %d \n",dec->m_Id);
      // dec->isRun = STATUS_DISCONNECT;

      break;
    }
    default:
      break;
  }

  return TRUE;
}

// static void
// buffer_to_file (struct CustomData *dec, GstBuffer * buf)
// {
//   int ret;
//   GstVideoMeta *meta = gst_buffer_get_video_meta (buf);
//   guint nplanes = GST_VIDEO_INFO_N_PLANES (&(dec->info));
//   guint width, height;
//   GstMapInfo map_info;
//   gchar filename[128];
//   GstVideoFormat pixfmt;
//   const char *pixfmt_str;

//   pixfmt = GST_VIDEO_INFO_FORMAT (&(dec->info));
//   pixfmt_str = gst_video_format_to_string (pixfmt);

//   /* TODO: use the DMABUF directly */

//   gst_buffer_map (buf, &map_info, GST_MAP_READ);

//   width = GST_VIDEO_INFO_WIDTH (&(dec->info));
//   height = GST_VIDEO_INFO_HEIGHT (&(dec->info));

//   /* output some information at the beginning (= when the first frame is handled) */
//   if (dec->frame == 0) {
//     printf ("===================================\n");
//     printf ("GStreamer video stream information:\n");
//     printf ("  size: %u x %u pixel\n", width, height);
//     printf ("  pixel format: %s  number of planes: %u\n", pixfmt_str, nplanes);
//     printf ("  video meta found: %s\n", yesno (meta != NULL));
//     printf ("===================================\n");
//   }

//   gst_buffer_unmap (buf, &map_info);

//   return;
// }

// static void *
// video_frame_loop (void *arg)
// {
//   struct CustomData *dec = (struct CustomData *) arg;

//   // gst_app_sink_set_max_buffers (GST_APP_SINK(dec->appsink),1);
//   // gst_app_sink_set_buffer_list_support (GST_APP_SINK(dec->appsink),FALSE);

//   do {
//     GstSample *samp;
//     GstBuffer *buf;

//     samp = gst_app_sink_pull_sample (GST_APP_SINK (dec->appsink));
//     // samp = gst_app_sink_try_pull_sample (GST_APP_SINK (dec->appsink),100000);
//     if (!samp) {
//       GST_DEBUG ("got no appsink sample");
//       if (gst_app_sink_is_eos (GST_APP_SINK (dec->appsink)))
//         GST_DEBUG ("eos");
//       return((void *)0);
//     }

//     buf = gst_sample_get_buffer (samp);
//     buffer_to_file (dec, buf);

//     gst_sample_unref (samp);
//     dec->frame++;

//     // sleep(2);

//   } while (1);

//   return((void *)0);

// }

// rtsp init
static int
rtsp_init(struct CustomData *data) {

    /* Build Pipeline */
    data->pipeline = gst_pipeline_new(std::to_string(data->m_Id).c_str());
    g_print(("rtsp_init rtspsrc" + std::to_string(data->m_Id) + "\n").c_str());
    data->rtspsrc = gst_element_factory_make ( "rtspsrc", ("rtspsrc" + std::to_string(data->m_Id)).c_str());
    data->decode  = gst_element_factory_make ( "decodebin", ("decodebin"+ std::to_string(data->m_Id)).c_str());
    //data->mppdec = gst_element_factory_make ( "mppvideodec", "mppvideodec0");
    data->tee    = gst_element_factory_make ( "tee", ("tee"+ std::to_string(data->m_Id)).c_str());

    data->queue_appsink      = gst_element_factory_make ( "queue", ("queue_appsink"+ std::to_string(data->m_Id)).c_str());
    data->appsink            = gst_element_factory_make ( "appsink", ("app_sink"+ std::to_string(data->m_Id)).c_str());

    if ( !data->pipeline || !data->rtspsrc | !data->decode || !data->tee || !data->queue_appsink || !data->appsink) {
        g_printerr("One element could not be created.\n");
    }
    // Config appsink
    g_object_set (G_OBJECT (data->appsink), "sync", FALSE, NULL);
    /* Implement the allocation query using a pad probe. This probe will
    * adverstize support for GstVideoMeta, which avoid hardware accelerated
    * decoder that produce special strides and offsets from having to
    * copy the buffers.
    */
    GstPad * apppad = gst_element_get_static_pad (data->appsink, "sink");
    gst_pad_add_probe (apppad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
        appsink_query_cb, NULL, NULL);
    gst_object_unref (apppad);

    gst_base_sink_set_max_lateness (GST_BASE_SINK (data->appsink), 70 * GST_MSECOND);
    gst_base_sink_set_qos_enabled (GST_BASE_SINK (data->appsink), TRUE);

    g_object_set (G_OBJECT (data->appsink), "max-buffers", 1, NULL);

    gst_pad_add_probe (gst_element_get_static_pad (data->appsink, "sink"),
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, pad_probe, data, NULL);
    g_object_set (G_OBJECT (data->appsink), "emit-signals", TRUE, NULL );
    // g_signal_connect ( G_OBJECT (data->appsink), "new-sample", G_CALLBACK (new_sample), data->pipeline);

    //create_uri(url,url_size, ip_address, port);
    g_print("rtsp_uri:%s\n",data->m_RtspUri);
    g_object_set(GST_OBJECT(data->rtspsrc), "location", data->m_RtspUri, NULL);
    g_object_set (GST_OBJECT (data->rtspsrc), "latency", 2000, NULL);
    // g_object_set (GST_OBJECT (data->rtspsrc), "timeout", 1000000, NULL);
    // g_object_set (GST_OBJECT (data->rtspsrc), "udp-reconnect", 1, NULL);
    // g_object_set (GST_OBJECT (data->rtspsrc), "retry", 1000, NULL);
    // g_object_set (GST_OBJECT (data->rtspsrc), "debug", 1, NULL);
    if (data->conn_Mode == 1) {
    	g_object_set (GST_OBJECT (data->rtspsrc), "protocols", GST_RTSP_LOWER_TRANS_TCP, NULL);
    }else if ( data->conn_Mode == 2){
        g_object_set (GST_OBJECT (data->rtspsrc), "protocols", GST_RTSP_LOWER_TRANS_UDP, NULL);
    } else {
        g_object_set (GST_OBJECT (data->rtspsrc), "protocols", GST_RTSP_LOWER_TRANS_UNKNOWN, NULL);
    }

    /**
     * GstRTSPLowerTrans:
     * @GST_RTSP_LOWER_TRANS_UNKNOWN: invalid transport flag
     * @GST_RTSP_LOWER_TRANS_UDP: stream data over UDP
     * @GST_RTSP_LOWER_TRANS_UDP_MCAST: stream data over UDP multicast
     * @GST_RTSP_LOWER_TRANS_TCP: stream data over TCP
     * @GST_RTSP_LOWER_TRANS_HTTP: stream data tunneled over HTTP.
     * @GST_RTSP_LOWER_TRANS_TLS: encrypt TCP and HTTP with TLS
     *
     * The different transport methods.
     */
    // g_object_set (GST_OBJECT (data->rtspsrc), "protocols", GST_RTSP_LOWER_TRANS_HTTP, NULL);
    //"rtsp://<ip>:554/live/ch00_0"

    g_object_set (G_OBJECT (data->queue_appsink), "max-size-buffers", 1, NULL);
    // g_object_set (G_OBJECT (data->queue_appsink), "max-size-bytes", 1000, NULL);
    g_object_set (G_OBJECT (data->queue_appsink), "leaky", 2, NULL);
    gst_app_sink_set_max_buffers (GST_APP_SINK(data->appsink),1);
    gst_app_sink_set_buffer_list_support (GST_APP_SINK(data->appsink),FALSE);

    gst_bin_add_many(GST_BIN(data->pipeline), data->rtspsrc, data->decode, data->tee, NULL);
    // gst_bin_add_many(GST_BIN(data->pipeline), data->queue_appsink, data->appsink, NULL);
    // gst_bin_add_many(GST_BIN(data->pipeline), data->queue_appsink, data->videoconvert, data->appsink, NULL);
    gst_bin_add(GST_BIN(data->pipeline), data->queue_appsink);
    gst_bin_add(GST_BIN(data->pipeline), data->appsink);// queue -> rkximagesink  ||  queue -> appsink
    if (!gst_element_link_many (data->queue_appsink, data->appsink, NULL)) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data->pipeline);
        return -1;
    }

    //tee -> queue1 -> queue2
    GstPad * queue2_video_pad = gst_element_get_static_pad ( data->queue_appsink, "sink");
    GstPad * tee2_video_pad = gst_element_get_request_pad ( data->tee, "src_%u");
    if (gst_pad_link ( tee2_video_pad, queue2_video_pad) != GST_PAD_LINK_OK) {
        g_printerr ("tee link queue error. \n");
        gst_object_unref (data->pipeline);
        return -1;
    }
    gst_object_unref (queue2_video_pad);
    gst_object_unref (tee2_video_pad);

    g_signal_connect(data->rtspsrc, "pad-added", G_CALLBACK( on_src_decodebin_added), data);
    g_signal_connect(data->decode, "pad-added", G_CALLBACK( on_src_tee_added), data);
    
    data->bus = gst_pipeline_get_bus( GST_PIPELINE (data->pipeline));
    gst_bus_add_watch (data->bus, bus_watch_cb, data);
    //gst_object_unref ( GST_OBJECT (bus));

    // bus = gst_element_get_bus( GST_ELEMENT (data->appsink));
    // gst_bus_add_watch (bus, (GstBusFunc) bus_uridecodebin_cb, data);
    // gst_object_unref ( GST_OBJECT (bus));

    // start playing
    printf ("start playing \n");
    gst_element_set_state (GST_ELEMENT(data->pipeline), GST_STATE_PLAYING);

    //g_main_loop_run (main_loop);
    // pthread_create (&data->gst_thread, NULL, video_frame_loop, data);
    
    return 1;
}

// destroy
static void 
rtsp_destroy (struct CustomData *data)
{
  if (data != NULL) {
    try{
        if (data->loop != NULL) {
    	   g_main_loop_quit (data->loop);
        }

        if (data->pipeline != NULL) {    
           gst_element_set_state (GST_ELEMENT(data->pipeline), GST_STATE_PAUSED);
           gst_element_set_state (GST_ELEMENT(data->pipeline), GST_STATE_NULL);
        }
        printf ("start gst_object_unref element \n");
        if (data->bus != NULL) 
        {
            gst_bus_remove_watch (data->bus);
            gst_object_unref ( GST_OBJECT (data->bus));
            data->bus = NULL;
        }

        try{
          gst_element_unlink (data->appsink, data->queue_appsink);
          gst_bin_remove_many(GST_BIN(data->pipeline), data->rtspsrc, data->decode, data->tee, NULL);
          gst_bin_remove_many(GST_BIN(data->pipeline), data->queue_appsink, data->appsink, NULL);
          data->rtspsrc = NULL;
          data->decode = NULL;
          data->tee = NULL;
          data->queue_appsink = NULL;
          data->appsink = NULL;
        } catch (...) 
        {
          printf("rtsp_destroy error for gst_bin_remove_many \n");
        }

        printf("finish bus unref \n");
        if (data->pipeline != NULL)
        {
            gst_object_unref (data->pipeline);
            data->pipeline = NULL;
        }
        printf("finish pipeline unref \n");
        if (data->loop != NULL) {
            g_main_loop_unref (data->loop);
            data->loop = NULL;
        }
        printf("finish loop unref \n");
        //pthread_join(m_thread,NULL);
    } catch (...) 
    {
       printf("rtsp_destroy error for gstreamer \n");
    }

    try {
        if (data->m_RtspUri != NULL){
    	    free (data->m_RtspUri);
          data->m_RtspUri = NULL;
        }
    } catch (...) 
    {
        printf("mRtspUri free fail \n");
    }
    
    printf("finish free m_RtspUri \n");
    
    try {
        if (data->dst_buf != NULL)
        {
           free (data->dst_buf);
           data->dst_buf = NULL;
        }
        if (data->dst_output_buf != NULL)
        {
           free (data->dst_output_buf);
           data->dst_output_buf = NULL;
        }
        if (data->dst_resize_output_buf != NULL)
        {
           free (data->dst_resize_output_buf);
           data->dst_resize_output_buf = NULL;
        }
        if (data->dst_output_resize_buf != NULL)
        {
           free (data->dst_output_resize_buf);
           data->dst_output_resize_buf = NULL;
        }
        if (data->dst_resize_output_resize_buf != NULL)
        {
           free (data->dst_resize_output_resize_buf);
           data->dst_resize_output_resize_buf = NULL;
	}
    } catch (...) {
        printf("rtsp_destroy error for bufi \n");
    }

    printf("rtsp_destory data = null \n");
    data->isRun = STATUS_DISCONNECT;
    
  } else {
    printf("rtspDestory data== NULL \n");
  }

}

RtspClient::RtspClient() {
    printf("rtsp rtspclient\n");
}

RtspClient::~RtspClient() {
    printf("rtsp ~rtspclient!\n");
}

int rtsp_check_url(char const* url)
{
#define RTSP_URL_PREFIX "rtsp://"

    int pos = sizeof(RTSP_URL_PREFIX)-1;

    // Parse URL, get configuration
    // the url MUST start with "rtsp://"
    if (strncmp(url, RTSP_URL_PREFIX, pos) != 0) {
        printf("URL not started with \"rtsp://\".\n");
        return -1;
    }

    return 1;
}

// connect rtsp
static void* connectrtsp(void *arg) {

  // int p = fork();
  struct CustomData *data = (struct CustomData *)arg;

  printf("connectrtsp m_RtspUri %s \n", data->m_RtspUri);
  
  int ret = -1;
  try {	
      ret = rtsp_check_url(data->m_RtspUri);
  } catch (...) {
      printf("connectrtsp mRtspUri check URI fail !");
      data->isRun = STATUS_DISCONNECT;
      pthread_detach(pthread_self());
      printf("pthread_exit rtsp_check_url exit\n");
      pthread_exit(0);
      return NULL;
  }
  printf("connectrtsp m_RtspUri check ret %d \n", ret);
  if (ret == 1) {
      // gst init
      // gst_init (NULL, NULL);
      /** gst debug */
      // GST_DEBUG_CATEGORY_INIT (rk_appsink_debug, "rk_appsink", 2, "App sink");
      printf("mId %d mRtspUri %s \n", data->m_Id, data->m_RtspUri);

      data->isRun = STATUS_CONNECTING;
      int ret = 1;
      try {
         ret = rtsp_init (data);
      } catch (...){
         data->isRun = STATUS_DISCONNECT;
         rtsp_destroy(data);
         pthread_detach(pthread_self());
         printf("pthread_exit rtsp_init exit\n");
         pthread_exit(0);
         return NULL;
      }
      if ( ret == -1)
      {
         data->isRun = STATUS_DISCONNECT;
         rtsp_destroy(data);
         pthread_detach(pthread_self());
         printf("pthread_exit rtsp_init return fail exit\n");
         pthread_exit(0);
         return NULL;
      }
      data->loop = g_main_loop_new (NULL, FALSE);
      g_main_loop_run (data->loop);

      printf("init exit mId %d \n", data->m_Id);
      data->isRun = STATUS_DISCONNECT;
      rtsp_destroy(data);
      pthread_detach(pthread_self());
      printf("pthread_exit loop exit\n");
      pthread_exit(0);
      return NULL;
    
  }

  data->isRun = STATUS_DISCONNECT;
  rtsp_destroy(data);
  pthread_detach(pthread_self());
  printf("pthread_exit url check fail\n");
  pthread_exit(0);
  return NULL;

}

bool
RtspClient::changeURL(int id, const char* url, int conn_mode) {

  // g_print("RtspClient enable ! %d \n", id);
  if (this->m_data.isRun == STATUS_CONNECTED) {
    printf("enable fail, rtsp thread running \n");
    return FALSE;
  }
  if (this->m_data.m_RtspUri != NULL){
    free(this->m_data.m_RtspUri);
    this->m_data.m_RtspUri = NULL;
  }
  if (this->m_data.dst_buf != NULL){
    free(this->m_data.dst_buf);
    this->m_data.dst_buf = NULL;
  }
  if (this->m_data.dst_output_buf != NULL){
    free(this->m_data.dst_output_buf);
    this->m_data.dst_output_buf = NULL;
  }
  if (this->m_data.dst_resize_output_buf != NULL){
    free(this->m_data.dst_resize_output_buf);
    this->m_data.dst_resize_output_buf = NULL;
  }
  if (this->m_data.dst_resize_output_resize_buf != NULL){
    free(this->m_data.dst_resize_output_resize_buf);
    this->m_data.dst_resize_output_resize_buf = NULL;
  }
  if (this->m_data.dst_output_resize_buf != NULL){
    free(this->m_data.dst_output_resize_buf);
    this->m_data.dst_output_resize_buf = NULL;
  }
  this->m_data.conn_Mode = conn_mode;
  size_t len = strlen(url) + 1;
  this->m_data.m_RtspUri = (char*)malloc(len);
  memset(this->m_data.m_RtspUri, 0, len);
  memcpy(this->m_data.m_RtspUri, url, len);
  printf("changeURL RtspClient mRtspUri %s \n",this->m_data.m_RtspUri);
  g_object_set(GST_OBJECT(this->m_data.rtspsrc), "location", this->m_data.m_RtspUri, NULL);
  g_object_set (GST_OBJECT (this->m_data.rtspsrc), "latency", 2000, NULL);
  //this->m_data->isRun = STATUS_CONNECTING;
  sleep(2);
  gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PAUSED);
  gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_READY);
  gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PLAYING);
  return TRUE;

}

bool
RtspClient::reConnect(int id) {

  // g_print("RtspClient enable ! %d \n", id);
  if (this->m_data.isRun != STATUS_CONNECTED) {
      sleep(2);
      gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PAUSED);
      gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_READY);
      gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PLAYING);
  } else {
      printf("enable fail, reConnect rtsp thread running \n");
      return FALSE;
  } 
  return TRUE;
}

// start create sync
bool
RtspClient::enable(int id, const char* url, int conn_mode) {

  printf("RtspClient enable ! %d \n", id);

  if (this->m_data.isRun != STATUS_CONNECTING) {
      //this.m_data = g_new0 (struct CustomData, 1);
      this->m_data.m_Id = id;
      this->m_data.conn_Mode = conn_mode;
      size_t len = strlen(url) + 1;
      this->m_data.m_RtspUri = (char*)malloc(len);
      memset(this->m_data.m_RtspUri, 0, len);
      memcpy(this->m_data.m_RtspUri, url, len);
      printf("RtspClient mRtspUri %s \n",this->m_data.m_RtspUri);
      this->m_data.isRun = STATUS_CONNECTING;
      int ret = pthread_create(&m_thread, NULL, connectrtsp, &this->m_data);
      if ( ret != 0) {
          printf("enable fail, rtsp thread create fail \n");
          return false;
      }
  } else {
      printf("enable fail, rtsp thread running \n");
      return false;
  } 

  return true;

}

// destroy instance
void 
RtspClient::disable() {
    g_print("RtspClient start ~disable! mId %d \n",this->m_data.m_Id);
    // g_main_loop_quit(this->m_data->loop);
    if (this->m_data.isRun == STATUS_CONNECTED){
      rtsp_destroy(&this->m_data);
      this->m_data.isRun = STATUS_DISCONNECT;
    }
    try{
        pthread_cancel(this->m_thread);
    } catch (...) {
        printf("pthread_cancel error \n");
    }
    pthread_join(this->m_thread, NULL);
    sleep(1);
    printf("RtspClient ~disable! end mId %d \n", this->m_data.m_Id);
}

// connect status
int RtspClient::isConnect() 
{
  return this->m_data.isRun;
}

// read frame
struct FrameData *
RtspClient::read_Rga(int width, int height, int resize_width, int resize_height) {

  FrameData * data = new FrameData();

  if ( this->m_data.isRun == STATUS_DISCONNECT){
    data->isRun = STATUS_DISCONNECT;
    data->size = 0;
    return data;
  }

  try { 
 
    data->isRun = this->m_data.isRun;
    data->size = 0;
    data->width = this->m_data.info.width;
    data->height = this->m_data.info.height;

    GstSample *samp;
    GstBuffer *buf;
  
    // samp = gst_app_sink_pull_sample (GST_APP_SINK (this->m_data->appsink));
    samp = gst_app_sink_try_pull_sample (GST_APP_SINK (this->m_data.appsink), GST_SECOND * 3); // 1000 * 5 100000
    if (!samp) {
      GST_DEBUG ("got no appsink sample");
      if (gst_app_sink_is_eos (GST_APP_SINK (this->m_data.appsink))){
        GST_DEBUG ("eos");
        //g_main_loop_quit(this->m_data.loop);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PAUSED);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_READY);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PLAYING);
        sleep(2);
        data->isRun = STATUS_DISCONNECT;
        data->size = 0;
        this->m_data.frame=0; 
        return data;
      }else{
        GST_DEBUG ("gst_app_sink_try_pull_sample null");
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PAUSED);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_READY);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PLAYING);
        sleep(2);
        data->isRun = STATUS_DISCONNECT;
        data->size = 0;
        this->m_data.frame=0; 
        return data;
      }
    }

    buf = gst_sample_get_buffer (samp);
    
    // ** 
    int ret;
    GstVideoMeta *meta = gst_buffer_get_video_meta (buf);
    guint nplanes = GST_VIDEO_INFO_N_PLANES (&(this->m_data.info));
    // guint width, height;
    GstMapInfo map_info;
    gchar filename[128];
    GstVideoFormat pixfmt;
    const char *pixfmt_str;

    pixfmt = GST_VIDEO_INFO_FORMAT (&(this->m_data.info));
    pixfmt_str = gst_video_format_to_string (pixfmt);

    /* TODO: use the DMABUF directly */
    // gst_buffer_map (buf, &map_info, GST_MAP_READ);
    // gst_buffer_map (buf, &map_info, GST_MAP_READ);
    if (!gst_buffer_map (buf, &map_info, (GstMapFlags)GST_MAP_READ))
    {
      g_print ("gst_buffer_map() error! \n ");
      data->isRun = STATUS_DISCONNECT;
      data->size = 0;
      return data;
    }

    int source_width = GST_VIDEO_INFO_WIDTH (&(this->m_data.info));
    int source_height = GST_VIDEO_INFO_HEIGHT (&(this->m_data.info));

    if (this->m_data.last_time_width == 0 || this->m_data.last_time_hetight == 0) {
      this->m_data.last_time_width = source_width;
      this->m_data.last_time_hetight = source_height;
    }

    if (this->m_data.last_time_width != source_height && this->m_data.last_time_hetight != source_width) {
        if (this->m_data.dst_buf != NULL){
          free(this->m_data.dst_buf);
          this->m_data.dst_buf = NULL;
        }
        if (this->m_data.dst_output_buf != NULL){
          free(this->m_data.dst_output_buf);
          this->m_data.dst_output_buf = NULL;
        }
        if (this->m_data.dst_resize_output_buf != NULL){
          free(this->m_data.dst_resize_output_buf);
          this->m_data.dst_resize_output_buf = NULL;
        }
        if (this->m_data.dst_resize_output_resize_buf != NULL){
          free(this->m_data.dst_resize_output_resize_buf);
          this->m_data.dst_resize_output_resize_buf = NULL;
        }
        if (this->m_data.dst_output_resize_buf != NULL){
          free(this->m_data.dst_output_resize_buf);
          this->m_data.dst_output_resize_buf = NULL;
        }
    }

    /* output some information at the beginning (= when the first frame is handled) */
    if (this->m_data.frame == 0) {
      printf ("===================================\n");
      printf ("GStreamer video stream information:\n");
      printf ("  size: %u x %u pixel\n", source_width, source_height);
      printf ("  pixel format: %s  number of planes: %u\n", pixfmt_str, nplanes);
      printf ("  video meta found: %s\n", yesno (meta != NULL));
      printf ("===================================\n");
      printf ("mpp frame size : %d \n", map_info.size);
    }
    
    // g_print("mpp frame size : %d \n", map_info.size);
    //g_print("format %f \n",get_bpp_from_format(RK_FORMAT_YCrCb_420_SP));

    // rga
    rga_buffer_t 	src;
    rga_buffer_t 	dst;
    rga_buffer_t  dst_output;
    rga_buffer_t  dst_resize_output;
    rga_buffer_t  dst_two_resize_output;
    rga_buffer_t  dst_two_output;

    // h265 256bit 1920 * 1080  == 2304 * 1080
    if (map_info.size == 3732480 || map_info.size == 4976640 || map_info.size == 3760128 ) {
      src = wrapbuffer_virtualaddr((char *) map_info.data, 2304, 1080, SRC_FORMAT);
      if (this->m_data.dst_buf == NULL){
        this->m_data.dst_buf = (char*)malloc(2304*1080*get_bpp_from_format(DST_FORMAT));
      }
      dst = wrapbuffer_virtualaddr(this->m_data.dst_buf, 2304, 1080, DST_FORMAT);
      if (this->m_data.dst_output_buf == NULL){
          this->m_data.dst_output_buf = (char*)malloc(1920*1080*get_bpp_from_format(DST_FORMAT));
      }
      dst_output = wrapbuffer_virtualaddr(this->m_data.dst_output_buf, 1920, 1080, DST_FORMAT);
      if (this->m_data.dst_resize_output_buf == NULL){
          this->m_data.dst_resize_output_buf = (char*)malloc(width*height*get_bpp_from_format(DST_FORMAT));
      }
      dst_resize_output = wrapbuffer_virtualaddr(this->m_data.dst_resize_output_buf, width, height,DST_FORMAT);
      if(src.width == 0 || dst.width == 0 || dst_output.width == 0) {
        printf("%s, %s\n", __FUNCTION__, imStrError());
        // return data;
      } else {
        // g_print("1080 h265 imcvtcolor \n");
        imcvtcolor(src, dst, src.format, dst.format);
        im_rect src_rect = {0, 0, 1920, 1080};
        //g_print("imcrop %d",src_rect.width);
        imcrop(dst,dst_output,src_rect);

        imresize(dst_output,dst_resize_output);

        data->data = this->m_data.dst_resize_output_buf;
        data->size = width*height*get_bpp_from_format(DST_FORMAT);
    }

    } else 
    // h264 16bit 1920 * 1080 == 1920 * 1088
    if (map_info.size == 3133440 || map_info.size == 4177920 ) {  
      src = wrapbuffer_virtualaddr((char *) map_info.data, 1920, 1088, SRC_FORMAT);
      if (this->m_data.dst_buf == NULL){
          this->m_data.dst_buf = (char*)malloc(1920*1088*get_bpp_from_format(DST_FORMAT));
      }
      dst = wrapbuffer_virtualaddr(this->m_data.dst_buf, 1920, 1088, DST_FORMAT);
      if (this->m_data.dst_output_buf == NULL){
          this->m_data.dst_output_buf = (char*)malloc(1920*1080*get_bpp_from_format(DST_FORMAT));
      }
      dst_output = wrapbuffer_virtualaddr(this->m_data.dst_output_buf, 1920, 1080, DST_FORMAT);
      if (this->m_data.dst_resize_output_buf == NULL){
          this->m_data.dst_resize_output_buf = (char*)malloc(width*height*get_bpp_from_format(DST_FORMAT));
      }
      dst_resize_output = wrapbuffer_virtualaddr(this->m_data.dst_resize_output_buf, width, height, DST_FORMAT);
      if(src.width == 0 || dst.width == 0 || dst_output.width == 0) {
        printf("%s, %s\n", __FUNCTION__, imStrError());
        // return data;
      } else {
        // g_print("1080 h264 imcvtcolor \n");
        imcvtcolor(src, dst, src.format, dst.format);
        im_rect src_rect = {0, 0, 1920, 1080};
        //g_print("imcrop %d",src_rect.width);
        imcrop(dst,dst_output,src_rect);

        imresize(dst_output,dst_resize_output);

        data->width = width;
        data->height = height;
        data->data = this->m_data.dst_resize_output_buf;
        data->size = width*height*get_bpp_from_format(DST_FORMAT);
      }

    } else
    // h265 2560 * 1440 256 2816 1584
    if (map_info.size == 6082560 || map_info.size == 8110080) {
      src = wrapbuffer_virtualaddr((char *) map_info.data, 2816, 1440, SRC_FORMAT);
      if (this->m_data.dst_buf == NULL){
          this->m_data.dst_buf = (char*)malloc(2816*1440*get_bpp_from_format(DST_FORMAT));
      }
      dst = wrapbuffer_virtualaddr(this->m_data.dst_buf, 2816, 1440, DST_FORMAT);
      if (this->m_data.dst_output_buf == NULL){
          this->m_data.dst_output_buf = (char*)malloc(2560*1440*get_bpp_from_format(DST_FORMAT));
      }
      dst_output = wrapbuffer_virtualaddr(this->m_data.dst_output_buf, 2560, 1440, DST_FORMAT);
      if (this->m_data.dst_resize_output_buf == NULL){
          this->m_data.dst_resize_output_buf = (char*)malloc(width*height*get_bpp_from_format(DST_FORMAT));
      }
      dst_resize_output = wrapbuffer_virtualaddr(this->m_data.dst_resize_output_buf, width, height, DST_FORMAT);
      if(src.width == 0 || dst.width == 0 || dst_output.width == 0) {
        printf("%s, %s\n", __FUNCTION__, imStrError());
        // return data;
      } else {
        // g_print("1080 h264 imcvtcolor \n");
        imcvtcolor(src, dst, src.format, dst.format);
        im_rect src_rect = {0, 0, 2560, 1440};
        //g_print("imcrop %d",src_rect.width);
        imcrop(dst,dst_output,src_rect);

        imresize(dst_output,dst_resize_output);

        data->width = width;
        data->height = height;
        data->data = this->m_data.dst_resize_output_buf;
        data->size = width*height*get_bpp_from_format(DST_FORMAT);
      }
      
    } else
    //h265 640*480 768 * 480 
    if (map_info.size == 552960) {
      src = wrapbuffer_virtualaddr((char *) map_info.data, 768, 480, SRC_FORMAT);
      if (this->m_data.dst_buf == NULL){
          this->m_data.dst_buf = (char*)malloc(768*480*get_bpp_from_format(DST_FORMAT));
      }
      dst = wrapbuffer_virtualaddr(this->m_data.dst_buf, 768, 480, DST_FORMAT);
      if (this->m_data.dst_output_buf == NULL){
          this->m_data.dst_output_buf = (char*)malloc(640*480*get_bpp_from_format(DST_FORMAT));
      }
      dst_output = wrapbuffer_virtualaddr(this->m_data.dst_output_buf, 640, 480, DST_FORMAT);
      if (this->m_data.dst_resize_output_buf == NULL){
          this->m_data.dst_resize_output_buf = (char*)malloc(width*height*get_bpp_from_format(DST_FORMAT));
      }
      dst_resize_output = wrapbuffer_virtualaddr(this->m_data.dst_resize_output_buf, width, height, DST_FORMAT);
      if(src.width == 0 || dst.width == 0 || dst_output.width == 0) {
        printf("%s, %s\n", __FUNCTION__, imStrError());
        // return data;
      } else {
        // g_print("1080 h264 imcvtcolor \n");
        imcvtcolor(src, dst, src.format, dst.format);
        im_rect src_rect = {0, 0, 640, 480};
        //g_print("imcrop %d",src_rect.width);
        imcrop(dst,dst_output,src_rect);

        imresize(dst_output,dst_resize_output);

        data->width = width;
        data->height = height;
        data->data = this->m_data.dst_resize_output_buf;
        data->size = width*height*get_bpp_from_format(DST_FORMAT);
      }

    }
    else 
    // h265 h264
    // supoort 1280*720 3840*2160 
    // h264 h265 3840*2160 12441600 12441600
    {
        
        src = wrapbuffer_virtualaddr((char *) map_info.data, source_width, source_height, SRC_FORMAT);
        if (this->m_data.dst_buf == NULL){
            this->m_data.dst_buf = (char*)malloc(source_width*source_height*get_bpp_from_format(DST_FORMAT));
        }
        dst = wrapbuffer_virtualaddr(this->m_data.dst_buf, source_width, source_height, DST_FORMAT);
        if (this->m_data.dst_output_buf == NULL){
            this->m_data.dst_output_buf = (char*)malloc(source_width*source_height*get_bpp_from_format(DST_FORMAT));
        }
        dst_output = wrapbuffer_virtualaddr(this->m_data.dst_output_buf, source_width, source_height, DST_FORMAT);
        if (this->m_data.dst_resize_output_buf == NULL){
            this->m_data.dst_resize_output_buf = (char*)malloc(width*height*get_bpp_from_format(DST_FORMAT));
        }
        dst_resize_output = wrapbuffer_virtualaddr(this->m_data.dst_resize_output_buf, width, height, DST_FORMAT);
        if(src.width == 0 || dst.width == 0 || dst_resize_output.width == 0) {
          printf("%s, %s\n", __FUNCTION__, imStrError());
          // return data;
        } else {

          imcvtcolor(src, dst, src.format, dst.format);
          imresize(dst,dst_resize_output);

          data->width = width;
          data->height = height;
          data->data = this->m_data.dst_resize_output_buf;
          data->size = width*height*get_bpp_from_format(DST_FORMAT);

        }

    }

    if (resize_width > 0 and resize_height > 0){
      // scale 640 resize data
      if (this->m_data.dst_resize_output_resize_buf == NULL){
          this->m_data.dst_resize_output_resize_buf = (char*)malloc(resize_width*resize_height*get_bpp_from_format(DST_FORMAT));
      }
      if (this->m_data.dst_output_resize_buf == NULL){
          this->m_data.dst_output_resize_buf = (char*)malloc(resize_width*resize_height*get_bpp_from_format(DST_FORMAT));
      }
      dst_two_resize_output = wrapbuffer_virtualaddr(this->m_data.dst_resize_output_resize_buf, resize_width, resize_height, DST_FORMAT);
      dst_two_output = wrapbuffer_virtualaddr(this->m_data.dst_output_resize_buf, resize_width, resize_height, DST_FORMAT);
      if(src.width == 0 || dst.width == 0 || dst_two_resize_output.width == 0) {
        printf("%s, %s\n", __FUNCTION__, imStrError());
      }else{
        imresize(dst, dst_two_resize_output, (double(resize_width)/width), (double(resize_height)/width));
        imtranslate(dst_two_resize_output, dst_two_output, 0, int((resize_height - (height * (double(resize_height)/width)))/2));
        data->data_resize = this->m_data.dst_output_resize_buf;
        data->size_resize = resize_width * resize_height * get_bpp_from_format(DST_FORMAT);
      }
    }

    gst_buffer_unmap (buf, &map_info);
    // ** 
    gst_sample_unref (samp);

    this->m_data.frame++; 

  } catch (...) {
    data->isRun = STATUS_DISCONNECT;
    data->size = 0;
    return data;
  }

  return data;

}

// read frame
struct FrameData *
RtspClient::read_Opencv() {

  FrameData * data = new FrameData();

  if ( this->m_data.isRun == STATUS_DISCONNECT){
    data->isRun = STATUS_DISCONNECT;
    data->size = 0;
    return data;
  }

  try { 
 
    data->isRun = this->m_data.isRun;
    data->size = 0;
    data->width = this->m_data.info.width;
    data->height = this->m_data.info.height;

    GstSample *samp;
    GstBuffer *buf;
  
    // samp = gst_app_sink_pull_sample (GST_APP_SINK (this->m_data->appsink));
    samp = gst_app_sink_try_pull_sample (GST_APP_SINK (this->m_data.appsink), GST_SECOND * 3); // 1000 * 5 100000
    if (!samp) {
      GST_DEBUG ("got no appsink sample");
      if (gst_app_sink_is_eos (GST_APP_SINK (this->m_data.appsink))){
        GST_DEBUG ("eos");
        //g_main_loop_quit(this->m_data.loop);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PAUSED);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_READY);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PLAYING);
        sleep(2);
        data->isRun = STATUS_DISCONNECT;
        data->size = 0;
        this->m_data.frame=0; 
        return data;
      }else{
        GST_DEBUG ("gst_app_sink_try_pull_sample null");
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PAUSED);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_READY);
        gst_element_set_state (GST_ELEMENT(this->m_data.pipeline), GST_STATE_PLAYING);
        sleep(2);
        data->isRun = STATUS_DISCONNECT;
        data->size = 0;
        this->m_data.frame=0; 
        return data;
      }
    }

    buf = gst_sample_get_buffer (samp);
    
    // ** 
    int ret;
    GstVideoMeta *meta = gst_buffer_get_video_meta (buf);
    guint nplanes = GST_VIDEO_INFO_N_PLANES (&(this->m_data.info));
    // guint width, height;
    GstMapInfo map_info;
    gchar filename[128];
    GstVideoFormat pixfmt;
    const char *pixfmt_str;

    pixfmt = GST_VIDEO_INFO_FORMAT (&(this->m_data.info));
    pixfmt_str = gst_video_format_to_string (pixfmt);

    /* TODO: use the DMABUF directly */
    // gst_buffer_map (buf, &map_info, GST_MAP_READ);
    // gst_buffer_map (buf, &map_info, GST_MAP_READ);
    if (!gst_buffer_map (buf, &map_info, (GstMapFlags)GST_MAP_READ))
    {
      g_print ("gst_buffer_map() error! \n");
      data->isRun = STATUS_DISCONNECT;
      data->size = 0;
      return data;
    }

    int source_width = GST_VIDEO_INFO_WIDTH (&(this->m_data.info));
    int source_height = GST_VIDEO_INFO_HEIGHT (&(this->m_data.info));

    if (this->m_data.last_time_width == 0 || this->m_data.last_time_hetight == 0) {
      this->m_data.last_time_width = source_width;
      this->m_data.last_time_hetight = source_height;
    }

    if (this->m_data.last_time_width != source_height && this->m_data.last_time_hetight != source_width) {
        if (this->m_data.dst_buf != NULL){
          free(this->m_data.dst_buf);
          this->m_data.dst_buf = NULL;
        }
        if (this->m_data.dst_output_buf != NULL){
          free(this->m_data.dst_output_buf);
          this->m_data.dst_output_buf = NULL;
        }
        if (this->m_data.dst_resize_output_buf != NULL){
          free(this->m_data.dst_resize_output_buf);
          this->m_data.dst_resize_output_buf = NULL;
        }
        if (this->m_data.dst_resize_output_resize_buf != NULL){
          free(this->m_data.dst_resize_output_resize_buf);
          this->m_data.dst_resize_output_resize_buf = NULL;
        }
        if (this->m_data.dst_output_resize_buf != NULL){
          free(this->m_data.dst_output_resize_buf);
          this->m_data.dst_output_resize_buf = NULL;
        }
    }

    /* output some information at the beginning (= when the first frame is handled) */
    if (this->m_data.frame == 0) {
      printf ("===================================\n");
      printf ("GStreamer video stream information:\n");
      printf ("  size: %u x %u pixel\n", source_width, source_height);
      printf ("  pixel format: %s  number of planes: %u\n", pixfmt_str, nplanes);
      printf ("  video meta found: %s\n", yesno (meta != NULL));
      printf ("===================================\n");
      printf ("mpp frame size : %d \n", map_info.size);
    }
    
    // g_print("mpp frame size : %d \n", map_info.size);
    // g_print("format %f \n",get_bpp_from_format(RK_FORMAT_YCrCb_420_SP));

    // opencv
    // NV12/NV21说明:
    // NV12: YYYYYYYY UVUV     =>YUV420SP  -> RK_FORMAT_YCbCr_420_SP
    // NV21: YYYYYYYY VUVU     =>YUV420SP    -> RK_FORMAT_YCrCb_420_SP
      
    if (this->m_data.dst_buf == NULL){
          this->m_data.dst_buf = (char*)malloc(map_info.size);
    }
    memcpy(this->m_data.dst_buf, map_info.data, map_info.size);

    // TODO mpp没问题
    // h265 256bit 1920 * 1080  == 2304 * 1080
    if (map_info.size == 3732480 || map_info.size == 4976640) {
      data->width = 2304;
      data->height = 1080;
      data->data = this->m_data.dst_buf;
      data->size = map_info.size;
    } else if (map_info.size == 3760128) {
      data->width = 2304;
      data->height = 1088;
      data->data = this->m_data.dst_buf;
      data->size = map_info.size;
    }
    else 
    // // h264 16bit 1920 * 1080 == 1920 * 1088
    if (map_info.size == 3133440 || map_info.size == 4177920 ) { 
      data->width = 1920;
      data->height = 1088;
      data->data = this->m_data.dst_buf;
      data->size = map_info.size;
    }
    else
    // // h265 2560 * 1440 256 2816 1584
    if (map_info.size == 6082560 || map_info.size == 8110080) {
      data->width = 2816;
      data->height = 1584;
      data->data = this->m_data.dst_buf;
      data->size = map_info.size;
    }
    else
    // //h265 640*480 768 * 480 
    if (map_info.size == 552960) {
      data->width = 640;
      data->height = 480;
      data->data = this->m_data.dst_buf;
      data->size = map_info.size;
    } else 
    // h265 h264
    // supoort 1280*720 3840*2160
    {
      data->width = source_width;
      data->height = source_height;
      data->data = this->m_data.dst_buf;
      data->size = map_info.size;
    }

    gst_buffer_unmap (buf, &map_info);
    // ** 
    gst_sample_unref (samp);

    this->m_data.frame++; 

  } catch (...) {
    data->isRun = STATUS_DISCONNECT;
    data->size = 0;
    return data;
  }

  return data;

}
