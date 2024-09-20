// Includes necesarios para el proyecto
#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"
#include "Lpf2Hub.h"

// Definición de constantes
#define LVGL_PORT_ROTATION_DEGREE (90)

// Definición de variables globales
int current_ligth = 0;
int velocidad = 0;
std::string COLORS_NAME[11] = {"OFF", "PINK", "PURPLE", "BLUE", "LIGHTBLUE", "CYAN", "GREEN", "YELLOW", "ORANGE", "RED", "WHITE"};

// Punteros a objetos gráficos
lv_obj_t *slider;
lv_obj_t *value_label;
lv_obj_t *kb;
lv_obj_t *btn_connect;
lv_obj_t *label_led;
lv_obj_t *label_info;
lv_meter_indicator_t *indic_line;
lv_obj_t *meter3;

// Estilos de los botones
lv_style_t style_btn_red;
lv_style_t style_btn_yellow;
lv_style_t style_btn_green;
lv_style_t style_btn_orange;
lv_style_t style_btn_grey;

// Controlador del hub del tren
Lpf2Hub myHub;
byte motorPort = (byte)DuploTrainHubPort::MOTOR;

// Declaración de funciones
void connect_train();
void play_horn();
void light_state();
void fill_water();
void stop_train();
void update_battery(lv_obj_t *label_battery);
void slider_event_cb(lv_event_t *e);
void stop_slider_event_cb(lv_event_t *e);
void create_train_ui(lv_obj_t *tab);
void slider_control(lv_obj_t *slider);
void train_tick();
void colorSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData);
void speedometerSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData);

// Implementación de funciones

// Simulación de conexión al tren
void connect_train() {

    if (myHub.isConnecting())
    {
      lv_label_set_text(label_info, "STATE: Conectando al tren...");
    }
    else if (myHub.isConnected()) 
    {
      lv_label_set_text(label_info, "STATE: Tren conectado.");
    } 
    else 
    {
        lv_label_set_text(label_info, "STATE: Tren desconectado.");
        myHub.init();
    }
}

// Simulación del claxon
void play_horn() {
    if (myHub.isConnected()) {
      myHub.playSound((byte)DuploTrainBaseSound::HORN);
    } else {
        lv_label_set_text(label_info, "STATE: Tren desconectado.");
    }
}

// Simulación del estado de las luces
void light_state() {
    if (myHub.isConnected())
    {
      current_ligth++;
      if(current_ligth > WHITE)
      {
        current_ligth = BLACK;
      }
      myHub.setLedColor((Color)current_ligth);

      char buffer[16];
      sprintf(buffer, "LED: %s", COLORS_NAME[current_ligth].c_str());
      lv_label_set_text(label_led, buffer);
    }
    else
    {
        lv_label_set_text(label_info, "STATE: Tren desconectado.");
    }
}

// Simulación de llenar agua
void fill_water() {
    if (myHub.isConnected()) {
        myHub.playSound((byte)DuploTrainBaseSound::WATER_REFILL);
    } else {
        lv_label_set_text(label_info, "STATE: Tren desconectado.");
    }
}

// Detener el tren
void stop_train() {
    if (myHub.isConnected()) {
      stop_slider_event_cb(nullptr);
    } else {
        lv_label_set_text(label_info, "STATE: Tren desconectado.");
    }
}

// Actualización del nivel de batería (simulación)
//TODO
void update_battery(lv_obj_t *label_battery) {
    char buffer[16];
    sprintf(buffer, "Bateria: %d%%", 100);
    lv_label_set_text(label_battery, buffer);
}

// Control del slider (palanca) y actualización del velocímetro
void slider_event_cb(lv_event_t *e) 
{ 
  if(e != nullptr)
  {
    lv_obj_t *slider = lv_event_get_target(e);
  }
  
  int slider_value = lv_slider_get_value(slider);

  int speed = 0;
  if ( slider_value > 40) speed = 64;       //Fast foreward
  else if (slider_value > 30) speed = 32;   //normal forweard
  else if (slider_value > 20) speed = 16;   //slow forefard (might not work on low battery) 
  else if (slider_value > -10) speed = 0;   //stop
  else if (slider_value > -30) speed = -32; //slow backward
  else speed = -64;                         //fast foreward

  if (myHub.isConnected())
  {       
    if(speed != velocidad)
    { 
      if(velocidad == 0 && speed > 0)
      {
          myHub.playSound((byte)DuploTrainBaseSound::STATION_DEPARTURE);
          delay(100);
      }
      velocidad = speed;
      myHub.setBasicMotorSpeed(motorPort, speed);
    }
  }

  //marcador
  lv_meter_set_indicator_value(meter3, indic_line, slider_value>0 ? slider_value : slider_value*-1);
}

// Función de callback para el botón "Detener" que ajusta el slider a 0
void stop_slider_event_cb(lv_event_t *e)
{
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);
  slider_event_cb(nullptr); 
}

