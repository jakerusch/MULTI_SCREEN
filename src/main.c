#include <pebble.h>
#define KEY_CITY 0
#define KEY_TIME1 1
#define KEY_TEMP1 2
#define KEY_WEATHERDESC1 3
#define KEY_TIME2 4
#define KEY_TEMP2 5
#define KEY_WEATHERDESC2 6
#define KEY_TIME3 7
#define KEY_TEMP3 8
#define KEY_WEATHERDESC3 9
#define KEY_TEMP 10

#define ANTIALIASING true


static Window *s_main_window;
static Layer *s_canvas_layer, *s_weather_canvas_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
static GFont s_font, s_font_sans;
static TextLayer *s_battery_text_layer, *s_day_text_layer, *s_date_text_layer, 
  *s_weather_layer, *s_weather_layer_full, *s_conditions_layer,
  *s_weather_layer_city, 
  *s_weather_time_1, *s_weather_temp_1, *s_weather_weatherdesc_1, 
  *s_weather_time_2, *s_weather_temp_2, *s_weather_weatherdesc_2, 
  *s_weather_time_3, *s_weather_temp_3, *s_weather_weatherdesc_3;
static PropertyAnimation *s_prop_animation, *s_prop_animation_return, 
  *s_prop_animation_window, *s_prop_animation_return_window, 
  *s_prop_animation_return_hidden, *s_prop_animation_hidden;
static Animation *anim, *anim_return, *anim_window, 
  *anim_return_window, *anim_return_hidden, *anim_hidden;
static int delay_ms = 50;
static int duration_ms = 1500;

static const GPathInfo MINUTE_HAND_POINTS = {
//   6, (GPoint []) {
//     {-8, 0},
//     {-4, 8},
//     {4, 8},
//     {8, 0},
//     {3, -64},
//     {-3, -64}
//   }
  
  4, (GPoint []) {
    {4, 14},
    {-4, 14},
    {-3, -66},
    {3, -66}
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
//   6, (GPoint []) {
//     {-8, 0},
//     {-4, 8},
//     {4, 8},
//     {8, 0},
//     {3, -50},
//     {-3, -50}
//   }
  4, (GPoint []) {
    {5, 12},
    {-5, 12},
    {-3, -50}, 
    {3, -50}
  }
};

// watch hands
static GPath *s_minute_arrow, *s_hour_arrow;
static Layer *s_hands_layer;

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  // second hand length
  const int16_t second_hand_length = (bounds.size.w / 2) -3;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  
  graphics_context_set_antialiased(ctx, true);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);
  
  // second hand
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, second_hand, center);  

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect((bounds.size.w / 2) - 1, ((bounds.size.h) / 2) - 1, 3, 3), 0, GCornerNone);
}

//////////////////
// up animation //
//////////////////
// animation returns clock face to original location
static void up_animation() {
  GRect to_rect_return = GRect(0, 0, 144, 168);
  GRect to_rect_return_hidden = GRect(0, -84, 144, 168);
  s_prop_animation_return = property_animation_create_layer_frame(bitmap_layer_get_layer(s_bitmap_layer), NULL, &to_rect_return);  
  anim_return = property_animation_get_animation(s_prop_animation_return);
  
  s_prop_animation_return_window = property_animation_create_layer_frame(s_hands_layer, NULL, &to_rect_return); 
  anim_return_window = property_animation_get_animation(s_prop_animation_return_window);
  
  s_prop_animation_return_hidden = property_animation_create_layer_frame(text_layer_get_layer(s_weather_layer_full), NULL, &to_rect_return_hidden);
  anim_return_hidden = property_animation_get_animation(s_prop_animation_return_hidden);
    
  // Get the Animation
  animation_set_curve(anim_return, AnimationCurveEaseIn);
  animation_set_delay(anim_return, delay_ms);
  animation_set_duration(anim_return, duration_ms);   
  animation_schedule(anim_return);  
  
  // Get the Animation
  animation_set_curve(anim_return_window, AnimationCurveEaseIn);
  animation_set_delay(anim_return_window, delay_ms);
  animation_set_duration(anim_return_window, duration_ms);   
  animation_schedule(anim_return_window);    
  
  // Get the Animation
  animation_set_curve(anim_return_hidden, AnimationCurveEaseIn);
  animation_set_delay(anim_return_hidden, delay_ms);
  animation_set_duration(anim_return_hidden, duration_ms);   
  animation_schedule(anim_return_hidden);      
}

