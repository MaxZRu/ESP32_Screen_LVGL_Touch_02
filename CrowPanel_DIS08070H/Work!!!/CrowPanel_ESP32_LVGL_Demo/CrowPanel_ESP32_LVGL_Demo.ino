/**************************CrowPanel ESP32 HMI Display Example Code************************
Version     :	1.1
Suitable for:	CrowPanel ESP32 HMI Display
Product link:	https://www.elecrow.com/esp32-display-series-hmi-touch-screen.html
Code	  link:	https://github.com/Elecrow-RD/CrowPanel-ESP32-Display-Course-File
Lesson	link:	https://www.youtube.com/watch?v=WHfPH-Kr9XU
Description	:	The code is currently available based on the course on YouTube, 
				        if you have any questions, please refer to the course video: Introduction 
				        to ask questions or feedback.
**************************************************************/


#include <Wire.h>
#include <SPI.h>
#include <PCA9557.h>
#include <WiFi.h>
#include <HTTPClient.h>
//#include <ESPping.h> //Библиотек ESPping 1.0.5 dvarrel

/**************************LVGL and UI************************
if you want to use the LVGL demo. you need to include <demos/lv_demos.h> and <examples/lv_examples.h>. 
if not, please do not include it. It will waste your Flash space.
**************************************************************/
#include <lvgl.h>
#include "ui.h"
// #include <demos/lv_demos.h>
// #include <examples/lv_examples.h>
/**************************LVGL and UI END************************/

/*******************************************************************************
   Config the display panel and touch panel in gfx_conf.h
 ******************************************************************************/
#include "gfx_conf.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf1[screenWidth * screenHeight / 10];
static lv_color_t disp_draw_buf2[screenWidth * screenHeight / 10];
static lv_disp_drv_t disp_drv;

PCA9557 Out;    //for touch timing init

const char* ssid = "home6";  
const char* password = "001002003220"; // Set a password  
//https://test.kvant-asbest.club/sport.php
String HOST_NAME   = "http://test.kvant-asbest.club"; // CHANGE IT
String PATH_NAME   = "/sport.php";      // CHANGE IT
//String PATH_NAME   = "/products/arduino.php";      // CHANGE IT
//String queryString = "temperature=26&humidity=70"; // OPTIONAL
HTTPClient http;


uint32_t tick = 0;
uint32_t y=0;


/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
   uint32_t w = ( area->x2 - area->x1 + 1 );
   uint32_t h = ( area->y2 - area->y1 + 1 );

   tft.pushImageDMA(area->x1, area->y1, w, h,(lgfx::rgb565_t*)&color_p->full);

   lv_disp_flush_ready( disp );

}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
   uint16_t touchX, touchY;
   bool touched = tft.getTouch( &touchX, &touchY);
   if( !touched )
   {
      data->state = LV_INDEV_STATE_REL;
   }
   else
   {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touchX;
      data->point.y = touchY;
/*
      Serial.print( "Data x " );
      Serial.println( touchX );

      Serial.print( "Data y " );
      Serial.println( touchY );
*/      
   }
}


void setup()
{
  Serial.begin(115200);
  Serial.println("LVGL Widgets Demo");

  //GPIO init
#if defined (CrowPanel_50) || defined (CrowPanel_70)
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  pinMode(42, OUTPUT);
  digitalWrite(42, LOW);

  //touch timing init
  Wire.begin(19, 20);
  Out.reset();
  Out.setMode(IO_OUTPUT);
  Out.setState(IO0, IO_LOW);
  Out.setState(IO1, IO_LOW);
  delay(20);
  Out.setState(IO0, IO_HIGH);
  delay(100);
  Out.setMode(IO1, IO_INPUT);

#elif defined (CrowPanel_43)
  pinMode(20, OUTPUT);
  digitalWrite(20, LOW);
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  pinMode(35, OUTPUT);
  digitalWrite(35, LOW);
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);
  pinMode(0, OUTPUT);//TOUCH-CS
#endif

  //Display Prepare
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);

  delay(200);

  lv_init();

  delay(100);

  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf1, disp_draw_buf2, screenWidth * screenHeight/10);
  /* Initialize the display */
  lv_disp_drv_init(&disp_drv);
  /* Change the following line to your display resolution */
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.full_refresh = 1;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Initialize the (dummy) input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  tft.fillScreen(TFT_BLACK);

  tft.println("Start..."); 
  // Set to Access Point mode  

  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  tft.println("Connecting to WiFi"); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    tft.print("."); 
  }
  
  Serial.println("\nConnected to WiFi");
  tft.println("Connected to WiFi"); 
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
  tft.setTextSize(4);
  tft.print("IP address: "); 
  tft.println(WiFi.localIP()); 
  
  tft.setTextSize(1);
  delay(5000);

  //please do not use LVGL Demo and UI export from Squareline Studio in the same time.
  // lv_demo_widgets();    // LVGL demo
  ui_init();
  
  lv_timer_t * timer = lv_timer_create(my_timer, 5,  NULL);
  lv_timer_t * timer_wifi = lv_timer_create(my_timer_wifi, 10000,  NULL);

  Serial.println( "Setup done" );

}

void my_timer(lv_timer_t * timer)
{
    //int status      = ;
    //char *tem       = (char *)malloc(24);
    char tem[24];
    
    sprintf(tem, "%d", tick);
    lv_label_set_text(ui_Label3, tem);

    //free(tem); tem=NULL;

//    tft.fillCircle    ( 100, 100      , 50, TFT_YELLOW);

    //tft.setCursor(70, 180);
    //tft.println(tick); 

    tick=tick+1;
    if (tick>10000) {
      tick=0;
    }
    

    lv_arc_set_value(ui_Arc1, tick);
    lv_event_send(ui_Arc1, LV_EVENT_VALUE_CHANGED, 0);

    //lv_chart_set_next_value(ui_Chart1, ui_Chart1_series_1, tem_float);
    //lv_chart_refresh(ui_Chart1);


}

void my_timer_wifi(lv_timer_t * timer)
{


  if (WiFi.status() != WL_CONNECTED) {
    tft.fillCircle    ( 10, 10,6, TFT_RED);
    Serial.println("WiFi not connected!");
    tft.println("WiFi not connected!"); 
    // Попробовать переподключиться
    WiFi.reconnect();
  } else {
    tft.fillCircle    ( 10, 10,6, TFT_GREEN);
  }

  getHTTP();
}

void getHTTP()
{
  
  http.begin(HOST_NAME + PATH_NAME); //HTTP
  http.setTimeout(300);
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      const char* myConstChar = payload.c_str();      
      lv_label_set_text(ui_Label1, myConstChar);
    } else {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

}

void loop()
{
    lv_timer_handler();
    delay(1);
  
}
