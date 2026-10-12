//esp32p4 -->dev module
//esp32p4 -->Core Board
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Index.html.h"
#include "WIFI-INI.h"
#include "MIPI.h"

float sendToWebsiteTimeMs=0;

//C:\Users\Robert\AppData\Local\Arduino15\packages\esp32\tools\esp32p4_es-libs\3.3.7\include\esp_driver_isp\include\driver
//C:\Users\Robert\AppData\Local\Arduino15\packages\esp32\tools\esp32p4_es-libs\3.3.7\include\hal\include\hal
//D:\ESP-IDF\esp-video-components-master\esp_cam_sensor\sensors\ov5647\include
//ipex antenna 2.4 ghz

void setup() 
{
  Serial.begin(115200);

  loginInspecifictWLan("WIFI-Name","Passwort");

  INIT_WIFI();
  INIT_MIPI();
  delay(300);

  INIT_RawPictureToJPEG();

  Serial.println("start finished");

  esp_cache_msync(buffer, width * height * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  esp_cache_msync(buffer2, width * height * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);

}

void updateCameraFPS()
{
  uint32_t now = millis();
  uint32_t elapsed = now - fpsLastMillis;

  // Mindestens 1 Sekunde messen
  if (elapsed >= 1000)
  {
    uint32_t callbacks;
    uint32_t copies;

    noInterrupts();

    callbacks = callbackCount;
    copies = copyCount;

    callbackCount = 0;
    copyCount = 0;

    interrupts();

    // FPS = Anzahl Frames / verstrichene Sekunden
    float elapsedSeconds = elapsed / 1000.0f;

    callbackFPS = callbacks / elapsedSeconds;
    copyFPS = copies / elapsedSeconds;

    fpsLastMillis = now;

    float CPU =0;

    CPU=(copyFrameToBufferMs/1000.0f)/elapsed*100.0f;
    Serial.printf("CPU-ToBuffer: (%.1f %) | ",CPU);
    copyFrameToBufferMs=0;

    CPU = calculateFrameToJPEGMs/elapsed*100;
    Serial.printf("CPU-ToJPEG: (%.1f %) | ",CPU);
    calculateFrameToJPEGMs=0;

    CPU = sendToWebsiteTimeMs/elapsed*100;
    Serial.printf("CPU-SendToWeb: (%.1f %) | ",CPU);
    sendToWebsiteTimeMs=0;

   

    Serial.printf(
        "Callback-FPS: %.2f | Copy-FPS: %.2f | Zeit: %lu ms\n",
        callbackFPS,
        copyFPS,
        (unsigned long)elapsed
    );
  }
}

void sendNewPicture()
{
  if(streamStart)
  {
    uint32_t startMicros = micros();
    
    if(!htmlclient.connected())
    {    
      streamStart=false;
      Serial.println("Bild down html");
      htmlclient.flush();
      htmlclient.print("\r\n");
      htmlclient.stop();       
      return;
    }

    htmlclient.write(&out_bufjpeg[0], out_sizejpeg);
    htmlclient.flush();
    htmlclient.print("\r\n");
    htmlclient.print("--frame\r\n");
    htmlclient.print("Content-Type: image/jpg\r\n\r\n");

    sendToWebsiteTimeMs += (micros() - startMicros)/ 1000.0f;

    
    updateCameraFPS();
  }
  
}

void loop() 
{
  if(CameraNewTransFinished==true)
  {
    RawPictureToJPEG();
    sendNewPicture();
    //delay(100);
    CounterCameraTransFinished++;
    CameraNewTransFinished=false;
    CameraGetNewPicture=true;
  }
  
  WIFI_Tick();
  delay(5);
}
