#include <pebble.h>
/*
#define ONE_SECOND 1000
*/
static Window *window;
static TextLayer *text_layer;
static AppTimer *timer;
static const uint16_t timer_interval_ms = 1000;
int hours;
int minutes;
int seconds;
char *single_vibes;
char *double_vibes;
char *long_vibes;
int default_hours;
int default_minutes;
int default_seconds;
char *default_single_vibes;
char *default_double_vibes;
char *default_long_vibes;
char *time_key;
char *single_key;
char *double_key;
char *long_key;
int running;

static void show_time(void) {
  static char body_text[9];
  static char time_test[9];
  if (hours > 0) {
    snprintf(body_text, sizeof(body_text), "%d\n%02d\n%02d", hours, minutes, seconds);
    snprintf(time_test, sizeof(body_text), "%d:%02d:%02d", hours, minutes, seconds);
  }
  else {
    snprintf(body_text, sizeof(body_text), "\n%d\n%02d", minutes, seconds);
    snprintf(time_test, sizeof(body_text), "%d:%02d", minutes, seconds);
  }
  text_layer_set_text(text_layer, body_text);
  if (strstr(single_vibes, time_test)) {
    vibes_short_pulse();
  }
  if (strstr(long_vibes, time_test)) {
    vibes_long_pulse();
  }
  if (strstr(double_vibes, time_test)) {
    vibes_double_pulse();
  }
}

static void timer_callback(void *data) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer: %d:%02d:%02d", hours, minutes, seconds);

  seconds--;
  if (seconds < 0) {
    minutes--;
    seconds = 59;
  }
  if (minutes < 0) {
    hours--;
    minutes = 59;
  }
  if (hours < 0) {
    app_timer_cancel(timer);
    running = 0;
    hours = 0;
    minutes = 0;
    seconds = 0;
  }
  else {
    timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  }
  show_time();
}

static void reset(void) {
  running = 0;
  hours = default_hours;
  minutes = default_minutes;
  seconds = default_seconds;
  long_vibes = default_long_vibes;
  single_vibes = default_single_vibes;
  double_vibes = default_double_vibes;
  show_time();
}

void in_received_handler(DictionaryIterator *received, void *context) {

  Tuple *msg_type = dict_read_first(received);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message from phone: %s", msg_type->value->cstring);
  if (strcmp(msg_type->value->cstring, time_key) == 0) {
    Tuple *hrs = dict_read_next(received);
    default_hours = hrs->value->int8;
    /* WTF: 2 becomes 50! */
    if (default_hours >= 50) {
     default_hours = default_hours - 48;
    }
    Tuple *mins = dict_read_next(received);
    default_minutes = mins->value->int8;
    Tuple *secs = dict_read_next(received);
    default_seconds = secs->value->int8;
	reset();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "New config: %d:%02d:%02d", hours, minutes, seconds);
  }
  else {
    Tuple *val = dict_read_next(received);
    if (strcmp(msg_type->value->cstring, long_key) == 0) {
      long_vibes = val->value->cstring;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set long vibes to: %s", long_vibes);
    }
    if (strcmp(msg_type->value->cstring, single_key) == 0) {
      single_vibes = val->value->cstring;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set single vibes to: %s", single_vibes);
    }
    if (strcmp(msg_type->value->cstring, double_key) == 0) {
      double_vibes = val->value->cstring;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set double vibes to: %s", double_vibes);
    }
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message from phone dropped: %d", reason);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if ((running) || ((hours == 0) && (minutes == 0) && (seconds == 0))) {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Stop: %d:%02d:%02d", hours, minutes, seconds);
   app_timer_cancel(timer);
   if (!running) {
     reset();
   }
   running = 0;
   return;
  }
  else {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Start: %d:%02d:%02d", hours, minutes, seconds);
   running = 1;
  }
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  show_time();
  vibes_short_pulse();
  seconds--;
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync up: %d:%02d:%02d", hours, minutes, seconds);
  app_timer_cancel(timer);
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  running = 1;
  minutes++;
  if (minutes > 59) {
    hours++;
    minutes = 0;
  }
  seconds = 0;
  show_time();
  vibes_short_pulse();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync down: %d:%02d:%02d", hours, minutes, seconds);
  app_timer_cancel(timer);
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  running = 1;
  seconds = 0;
  show_time();
  vibes_short_pulse();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  text_layer = text_layer_create(bounds);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  reset();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 128;
  const uint32_t outbound_size = 128;
  app_message_open(inbound_size, outbound_size);

  default_hours = 0;
  default_minutes = 5;
  default_seconds = 0;
  default_long_vibes = "|4:00|3:00|2:00|1:00|0:00";
  default_single_vibes = "|4:30|3:30|2:30|1:30";
  default_double_vibes = "|0:50|0:40|0:30|0:25|0:20|0:15|0:10|0:09|0:08|0:07|0:06|0:05|0:04|0:03|0:02|0:01";
  time_key = "time";
  single_key = "single";
  double_key = "double";
  long_key = "long";

  APP_LOG(APP_LOG_LEVEL_DEBUG, "OK: %d", APP_MSG_OK);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SEND_TIMEOUT: %d", APP_MSG_SEND_TIMEOUT);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SEND_REJECTED: %d", APP_MSG_SEND_REJECTED);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "NOT_CONNECTED: %d", APP_MSG_NOT_CONNECTED);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "APP_NOT_RUNNING: %d", APP_MSG_APP_NOT_RUNNING);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "INVALID_ARGS: %d", APP_MSG_INVALID_ARGS);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "BUSY: %d", APP_MSG_BUSY);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "BUFFER_OVERFLOW: %d", APP_MSG_BUFFER_OVERFLOW);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "ALREADY_RELEASED: %d", APP_MSG_ALREADY_RELEASED);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "CALLBACK_ALREADY_REGISTERED: %d", APP_MSG_CALLBACK_ALREADY_REGISTERED);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "CALLBACK_NOT_REGISTERED: %d", APP_MSG_CALLBACK_NOT_REGISTERED);
  
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
