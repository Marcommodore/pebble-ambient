#include <pebble.h>
#include <planets.h>
#include <math.h>

static Window * main_window;
static Layer * main_layer;

static const uint8_t SKY_COLORS[] = {
  GColorBlackARGB8,
  GColorBlackARGB8,
  GColorOxfordBlueARGB8,
  GColorOxfordBlueARGB8,
  GColorOxfordBlueARGB8,
  GColorLibertyARGB8,
  GColorLibertyARGB8,
  GColorLibertyARGB8,
  GColorPictonBlueARGB8,
  GColorPictonBlueARGB8,
  GColorCelesteARGB8,
  GColorCelesteARGB8,
  GColorCelesteARGB8,
  GColorCelesteARGB8,
  GColorCelesteARGB8,
  GColorPictonBlueARGB8,
  GColorPictonBlueARGB8,
  GColorLibertyARGB8,
  GColorLibertyARGB8,
  GColorLibertyARGB8,
  GColorOxfordBlueARGB8,
  GColorOxfordBlueARGB8,
  GColorOxfordBlueARGB8,
  GColorBlackARGB8,
  GColorBlackARGB8
};
static GPoint TICK_MARKS[12][2];
static const GPoint CENTER = {72, 84};
static const int RADIUS = 70;
static const int MINUTE_RADIUS = 58;
static const int HOUR_RADIUS = 42;
static const int STROKE_WIDTH = 4;
static const int TICK_LENGTH = 4;
static const int PLANET_STROKE_WIDTH = 1;
static const int ORBIT_STROKE_WIDTH = 1;
static const int PLANET_OFFSET = 7;
static const int PLANET_SPACING = 8;
static const int EARTH_INDEX = 2;
static const int MOON_ORBIT_RADIUS = 5;

static GPoint planet_locations[N_PLANETS];
static GPoint moon_location;
static GColor sky_color;
static GPoint minute_location, hour_location;
static PropertyAnimation * hour_animation;
static PropertyAnimation * minute_animation;

static void calculate_tick_marks() {
  for (int i = 0; i < 12; ++i) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    TICK_MARKS[i][0].x = CENTER.x + cos_lookup(angle) * (RADIUS - STROKE_WIDTH / 2) / TRIG_MAX_RATIO;
    TICK_MARKS[i][0].y = CENTER.y + sin_lookup(angle) * (RADIUS - STROKE_WIDTH / 2) / TRIG_MAX_RATIO;
    TICK_MARKS[i][1].x = CENTER.x + cos_lookup(angle) * (RADIUS - TICK_LENGTH) / TRIG_MAX_RATIO;
    TICK_MARKS[i][1].y = CENTER.y + sin_lookup(angle) * (RADIUS - TICK_LENGTH) / TRIG_MAX_RATIO;
  }
}

static void gpoint_setter(void * subject, GPoint gpoint) {
  ((GPoint *)subject)->x = gpoint.x;
  ((GPoint *)subject)->y = gpoint.y;
  layer_mark_dirty(main_layer);
}

static void calculate() {
  time_t now = time(NULL); 
  struct tm * now_tm = localtime(&now);
  
  // background
  sky_color = (GColor)SKY_COLORS[now_tm->tm_hour];
  
  // calculate planet locations
  calculate_planet_time(now);
  for (int i = 0; i < N_PLANETS; ++i) {
    int radius = PLANET_OFFSET + PLANET_SPACING * i;
    int32_t angle = get_planet_angle(i);
    planet_locations[i].x = CENTER.x + cos_lookup(angle) * radius / TRIG_MAX_RATIO;
    planet_locations[i].y = CENTER.y + sin_lookup(angle) * radius / TRIG_MAX_RATIO;
  }
  int32_t angle = get_moon_angle();
  moon_location.x = planet_locations[EARTH_INDEX].x + cos_lookup(angle) * MOON_ORBIT_RADIUS / TRIG_MAX_RATIO;
  moon_location.y = planet_locations[EARTH_INDEX].y + sin_lookup(angle) * MOON_ORBIT_RADIUS / TRIG_MAX_RATIO;
  
  // watch hands
  int32_t minute_angle = TRIG_MAX_ANGLE * now_tm->tm_min / 60 - TRIG_MAX_ANGLE / 4;
  int32_t hour_angle = (TRIG_MAX_ANGLE * (((now_tm->tm_hour % 12) * 6) + (now_tm->tm_min / 10))) / (12 * 6) - TRIG_MAX_ANGLE / 4;
  minute_location.x = CENTER.x + cos_lookup(minute_angle) * MINUTE_RADIUS / TRIG_MAX_RATIO;
  minute_location.y = CENTER.y + sin_lookup(minute_angle) * MINUTE_RADIUS / TRIG_MAX_RATIO;
  hour_location.x = CENTER.x + cos_lookup(hour_angle) * HOUR_RADIUS / TRIG_MAX_RATIO;
  hour_location.y = CENTER.y + sin_lookup(hour_angle) * HOUR_RADIUS / TRIG_MAX_RATIO;
}

static void init_animations() {
  static const PropertyAnimationImplementation implementation = {
    .base = {
      .update = (AnimationUpdateImplementation)property_animation_update_gpoint,
    },
    .accessors = {
      .setter = { .gpoint = gpoint_setter, },
    },
  };
  
  minute_animation = property_animation_create(&implementation, &minute_location, NULL, NULL);
  hour_animation = property_animation_create(&implementation, &hour_location, NULL, NULL);
}