////////////////////
// down animation //
////////////////////
// animation drops clock face to bottom half of screen
static void down_animation() {
  GRect to_rect = GRect(0, 84, 144, 168);
  GRect to_rect_hidden = GRect(0, 0, 144, 168);
  s_prop_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(s_bitmap_layer), NULL, &to_rect);
  anim = property_animation_get_animation(s_prop_animation);
  
  s_prop_animation_window = property_animation_create_layer_frame(s_hands_layer, NULL, &to_rect);
  anim_window = property_animation_get_animation(s_prop_animation_window);
  
  s_prop_animation_hidden = property_animation_create_layer_frame(text_layer_get_layer(s_weather_layer_full), NULL, &to_rect_hidden);
  anim_hidden = property_animation_get_animation(s_prop_animation_hidden);
  
  // Configure the Animation's curve, delay, and duration
  animation_set_curve(anim, AnimationCurveEaseOut);
  animation_set_delay(anim, delay_ms);
  animation_set_duration(anim, duration_ms);  
  animation_schedule(anim); 
  
  // Configure the Animation's curve, delay, and duration
  animation_set_curve(anim_window, AnimationCurveEaseOut);
  animation_set_delay(anim_window, delay_ms);
  animation_set_duration(anim_window, duration_ms);  
  animation_schedule(anim_window);
  
  // Configure the Animation's curve, delay, and duration
  animation_set_curve(anim_hidden, AnimationCurveEaseOut);
  animation_set_delay(anim_return_hidden, delay_ms);
  animation_set_duration(anim_window, duration_ms);  
  animation_schedule(anim_hidden);  
}

static void hide_hands() {
  layer_set_hidden(s_hands_layer, true); 
}

static void show_hands() {
  layer_set_hidden(s_hands_layer, false);
}

//////////////////
// select click //
//////////////////
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  hide_hands();
  app_timer_register(2000, show_hands, NULL);
}

//////////////
// up click //
//////////////
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  up_animation();
}

////////////////
// down click //
////////////////
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  down_animation();
  // delay up_animation
  app_timer_register(10000, up_animation, NULL);
}

///////////////////
// assign clicks //
///////////////////
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

///////////////////
// update canvas //
///////////////////
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  ////////////////////////////////
  // Battery rectangle and line //
  ////////////////////////////////
  
  // battery rectangle
  GRect battery_rect = GRect(19, 77, 43, 13);
  graphics_draw_round_rect(ctx, battery_rect, 3);
  
  // dividing line in battery round rectange
  GPoint start_battery_line = GPoint(30, 77);
  GPoint end_battery_line = GPoint(30, 88);
  graphics_draw_line(ctx, start_battery_line, end_battery_line);

  // draw top of battery
  GRect battery_top_rect = GRect(24, 79, 3, 1);
  graphics_fill_rect(ctx, battery_top_rect, 0, 0);
  
  // draw the bottom of the battery
  GRect battery_bottom_rect = GRect(23, 80, 5, 8);
  graphics_fill_rect(ctx, battery_bottom_rect, 0, 0);
  
  // right side rect
  GRect temp_rect = GRect(82, 77, 43, 13);
  graphics_draw_round_rect(ctx, temp_rect, 3);
  
  // dividing line in date round rectange
  GPoint start_temp_line = GPoint(109, 77);
  GPoint end_temp_line = GPoint(109, 88);
  graphics_draw_line(ctx, start_temp_line, end_temp_line);
}

///////////////////////////
// update weather border //
///////////////////////////
static void weather_borders_proc(Layer *layer, GContext *ctx) {
  GRect w_1 = GRect(0, 14, 48, 68);
  graphics_draw_rect(ctx, w_1);
  
  GRect w_2 = GRect(48, 14, 48, 68);
  graphics_draw_rect(ctx, w_2);
  
  GRect w_3 = GRect(96, 14, 48, 68);
  graphics_draw_rect(ctx, w_3);
  
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
}

