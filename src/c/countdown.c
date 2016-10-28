#include <pebble.h>
static Window *window;
static TextLayer *text_layer;
static AppTimer *timer;
static const uint16_t timer_interval_ms = 1000;
static int hours;
static int minutes;
static int seconds;
static char *single_vibes;
static char *double_vibes;
static char *long_vibes;
static int default_hours;
static int default_minutes;
static int default_seconds;
static char *default_long_vibes;
static char *default_single_vibes;
static char *default_double_vibes;
static int running;

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
  if (strstr(long_vibes, time_test)) {
    vibes_long_pulse();
  }
  if (strstr(single_vibes, time_test)) {
    vibes_short_pulse();
  }
  if (strstr(double_vibes, time_test)) {
    vibes_double_pulse();
  }
}

static void timer_callback(void *data);

static void advance_time() {
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
    if (timer) {
      app_timer_cancel(timer);
      timer = NULL;
    }
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

static void timer_callback(void *data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer: %d:%02d:%02d", hours, minutes, seconds);
  advance_time();
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
  Tuple *hour_tuple = dict_find(received, MESSAGE_KEY_hours);
  if(hour_tuple) {
    default_hours = hour_tuple->value->int8;
  }
  Tuple *minute_tuple = dict_find(received, MESSAGE_KEY_minutes);
  if(minute_tuple) {
    default_minutes = minute_tuple->value->int8;
  }
  Tuple *second_tuple = dict_find(received, MESSAGE_KEY_seconds);
  if(second_tuple) {
    default_seconds = second_tuple->value->int8;
  }
  Tuple *long_tuple = dict_find(received, MESSAGE_KEY_long);
  if(long_tuple) {
    strcpy(default_long_vibes, long_tuple->value->cstring);
  }
  Tuple *single_tuple = dict_find(received, MESSAGE_KEY_single);
  if(single_tuple) {
    strcpy(default_single_vibes, single_tuple->value->cstring);
  }
  Tuple *double_tuple = dict_find(received, MESSAGE_KEY_double);
  if(double_tuple) {
    strcpy(default_double_vibes, double_tuple->value->cstring);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "New config: %d:%02d:%02d; %s; %s; %s", hours, minutes, seconds,
    default_long_vibes, default_single_vibes, default_double_vibes);
  reset();
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message from phone dropped: %d", reason);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if ((running) || ((hours == 0) && (minutes == 0) && (seconds == 0))) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stop: %d:%02d:%02d", hours, minutes, seconds);
    if (timer) {
      app_timer_cancel(timer);
      timer = NULL;
    }
    if (!running) {
      reset();
    }
    running = 0;
    vibes_double_pulse();
    return;
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Start: %d:%02d:%02d", hours, minutes, seconds);
    running = 1;
  }
  vibes_short_pulse();
  advance_time();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync up: %d:%02d:%02d", hours, minutes, seconds);
  if (timer) {
    app_timer_cancel(timer);
    timer = NULL;
  }
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
  if (timer) {
    app_timer_cancel(timer);
    timer = NULL;
  }
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
  text_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(text_layer, GColorBlack);
  text_layer_set_text_color(text_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  reset();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 1024;
  const uint32_t outbound_size = 1024;
  app_message_open(inbound_size, outbound_size);

  default_hours = persist_exists(MESSAGE_KEY_hours) ? persist_read_int(MESSAGE_KEY_hours) : 0;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised hours to: %d", default_hours);
  default_minutes = persist_exists(MESSAGE_KEY_minutes) ? persist_read_int(MESSAGE_KEY_minutes) : 5;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised minutes to: %d", default_minutes);
  default_seconds = persist_exists(MESSAGE_KEY_seconds) ? persist_read_int(MESSAGE_KEY_seconds) : 0;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised seconds to: %d", default_seconds);
  default_long_vibes = "|4:00|3:00|2:00|1:00|0:00";
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised long vibes to: %s", default_long_vibes);
  if (persist_exists(MESSAGE_KEY_long)) {
    persist_read_string(MESSAGE_KEY_long, default_long_vibes, 256);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised persisted long vibes to: %s", default_long_vibes);
  }
  default_single_vibes = "|4:30|3:30|2:30|1:30";
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised single vibes to: %s", default_single_vibes);
  if (persist_exists(MESSAGE_KEY_single)) {
    persist_read_string(MESSAGE_KEY_single, default_single_vibes, 256);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised persisted single vibes to: %s", default_single_vibes);
  }
  default_double_vibes = "|0:50|0:40|0:30|0:25|0:20|0:15|0:10|0:09|0:08|0:07|0:06|0:05|0:04|0:03|0:02|0:01";
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised double vibes to: %s", default_double_vibes);
  if (persist_exists(MESSAGE_KEY_double)) {
    persist_read_string(MESSAGE_KEY_double, default_double_vibes, 256);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised persisted double vibes to: %s", default_double_vibes);
  }

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
  persist_write_int(MESSAGE_KEY_hours, default_hours);
  persist_write_int(MESSAGE_KEY_minutes, default_minutes);
  persist_write_int(MESSAGE_KEY_seconds, default_seconds);
  persist_write_string(MESSAGE_KEY_long, default_long_vibes);
  persist_write_string(MESSAGE_KEY_single, default_single_vibes);
  persist_write_string(MESSAGE_KEY_double, default_double_vibes);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