static void destroy_minute_animation(Animation * animation, bool finished, void * context) {
  property_animation_destroy(minute_animation);
}

static void destroy_hour_animation(Animation * animation, bool finished, void * context) {
  property_animation_destroy(hour_animation);
}

static void animate() {
  init_animations();
  
  property_animation_from(minute_animation, &minute_location, sizeof(minute_location), true);
  property_animation_from(hour_animation, &hour_location, sizeof(hour_location), true);
  
  calculate();
  
  property_animation_to(minute_animation, &minute_location, sizeof(minute_location), true);
  property_animation_to(hour_animation, &hour_location, sizeof(hour_location), true);
  
  Animation * minute_base_animation = property_animation_get_animation(minute_animation);
  Animation * hour_base_animation = property_animation_get_animation(hour_animation);
  
  animation_set_handlers(minute_base_animation, (AnimationHandlers) {
    .stopped = destroy_minute_animation
  }, NULL);

  animation_set_handlers(hour_base_animation, (AnimationHandlers) {
    .stopped = destroy_hour_animation
  }, NULL);
  
  animation_schedule(minute_base_animation);
  animation_schedule(hour_base_animation);
}

static void update_proc(Layer * layer, GContext * ctx) {
  // background
  graphics_context_set_fill_color(ctx, sky_color);
  graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
  
  // watch circle
  graphics_context_set_fill_color(ctx, (GColor)GColorWhiteARGB8);
  graphics_fill_circle(ctx, CENTER, RADIUS);
  graphics_context_set_stroke_color(ctx, (GColor)GColorBlackARGB8);
  graphics_context_set_stroke_width(ctx, STROKE_WIDTH);
  graphics_draw_circle(ctx, CENTER, RADIUS);
  
  // planet orbits
  graphics_context_set_stroke_width(ctx, ORBIT_STROKE_WIDTH);
  graphics_context_set_stroke_color(ctx, (GColor)GColorLightGrayARGB8);
  for (int i = 0; i < N_PLANETS; ++i) {
    int radius = PLANET_OFFSET + PLANET_SPACING * i;
    graphics_draw_circle(ctx, CENTER, radius);
  }
  graphics_draw_circle(ctx, planet_locations[EARTH_INDEX], MOON_ORBIT_RADIUS);
  
  // watch tick marks
  graphics_context_set_fill_color(ctx, (GColor)GColorBlackARGB8);
  graphics_context_set_stroke_width(ctx, STROKE_WIDTH);
  graphics_context_set_stroke_color(ctx, (GColor)GColorBlackARGB8);
  for (int i = 0; i < 12; ++i) {
    graphics_draw_line(ctx, TICK_MARKS[i][0], TICK_MARKS[i][1]);
    graphics_fill_circle(ctx, TICK_MARKS[i][1], STROKE_WIDTH / 2);
  }

  // watch hands
  graphics_context_set_fill_color(ctx, (GColor)GColorBlackARGB8);
  graphics_context_set_stroke_color(ctx, (GColor)GColorBlackARGB8);
  graphics_context_set_stroke_width(ctx, STROKE_WIDTH);
  graphics_draw_line(ctx, CENTER, hour_location);
  graphics_draw_line(ctx, CENTER, minute_location);
  graphics_fill_circle(ctx, CENTER, STROKE_WIDTH / 2);
  graphics_fill_circle(ctx, minute_location, STROKE_WIDTH / 2);
  graphics_fill_circle(ctx, hour_location, STROKE_WIDTH / 2);
  
  // planets
  graphics_context_set_stroke_width(ctx, PLANET_STROKE_WIDTH);
  graphics_context_set_stroke_color(ctx, (GColor)GColorBlackARGB8);
  for (int i = 0; i < N_PLANETS; ++i) {
    graphics_context_set_fill_color(ctx, (GColor)PLANET_COLORS[i]);
    graphics_fill_circle(ctx, planet_locations[i], PLANET_RADII[i]);
    graphics_draw_circle(ctx, planet_locations[i], PLANET_RADII[i]);
  }
  
  // moon (draw as square to get something like a circle with radius between 1 and 2 pixels, ie diameter 3)
  graphics_context_set_fill_color(ctx, (GColor)GColorBlackARGB8);
  graphics_fill_rect(ctx, GRect(moon_location.x - MOON_DIAMETER / 2, moon_location.y - MOON_DIAMETER / 2, MOON_DIAMETER, MOON_DIAMETER), MOON_DIAMETER / 2, GCornersAll);
  graphics_context_set_fill_color(ctx, (GColor)MOON_COLOR);
  graphics_fill_rect(ctx, GRect(moon_location.x - (MOON_DIAMETER / 2 - 1), moon_location.y - (MOON_DIAMETER / 2 - 1), MOON_DIAMETER / 2, MOON_DIAMETER / 2), MOON_DIAMETER / 2, GCornersAll);
}

static void main_window_load(Window * window) {
  Layer * window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  main_layer = layer_create(bounds);
  layer_set_update_proc(main_layer, update_proc);
  layer_add_child(window_layer, main_layer);
}

static void main_window_unload(Window * window) {
  layer_destroy(main_layer);
}

static void tick_handler(struct tm * tick_time, TimeUnits units_changed) {
  animate();
}

static void init() {
  main_window = window_create();

  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);
  
  calculate_tick_marks();
  calculate();
    
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}