void slider_control(lv_obj_t * slider)
{
  /*Create a transition*/
    static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, (lv_style_prop_t)0};
    static lv_style_transition_dsc_t transition_dsc;
    lv_style_transition_dsc_init(&transition_dsc, props, lv_anim_path_linear, 300, 0, NULL);

    static lv_style_t style_main;
    static lv_style_t style_indicator;
    static lv_style_t style_knob;
    static lv_style_t style_pressed_color;
    lv_style_init(&style_main);
    lv_style_set_bg_opa(&style_main, LV_OPA_COVER);
    lv_style_set_bg_color(&style_main, lv_color_hex3(0xddd));
    lv_style_set_radius(&style_main, LV_RADIUS_CIRCLE);
    lv_style_set_pad_ver(&style_main, -2);

    lv_style_init(&style_indicator);
    lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indicator,  lv_color_hex3(0xddd));
    lv_style_set_radius(&style_indicator, LV_RADIUS_CIRCLE);
    lv_style_set_transition(&style_indicator, &transition_dsc);

    lv_style_init(&style_knob);
    lv_style_set_bg_opa(&style_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&style_knob, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_border_color(&style_knob, lv_palette_darken(LV_PALETTE_GREY, 3));
    lv_style_set_border_width(&style_knob, 2);
    lv_style_set_radius(&style_knob, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_knob, 6);
    lv_style_set_transition(&style_knob, &transition_dsc);

    lv_style_init(&style_pressed_color);
    lv_style_set_bg_color(&style_pressed_color, lv_palette_darken(LV_PALETTE_GREY, 2));

    lv_obj_remove_style_all(slider);

    lv_obj_add_style(slider, &style_main, LV_PART_MAIN);
    lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);
}