//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  ///////////////
  // main font //
  ///////////////
  s_font = fonts_load_custom_font(fonts_get_system_font(FONT_KEY_GOTHIC_14));
  
  //////////////////////////////////
  // custom font for weather text //
  //////////////////////////////////
  s_font_sans = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MOBILE_SANS_FONT_12));
  
  // register button clicks
  window_set_click_config_provider(window, click_config_provider);
  
  ////////////////////
  // hidden weather //
  ////////////////////
  s_weather_layer_full = text_layer_create(GRect(0, -84, 144, 68));
  text_layer_set_background_color(s_weather_layer_full, GColorClear);
  text_layer_set_text_color(s_weather_layer_full, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer_full, GTextAlignmentCenter);
  text_layer_set_font(s_weather_layer_full, s_font_sans);
  // add to window layer, not bitmap layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer_full));   
  
  /////////////////////////////////////////////
  // create canvas layer for weather borders //
  /////////////////////////////////////////////
  s_weather_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_weather_canvas_layer, weather_borders_proc);
  // add to bitmap so animation works for all
  layer_add_child(text_layer_get_layer(s_weather_layer_full), s_weather_canvas_layer);   
  
  // city
  s_weather_layer_city = text_layer_create(GRect(0, 0, 144, 14));
  text_layer_set_background_color(s_weather_layer_city, GColorBlack);
  text_layer_set_text_color(s_weather_layer_city, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer_city, GTextAlignmentCenter);
  text_layer_set_font(s_weather_layer_city, s_font_sans);
  text_layer_set_text(s_weather_layer_city, "Fetching weather...");
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_layer_city));
  
  // 1.1
  s_weather_time_1 = text_layer_create(GRect(1, 15, 46, 14));
  text_layer_set_background_color(s_weather_time_1, GColorClear);
  text_layer_set_text_color(s_weather_time_1, GColorBlack);
  text_layer_set_text_alignment(s_weather_time_1, GTextAlignmentCenter);
  text_layer_set_font(s_weather_time_1, s_font);
  text_layer_set_text(s_weather_time_1, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_time_1));      
  
  // 1.2
  s_weather_temp_1 = text_layer_create(GRect(1, 29, 46, 14));
  text_layer_set_background_color(s_weather_temp_1, GColorClear);
  text_layer_set_text_color(s_weather_temp_1, GColorBlack);
  text_layer_set_text_alignment(s_weather_temp_1, GTextAlignmentCenter);
  text_layer_set_font(s_weather_temp_1, s_font);
  text_layer_set_text(s_weather_temp_1, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_temp_1));      
   
  // 1.3
  s_weather_weatherdesc_1 = text_layer_create(GRect(1, 43, 46, 38));
  text_layer_set_background_color(s_weather_weatherdesc_1, GColorClear);
  text_layer_set_text_color(s_weather_weatherdesc_1, GColorBlack);
  text_layer_set_text_alignment(s_weather_weatherdesc_1, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_weather_weatherdesc_1, GTextOverflowModeWordWrap);
  text_layer_set_font(s_weather_weatherdesc_1, s_font_sans);
  text_layer_set_text(s_weather_weatherdesc_1, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_weatherdesc_1));   
  
  // 2.1
  s_weather_time_2 = text_layer_create(GRect(49, 15, 46, 14));
  text_layer_set_background_color(s_weather_time_2, GColorClear);
  text_layer_set_text_color(s_weather_time_2, GColorBlack);
  text_layer_set_text_alignment(s_weather_time_2, GTextAlignmentCenter);
  text_layer_set_font(s_weather_time_2, s_font);
  text_layer_set_text(s_weather_time_2, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_time_2));      
  
  // 2.2
  s_weather_temp_2 = text_layer_create(GRect(49, 29, 46, 14));
  text_layer_set_background_color(s_weather_temp_2, GColorClear);
  text_layer_set_text_color(s_weather_temp_2, GColorBlack);
  text_layer_set_text_alignment(s_weather_temp_2, GTextAlignmentCenter);
  text_layer_set_font(s_weather_temp_2, s_font);
  text_layer_set_text(s_weather_temp_2, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_temp_2));      
  
  // 2.3
  s_weather_weatherdesc_2 = text_layer_create(GRect(49, 43, 46, 38));
  text_layer_set_background_color(s_weather_weatherdesc_2, GColorClear);
  text_layer_set_text_color(s_weather_weatherdesc_2, GColorBlack);
  text_layer_set_text_alignment(s_weather_weatherdesc_2, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_weather_weatherdesc_2, GTextOverflowModeWordWrap);
  text_layer_set_font(s_weather_weatherdesc_2, s_font_sans);
  text_layer_set_text(s_weather_weatherdesc_2, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_weatherdesc_2));   
  
  // 3.1
  s_weather_time_3 = text_layer_create(GRect(97, 15, 46, 14));
  text_layer_set_background_color(s_weather_time_3, GColorClear);
  text_layer_set_text_color(s_weather_time_3, GColorBlack);
  text_layer_set_text_alignment(s_weather_time_3, GTextAlignmentCenter);
  text_layer_set_font(s_weather_time_3, s_font);
  text_layer_set_text(s_weather_time_3, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_time_3));      
  
  // 3.2
  s_weather_temp_3 = text_layer_create(GRect(97, 29, 46, 14));
  text_layer_set_background_color(s_weather_temp_3, GColorClear);
  text_layer_set_text_color(s_weather_temp_3, GColorBlack);
  text_layer_set_text_alignment(s_weather_temp_3, GTextAlignmentCenter);
  text_layer_set_font(s_weather_temp_3, s_font);
  text_layer_set_text(s_weather_temp_3, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_temp_3));      
  
  // 3.3
  s_weather_weatherdesc_3 = text_layer_create(GRect(97, 43, 46, 38));
  text_layer_set_background_color(s_weather_weatherdesc_3, GColorClear);
  text_layer_set_text_color(s_weather_weatherdesc_3, GColorBlack);
  text_layer_set_text_alignment(s_weather_weatherdesc_3, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_weather_weatherdesc_3, GTextOverflowModeWordWrap);
  text_layer_set_font(s_weather_weatherdesc_3, s_font_sans);
  text_layer_set_text(s_weather_weatherdesc_3, "-");
  // add to hidden weather layer
  layer_add_child(text_layer_get_layer(s_weather_layer_full), text_layer_get_layer(s_weather_weatherdesc_3));  
  
  ////////////////////////////
  // create clockface image //
  ////////////////////////////
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOCK_IMAGE);
  s_bitmap_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));
  
  ///////////////////////////////////
  // create canvas layer for lines //
  ///////////////////////////////////
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  // add to bitmap so animation works for all
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer), s_canvas_layer); 
  
  //////////////////
  // Battery Text //
  //////////////////
  s_battery_text_layer = text_layer_create(GRect(31, 74, 30, 14));
  text_layer_set_background_color(s_battery_text_layer, GColorClear);
  text_layer_set_text_color(s_battery_text_layer, GColorBlack);
  text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_battery_text_layer, s_font);
  text_layer_set_text(s_battery_text_layer, "-");
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer), text_layer_get_layer(s_battery_text_layer));    
  
  //////////////
  // Day Text //
  //////////////
  s_day_text_layer = text_layer_create(GRect(83, 74, 26, 14));
  text_layer_set_background_color(s_day_text_layer, GColorClear);
  text_layer_set_text_color(s_day_text_layer, GColorBlack);
  text_layer_set_text_alignment(s_day_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_day_text_layer, s_font);
  text_layer_set_text(s_day_text_layer, "-");
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer), text_layer_get_layer(s_day_text_layer));
  
  ///////////////
  // Date text //
  ///////////////
  s_date_text_layer = text_layer_create(GRect(108, 74, 16, 14));
  text_layer_set_background_color(s_date_text_layer, GColorClear);
  text_layer_set_text_color(s_date_text_layer, GColorBlack);
  text_layer_set_text_alignment(s_date_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_text_layer, s_font);
  text_layer_set_text(s_date_text_layer, "-");
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer), text_layer_get_layer(s_date_text_layer));
  
  /////////////////
  // Add Weather //
  /////////////////
  s_weather_layer = text_layer_create(GRect(116, 6, 24, 20));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_font(s_weather_layer, s_font);
  text_layer_set_text(s_weather_layer, "-");    
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer), text_layer_get_layer(s_weather_layer));
  
