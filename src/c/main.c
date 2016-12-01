#include <pebble.h>
#define KEY_SHOW_BATTERY 0
#define KEY_SHOW_BLUETOOTH 1
#define KEY_ANIMATE 2

static Window *s_main_window;
static TextLayer *s_date_layer;
static TextLayer *s_date_outline;
static TextLayer *s_bigHour_layer;
static TextLayer *s_bigMinute_layer;

static BitmapLayer *s_charge_layer;
static GBitmap *s_charge_bitmap;
static GBitmap *s_noCharge_bitmap;

static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_btc_bitmap;
static GBitmap *s_nobtc_bitmap;

char hourBuffer[] = "00";
char minuteBuffer[] = "00";
char dateBuffer[] = "XXX, XXX 00";

static int s_battery_level;
static Layer *s_battery_layer;

static BitmapLayer *s_bg_bmpLayer;
static GBitmap *s_bg_EH;


static GBitmap *s_abitmap;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence;


void battery_callback(BatteryChargeState state) {
  //Might use this later for a charging animation.
  if (state.is_charging) {
    //layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), false);
    bitmap_layer_set_bitmap(s_charge_layer,s_noCharge_bitmap);
  } else{
    //layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), true);
    bitmap_layer_set_bitmap(s_charge_layer,s_charge_bitmap);
  }
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  //layer_mark_dirty(s_battery_layer);
}
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 8.0F);

  // Draw the background
  //graphics_context_set_fill_color(ctx, GColorBlack);
  //graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(0, 0, 10, 8));
  graphics_draw_rect(ctx, GRect(10,2,2,4));
  
  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(1, 0, width, bounds.size.h), 0, GCornerNone);
}
static void bluetooth_callback(bool connected) {
  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
    bitmap_layer_set_bitmap(s_bluetooth_layer,s_nobtc_bitmap);
  }else{
    bitmap_layer_set_bitmap(s_bluetooth_layer,s_btc_bitmap);
  }
}
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  // Write the current hour
  strftime(dateBuffer,sizeof(dateBuffer), "%a, %b %d", tick_time);
  
  strftime(hourBuffer,sizeof(hourBuffer), "%H", tick_time);
  
  strftime(minuteBuffer,sizeof(minuteBuffer),"%M",tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_date_layer, dateBuffer);
  text_layer_set_text(s_date_outline, dateBuffer);
  
  text_layer_set_text(s_bigHour_layer, hourBuffer);
  text_layer_set_text(s_bigMinute_layer, minuteBuffer);
}

static void timer_handler(void *context){
  uint32_t next_delay;
  //Advance to next APNG frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence,s_abitmap,&next_delay)){
    bitmap_layer_set_bitmap(s_bitmap_layer,s_abitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    app_timer_register(next_delay, timer_handler, NULL);
  }else{
    gbitmap_sequence_restart(s_sequence);
    gbitmap_sequence_update_bitmap_next_frame(s_sequence,s_abitmap,&next_delay);
    bitmap_layer_set_bitmap(s_bitmap_layer,s_abitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    update_time();
    layer_mark_dirty(s_battery_layer);
  }
}
static void init_sequence() {
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_IMAGE_EHAPNG);
  s_abitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);
  gbitmap_sequence_set_play_count(s_sequence, 1);
  gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_abitmap, NULL);
  bitmap_layer_set_bitmap(s_bitmap_layer,s_abitmap);
  layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
}
static void run_sequence() {
  uint32_t next_delay;
  gbitmap_sequence_restart(s_sequence);
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence,s_abitmap,&next_delay)) {
    bitmap_layer_set_bitmap(s_bitmap_layer,s_abitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    app_timer_register(next_delay, timer_handler, NULL);
  }
}
static void time_timer_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {  // if minute changed - display animation (only if it's not already animating due to initial load)
    run_sequence();  // on minute change begin animation from 0 frame
  }else{
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    update_time();
  }
}
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  s_bg_bmpLayer = bitmap_layer_create(GRect(0,0,144,168));
  s_bg_EH = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EHBG);
  bitmap_layer_set_bitmap(s_bg_bmpLayer,s_bg_EH);
  layer_add_child(window_layer,bitmap_layer_get_layer(s_bg_bmpLayer));
  
  s_bitmap_layer = bitmap_layer_create(GRect(45,0,55,92));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  
  init_sequence();
  run_sequence();
  
  GFont s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BIGTIME_16));
  GFont s_hud_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HUD_16));
  //-------------------------------------------------------------------------------------------------------------------HUD
  s_date_layer = text_layer_create(GRect(0,-4,144,16));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_hud_font);
  
  s_date_outline = text_layer_create(GRect(1,-3,144,16));
  text_layer_set_background_color(s_date_outline, GColorClear);
  text_layer_set_text_color(s_date_outline, GColorBlack);
  text_layer_set_font(s_date_outline, s_hud_font);
  
  
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_date_outline, GTextAlignmentCenter);

  s_charge_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WATCH);
  s_noCharge_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE);
  
  s_charge_layer = bitmap_layer_create(GRect(116,4,8,8));
  bitmap_layer_set_bitmap(s_charge_layer,s_charge_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_charge_layer));
  bitmap_layer_set_compositing_mode(s_charge_layer, GCompOpSet);
  
  s_btc_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BTC);
  s_nobtc_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOBTC);
  
  s_bluetooth_layer = bitmap_layer_create(GRect(2,2,16,16));
  bitmap_layer_set_bitmap(s_bluetooth_layer,s_btc_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
  bitmap_layer_set_compositing_mode(s_bluetooth_layer, GCompOpSet);
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(124, 4, 11, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  //-------------------------------------------------------------------------------------------------------------------Big Hour/Minute
  s_bigHour_layer = text_layer_create(GRect(44,103,22,16));
  s_bigMinute_layer = text_layer_create(GRect(78,103,22,16));
  
  text_layer_set_background_color(s_bigHour_layer, GColorClear);
  text_layer_set_text_color(s_bigHour_layer, GColorWhite);
  text_layer_set_background_color(s_bigMinute_layer, GColorClear);
  text_layer_set_text_color(s_bigMinute_layer, GColorWhite);
  
  text_layer_set_font(s_bigHour_layer, s_time_font);
  text_layer_set_font(s_bigMinute_layer, s_time_font);
  
  text_layer_set_text_alignment(s_bigHour_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_bigMinute_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_bigHour_layer));
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_bigMinute_layer));
  
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_date_outline));
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_date_layer));
  
  layer_add_child(window_get_root_layer(window), s_battery_layer);
}
static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer);
}
/*
static void in_recv_handler(DictionaryIterator *iterator, void *context){
  //Get Tuple
  Tuple *t = dict_read_first(iterator);
  if(t)
  {
    switch(t->key)
    {
    case KEY_BG:
      //strcmp compares binary value of two strings, 0 means the two strings are equal
      if(strcmp(t->value->cstring, "summer") == 0){
        bitmap_layer_set_bitmap(s_bitmap_layer, s_bg_summer);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Summer selected");
        persist_write_string(KEY_BG, "summer");
      }
      else{
        APP_LOG(APP_LOG_LEVEL_DEBUG, "God damn it, what happened this time?");
      }
    }
  }
}
*/
static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  
  tick_timer_service_subscribe(MINUTE_UNIT, time_timer_tick);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Register for Bluetooth connection updates
  bluetooth_connection_service_subscribe(bluetooth_callback);
  // Show the correct state of the BT connection from the start
  bluetooth_callback(bluetooth_connection_service_peek());
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}