void create_train_ui(lv_obj_t *tab)
{
    lv_style_init(&style_btn_red);
    lv_style_set_bg_color(&style_btn_red, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_grad_color(&style_btn_red, lv_palette_lighten(LV_PALETTE_RED, 3));

    lv_style_init(&style_btn_yellow);
    lv_style_set_bg_color(&style_btn_yellow, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_color(&style_btn_yellow, lv_palette_main(LV_PALETTE_BROWN));
    lv_style_set_bg_grad_color(&style_btn_yellow, lv_palette_lighten(LV_PALETTE_YELLOW, 3));

    lv_style_init(&style_btn_green);
    lv_style_set_bg_color(&style_btn_green, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_grad_color(&style_btn_green, lv_palette_lighten(LV_PALETTE_GREEN, 3));

    lv_style_init(&style_btn_orange);
    lv_style_set_bg_color(&style_btn_orange, lv_palette_main(LV_PALETTE_ORANGE));
    lv_style_set_text_color(&style_btn_orange, lv_palette_main(LV_PALETTE_BROWN));
    lv_style_set_bg_grad_color(&style_btn_orange, lv_palette_lighten(LV_PALETTE_ORANGE, 3));

    lv_style_init(&style_btn_grey);
    lv_style_set_bg_color(&style_btn_grey, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_bg_grad_color(&style_btn_grey, lv_palette_lighten(LV_PALETTE_GREY, 3));

    lv_obj_t *btn_reconnect = lv_btn_create(tab);
    lv_obj_add_style(btn_reconnect, &style_btn_orange, 0);
    lv_obj_set_size(btn_reconnect, 100, 40);
    lv_obj_align(btn_reconnect, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t *label_reconnect = lv_label_create(btn_reconnect);
    lv_label_set_text(label_reconnect, "ReConect");
    lv_obj_add_event_cb(btn_reconnect, [](lv_event_t *e) { connect_train(); }, LV_EVENT_CLICKED, NULL);

    label_info = lv_label_create(tab);
    lv_label_set_text(label_info, "STATE: ");
    lv_obj_align(label_info, LV_ALIGN_TOP_LEFT, 100, 20);

    label_led = lv_label_create(tab);
    char buffer[16];
    sprintf(buffer, "LED: %s", COLORS_NAME[current_ligth].c_str());
    lv_label_set_text(label_led, buffer);
    lv_obj_align(label_led, LV_ALIGN_TOP_LEFT, 100, 50);

    lv_obj_t *label_battery = lv_label_create(tab);
    update_battery(label_battery); 
    lv_obj_align(label_battery, LV_ALIGN_TOP_LEFT, 100, 80);

    // Slider para controlar la velocidad del tren
    slider = lv_slider_create(tab);
    slider_control(slider);
    lv_slider_set_range(slider, -50, 50); 
    lv_obj_set_size(slider, 40, 200);
    lv_obj_align(slider, LV_ALIGN_LEFT_MID, 10, 10);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *btn_claxon = lv_btn_create(tab);
    lv_obj_add_style(btn_claxon, &style_btn_yellow, 0);
    lv_obj_set_size(btn_claxon, 80, 50);
    lv_obj_align(btn_claxon, LV_ALIGN_CENTER, 60, 10);
    lv_obj_t *label_claxon = lv_label_create(btn_claxon);
    lv_label_set_text(label_claxon, "SIRENA");
    lv_obj_add_event_cb(btn_claxon, [](lv_event_t *e) { play_horn(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_luces = lv_btn_create(tab);
    lv_obj_add_style(btn_luces, &style_btn_green, 0);
    lv_obj_set_size(btn_luces, 80, 50);
    lv_obj_align(btn_luces, LV_ALIGN_CENTER, 170, 10);
    lv_obj_t *label_luces = lv_label_create(btn_luces);
    lv_label_set_text(label_luces, "LUZ");
    lv_obj_add_event_cb(btn_luces, [](lv_event_t *e) { light_state(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_agua = lv_btn_create(tab);
    lv_obj_set_size(btn_agua, 80, 50);
    lv_obj_align(btn_agua, LV_ALIGN_CENTER, 60, 90);
    lv_obj_t *label_agua = lv_label_create(btn_agua);
    lv_label_set_text(label_agua, "AGUA");
    lv_obj_add_event_cb(btn_agua, [](lv_event_t *e) { fill_water(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_detener = lv_btn_create(tab);
    lv_obj_add_style(btn_detener, &style_btn_red, 0);
    lv_obj_set_size(btn_detener, 80, 50);
    lv_obj_align(btn_detener, LV_ALIGN_CENTER, 170, 90);
    lv_obj_t *label_detener = lv_label_create(btn_detener);
    lv_label_set_text(label_detener, "STOP");
    lv_obj_add_event_cb(btn_detener, stop_slider_event_cb, LV_EVENT_CLICKED, NULL);

    // Creación del medidor para el indicador de red
    meter3 = lv_meter_create(tab);
    lv_obj_set_size(meter3, 120, 120);
    lv_obj_align(meter3, LV_ALIGN_BOTTOM_LEFT, 80, -5);

    lv_meter_scale_t *scale = lv_meter_add_scale(meter3);
    lv_meter_set_scale_range(meter3, scale, 0, 50, 270, 90);
    lv_meter_set_scale_ticks(meter3, scale, 6, 2, 10, lv_color_black());
    lv_meter_set_scale_major_ticks(meter3, scale, 1, 4, 15, lv_color_black(), 10);

    // Indicadores de color en rangos
    lv_meter_indicator_t *indic;

    // Rango verde (50-75%)
    indic = lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(meter3, indic, 30);
    lv_meter_set_indicator_end_value(meter3, indic, 50);

    // Rango amarillo (25-50%)
    indic = lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_YELLOW), 0);
    lv_meter_set_indicator_start_value(meter3, indic, 10);
    lv_meter_set_indicator_end_value(meter3, indic, 30);

    // Rango rojo (0-25%)
    indic = lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(meter3, indic, 0);
    lv_meter_set_indicator_end_value(meter3, indic, 10);

    // Indicador para mostrar el valor actual (por ejemplo 30)
    indic_line = lv_meter_add_needle_line(meter3, scale, 4, lv_color_black(), -10);
    lv_meter_set_indicator_value(meter3, indic_line, 0);  // Ejemplo: valor actual en 30%
}

void setup() {
    Serial.begin(115200);

    // Inicializar la pantalla
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 180
        .rotate = LV_DISP_ROT_180,
#elif LVGL_PORT_ROTATION_DEGREE == 0
        .rotate = LV_DISP_ROT_NONE,
#endif
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    create_train_ui(lv_scr_act());
}

void train_tick()
{
  if (myHub.isConnecting())
  {
    myHub.connectHub();
    if (myHub.isConnected())
    {
      lv_label_set_text(label_info, "STATE: Connected");
      delay(200);
      // connect color sensor and activate it for updates
      myHub.activatePortDevice((byte)DuploTrainHubPort::SPEEDOMETER, speedometerSensorCallback);
      delay(200);
      // connect speed sensor and activate it for updates
      myHub.activatePortDevice((byte)DuploTrainHubPort::COLOR, colorSensorCallback);
      delay(200);
      myHub.setLedColor(GREEN);
    }
    else
    {
      lv_label_set_text(label_info, "STATE: Failed to connect");
    }
  }
}

void loop() {
    train_tick();
    unsigned long currentMillis = millis();  // Obtén el tiempo actual
    lv_task_handler();
    delay(5);
}

void colorSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData)
{
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  if (deviceType == DeviceType::DUPLO_TRAIN_BASE_COLOR_SENSOR)
  {
    int color = myHub->parseColor(pData);
    myHub->setLedColor((Color)color);

    if (color == (byte)RED)
    {
      myHub->playSound((byte)DuploTrainBaseSound::STATION_DEPARTURE);
    }
    else if (color == (byte)BLUE)
    {
      myHub->playSound((byte)DuploTrainBaseSound::WATER_REFILL);
    }
    else if (color == (byte)YELLOW)
    {
      myHub->playSound((byte)DuploTrainBaseSound::HORN);
    }
  }
}

void speedometerSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData)
{
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  if (deviceType == DeviceType::DUPLO_TRAIN_BASE_SPEEDOMETER)
  {
    int speed = myHub->parseSpeedometer(pData);
    if (speed > 10)
    {
      myHub->setBasicMotorSpeed(motorPort, 50);
    }
    else if (speed < -10)
    {
      myHub->setBasicMotorSpeed(motorPort, -50);
    }
    else
    {
      myHub->setBasicMotorSpeed(motorPort, 0);
      myHub->stopBasicMotor(motorPort);
    }
  }
}