//   /////////////////////////////
//   // battery percentage text //
//   /////////////////////////////
//   s_battery_percentage_layer = text_layer_create(GRect(0, 120, 50, 50));
//   text_layer_set_background_color(s_battery_text_layer, GColorDarkGray);
//   text_layer_set_text_color(s_battery_text_layer, GColorBlack);
//   text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentCenter);
//   text_layer_set_font(s_battery_text_layer, s_font);
//   text_layer_set_text(s_battery_text_layer, "-");   
//   layer_add_child(bitmap_layer_get_layer(s_bitmap_layer), text_layer_get_layer(s_battery_text_layer));
  
  /////////////////
  // hands layer //
  /////////////////
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void update_time() {
  // get a tm strucutre
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write the current hours into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
  
  // write the current minutes
  static char s_buffer_small[8];
  strftime(s_buffer_small, sizeof(s_buffer_small), "%M", tick_time);
  
  // write date to buffer
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%d", tick_time);
  
  // write day to buffer
  static char day_buffer[16];
  strftime(day_buffer, sizeof(day_buffer), "%a", tick_time);
  
  // display this time on the text layer
  text_layer_set_text(s_date_text_layer, date_buffer);
  text_layer_set_text(s_day_text_layer, day_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "c%d%%", charge_state.charge_percent);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_text_layer, battery_text);  
}

////////////////////////
// unload main window //
////////////////////////
static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  layer_destroy(s_weather_canvas_layer);
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
  fonts_unload_custom_font(s_font_sans);
  text_layer_destroy(s_battery_text_layer);
  text_layer_destroy(s_day_text_layer);
  text_layer_destroy(s_date_text_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_weather_layer_full);
  text_layer_destroy(s_conditions_layer);
  text_layer_destroy(s_weather_layer_city);
  text_layer_destroy(s_weather_time_1);
  text_layer_destroy(s_weather_temp_1);
  text_layer_destroy(s_weather_weatherdesc_1);
  text_layer_destroy(s_weather_time_2);
  text_layer_destroy(s_weather_temp_2);
  text_layer_destroy(s_weather_weatherdesc_2);  
  text_layer_destroy(s_weather_time_3);
  text_layer_destroy(s_weather_temp_3);
  text_layer_destroy(s_weather_weatherdesc_3);
}

// for weather calls
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char city_buf[32];
  static char temp_buf[8];
  
  static char time_buf_1[8];
  static char temp_buf_1[8];
  static char wea_desc_buf_1[32];
  
  static char time_buf_2[8];
  static char temp_buf_2[8];
  static char wea_desc_buf_2[32];  
  
  static char time_buf_3[8];
  static char temp_buf_3[8];
  static char wea_desc_buf_3[32];    
  
  static char city_layer_buf[32];
  static char temp_layer_buf[32];
  
  static char time_layer_buf_1[32];
  static char temp_layer_buf_1[32];
  static char wea_desc_layer_buf_1[32];

  static char time_layer_buf_2[32];
  static char temp_layer_buf_2[32];
  static char wea_desc_layer_buf_2[32];

  static char time_layer_buf_3[32];
  static char temp_layer_buf_3[32];
  static char wea_desc_layer_buf_3[32];  
  
  // Read tuples for data
  Tuple *city_tuple = dict_find(iterator, KEY_CITY);
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMP);
  
  Tuple *time_tuple_1 = dict_find(iterator, KEY_TIME1);
  Tuple *temp_tuple_1 = dict_find(iterator, KEY_TEMP1);
  Tuple *wea_desc_tuple_1 = dict_find(iterator, KEY_WEATHERDESC1);

  Tuple *time_tuple_2 = dict_find(iterator, KEY_TIME2);
  Tuple *temp_tuple_2 = dict_find(iterator, KEY_TEMP2);
  Tuple *wea_desc_tuple_2 = dict_find(iterator, KEY_WEATHERDESC2);  

  Tuple *time_tuple_3 = dict_find(iterator, KEY_TIME3);
  Tuple *temp_tuple_3 = dict_find(iterator, KEY_TEMP3);
  Tuple *wea_desc_tuple_3 = dict_find(iterator, KEY_WEATHERDESC3);    
  
  // If all data is available, use it
  if(city_tuple && temp_tuple && 
      time_tuple_1 && temp_tuple_1 && wea_desc_tuple_1 && 
      time_tuple_2 && temp_tuple_2 && wea_desc_tuple_2 &&
      time_tuple_3 && temp_tuple_3 && wea_desc_tuple_3) {
    
    snprintf(city_buf, sizeof(city_buf), "%s", city_tuple->value->cstring);
    snprintf(temp_buf, sizeof(temp_buf), "%d°", (int)temp_tuple->value->int32);
    
    snprintf(time_buf_1, sizeof(time_buf_1), "%d", (int)time_tuple_1->value->int32);
    snprintf(temp_buf_1, sizeof(temp_buf_1), "%d°", (int)temp_tuple_1->value->int32);
    snprintf(wea_desc_buf_1, sizeof(wea_desc_buf_1), "%s", wea_desc_tuple_1->value->cstring);
    
    snprintf(time_buf_2, sizeof(time_buf_2), "%d", (int)time_tuple_2->value->int32);
    snprintf(temp_buf_2, sizeof(temp_buf_2), "%d°", (int)temp_tuple_2->value->int32);
    snprintf(wea_desc_buf_2, sizeof(wea_desc_buf_2), "%s", wea_desc_tuple_2->value->cstring);
    
    snprintf(time_buf_3, sizeof(time_buf_3), "%d", (int)time_tuple_3->value->int32);
    snprintf(temp_buf_3, sizeof(temp_buf_3), "%d°", (int)temp_tuple_3->value->int32);
    snprintf(wea_desc_buf_3, sizeof(wea_desc_buf_3), "%s", wea_desc_tuple_3->value->cstring);    
        
    //////////
    // city //
    //////////
    snprintf(city_layer_buf, sizeof(city_layer_buf), "%s Forecast", city_buf);
    text_layer_set_text(s_weather_layer_city, city_layer_buf);
    
    //////////
    // temp //
    //////////
    snprintf(temp_layer_buf, sizeof(temp_layer_buf), "%s", temp_buf);
    text_layer_set_text(s_weather_layer, temp_layer_buf);
    
    ////////////
    // time 1 //
    ////////////
    snprintf(time_layer_buf_1, sizeof(time_layer_buf_1), "%s:00", time_buf_1);
    text_layer_set_text(s_weather_time_1, time_layer_buf_1);
    
    ////////////
    // temp 1 //
    ////////////
    snprintf(temp_layer_buf_1, sizeof(temp_layer_buf_1), "%s", temp_buf_1);
    text_layer_set_text(s_weather_temp_1, temp_layer_buf_1);
    
    ////////////////////
    // weather desc 1 //
    ////////////////////
    snprintf(wea_desc_layer_buf_1, sizeof(wea_desc_layer_buf_1), "%s", wea_desc_buf_1);
    text_layer_set_text(s_weather_weatherdesc_1, wea_desc_layer_buf_1);    
    
    ////////////
    // time 2 //
    ////////////
    snprintf(time_layer_buf_2, sizeof(time_layer_buf_2), "%s:00", time_buf_2);
    text_layer_set_text(s_weather_time_2, time_layer_buf_2);
    
    ////////////
    // temp 2 //
    ////////////
    snprintf(temp_layer_buf_2, sizeof(temp_layer_buf_2), "%s", temp_buf_2);
    text_layer_set_text(s_weather_temp_2, temp_layer_buf_2);
    
    ////////////////////
    // weather desc 2 //
    ////////////////////
    snprintf(wea_desc_layer_buf_2, sizeof(wea_desc_layer_buf_2), "%s", wea_desc_buf_2);
    text_layer_set_text(s_weather_weatherdesc_2, wea_desc_layer_buf_2);          

    ////////////
    // time 3 //
    ////////////
    snprintf(time_layer_buf_3, sizeof(time_layer_buf_3), "%s:00", time_buf_3);
    text_layer_set_text(s_weather_time_3, time_layer_buf_3);
    
    ////////////
    // temp 3 //
    ////////////
    snprintf(temp_layer_buf_3, sizeof(temp_layer_buf_3), "%s", temp_buf_3);
    text_layer_set_text(s_weather_temp_3, temp_layer_buf_3);
    
    ////////////////////
    // weather desc 3 //
    ////////////////////
    snprintf(wea_desc_layer_buf_3, sizeof(wea_desc_layer_buf_3), "%s", wea_desc_buf_3);
    text_layer_set_text(s_weather_weatherdesc_3, wea_desc_layer_buf_3);          
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


////////////////////
// initialize app //
////////////////////
static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // show window on the watch with animated=true
  window_stack_push(s_main_window, true);
  
  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);  
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // register with Battery State Service
  battery_state_service_subscribe(handle_battery);
  handle_battery(battery_state_service_peek());
  
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  
  // Open AppMessage for weather callbacks
  const int inbox_size = 256;
  const int outbox_size = 256;
  app_message_open(inbox_size, outbox_size);  
}

///////////////////////
// de-initialize app //
///////////////////////
static void deinit() {
  window_destroy(s_main_window);
}

/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
}