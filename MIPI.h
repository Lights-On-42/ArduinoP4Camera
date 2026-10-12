
#include <Arduino.h>
#include <Wire.h>
#include <string.h>

#include "esp_cam_ctlr_csi.h"
#include "esp_cam_ctlr_interface.h"
#include "esp_cam_ctlr.h"
#include "esp_cache.h"

#include "esp_heap_caps.h"
#include "esp_ldo_regulator.h"

#include "driver/jpeg_encode.h"
#include "driver/isp_demosaic.h"
#include "driver/ledc.h"
#include "driver/isp.h"

#include <base64.h>


#define I2C_SDA 7
#define I2C_SCL 8
#define OV5647_ADDR 0x36


const int width = 800;
const int height = 640;
static size_t frameSize = (size_t)width * height * 2;

// ============================================================
// Globale Kameravariablen
// ============================================================

esp_cam_ctlr_trans_t trans;
esp_cam_ctlr_trans_t trans2;

isp_proc_handle_t isp_proc = NULL;
esp_cam_ctlr_handle_t handle= NULL;

uint8_t* buffer;
uint8_t* buffer2;

volatile bool CameraGetNewPicture = true;
volatile bool CameraNewTransFinished = false;
volatile int CounterCameraTransFinished = 0;

jpeg_encoder_handle_t encoder_engine;
jpeg_encode_engine_cfg_t engine_cfg;
jpeg_encode_cfg_t enc_cfg;

size_t allocated_size;
uint8_t* out_bufjpeg;
uint32_t out_sizejpeg = 0;

// ============================================================
// Frame-Rate-Messung
// ============================================================

volatile uint32_t callbackCount = 0;
volatile uint32_t copyCount = 0;


float callbackFPS = 0.0f;
float copyFPS = 0.0f;
volatile uint32_t copyFrameToBufferMs = 0;
float calculateFrameToJPEGMs=0;

volatile uint32_t fpsLastMillis = 0;


struct ov5647_reginfo_t {
    uint16_t reg;
    uint8_t val;
};

const ov5647_reginfo_t ov5647_init_800x640[] = {
    // Reset
    {0x0100, 0x00},
    {0x0103, 0x01},
    {0xFD, 10},  // delay

    // PLL & Clock
    {0x3034, 0x18},  // RAW8
    {0x3035, 0x41},
    {0x3036, 0x80},  // PLL Multiplier = 128 → 400 Mbps/Lane
    {0x303c, 0x11},
    {0x3106, 0xf5},

    // Flip/Mirror
    {0x3821, 0x03},
    {0x3820, 0x41},
    {0x3827, 0xec},
    {0x370c, 0x0f},
    {0x3612, 0x59},
    {0x3618, 0x00},
    {0x5000, 0xff},  // ← ALLE Filter an, inkl. LENC
    //{0x5000, 0x00},  // ← ALLE internen Filter aus
    //{0x5001, 0x00},  // ← AWB aus
    //{0x5003, 0x00},  // ← weitere Filter aus
    //{0x5000, 0x7F}, 
    // LSC
    {0x583e, 0xf0},
    {0x583f, 0x20},
    {0x5002, 0x41},
    {0x5003, 0x08},
    {0x5a00, 0x08},

    // Power
    {0x3000, 0x00},
    {0x3001, 0x00},
    {0x3002, 0x00},
    {0x3016, 0x08},
    {0x3017, 0xe0},
    {0x3018, 0x44},  // ← 2 MIPI Lanes! (vorher 0x00 = falsch)
    {0x301c, 0xf8},
    {0x301d, 0xf0},

    // AEC/AGC
    {0x3a18, 0x00},
    {0x3a19, 0xf8},
    {0x3c01, 0x80},
    {0x3c00, 0x40},
    {0x3b07, 0x0c},

    // HTS / VTS
    {0x380c, (1896 >> 8) & 0x1F},
    {0x380d, 1896 & 0xFF},
    {0x380e, (984 >> 8) & 0xFF},
    {0x380f, 984 & 0xFF},

    // Subsampling
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3708, 0x64},
    {0x3709, 0x52},

    // Crop Fenster
    {0x3800, (500 >> 8) & 0x0F},
    {0x3801, 500 & 0xFF},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, ((2624-1) >> 8) & 0x0F},
    {0x3805, (2624-1) & 0xFF},
    {0x3806, ((1954-1) >> 8) & 0x07},
    {0x3807, (1954-1) & 0xFF},

    // Auflösung
    {0x3808, (800 >> 8) & 0x0F},
    {0x3809, 800 & 0xFF},
    {0x380a, (640 >> 8) & 0x7F},
    {0x380b, 640 & 0xFF},

    // Timing Offset
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x00},

    // Analog
    {0x3630, 0x2e}, {0x3632, 0xe2}, {0x3633, 0x23},
    {0x3634, 0x44}, {0x3636, 0x06}, {0x3620, 0x64},
    {0x3621, 0xe0}, {0x3600, 0x37}, {0x3704, 0xa0},
    {0x3703, 0x5a}, {0x3715, 0x78}, {0x3717, 0x01},
    {0x3731, 0x02}, {0x370b, 0x60}, {0x3705, 0x1a},
    {0x3f05, 0x02}, {0x3f06, 0x10}, {0x3f01, 0x0a},

    // AEC
    {0x3a08, 0x01}, {0x3a09, 0x27}, {0x3a0a, 0x00},
    {0x3a0b, 0xf6}, {0x3a0d, 0x04}, {0x3a0e, 0x03},
    {0x3a0f, 0x58}, {0x3a10, 0x50}, {0x3a1b, 0x58},
    {0x3a1e, 0x50}, {0x3a11, 0x60}, {0x3a1f, 0x28},

    // BLC
    {0x4001, 0x02},
    {0x4004, 0x02},
    {0x4000, 0x09},

    // MIPI Takt-Periode (kritisch!)
    {0x4837, 40},  // 1000000000 / (100MHz / 4) = 10 ns

    {0x4050, 0x6e},
    {0x4051, 0x8f},

    // Streaming ON
    //{0x0100, 0x01},
    {0xFFFF, 0x00}
};

// ============================================================
// I2C-Bus scannen
// ============================================================

void scanI2C()
{
    Serial.println();
    Serial.println("================================");
    Serial.println("       I2C-Bus Scan");
    Serial.println("================================");

    uint8_t foundDevices = 0;
    Wire.begin(I2C_SDA, I2C_SCL);

    for (uint8_t address = 0x08; address <= 0x77; address++)
    {

        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.printf(
                "I2C-Geraet gefunden: Adresse 0x%02X (%u)\n",
                address,
                address
            );

            foundDevices++;
        }

        delay(1);
    }

    Serial.println("--------------------------------");

    if (foundDevices == 0)
    {
        Serial.println("Keine I2C-Geraete gefunden.");
    }
    else
    {
        Serial.printf(
            "Scan abgeschlossen: %u Geraet(e) gefunden.\n",
            foundDevices
        );
    }

    Serial.println("================================");
    Serial.println();
}

// Hilfsfunktion zum Schreiben von 16-Bit Register-Adressen
void writeReg(uint16_t reg, uint8_t val) 
{
  Wire.beginTransmission(OV5647_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.write(val);
  Wire.endTransmission();
}

void writeRegList(const ov5647_reginfo_t* list) 
{
    for (int i = 0; list[i].reg != 0xFFFF; i++) 
    { 
        if (list[i].reg == 0xFD) { 
            delay(list[i].val);
        } else {
            writeReg(list[i].reg, list[i].val);
        }
    }
}

uint8_t readReg(uint16_t reg) 
{
  Wire.beginTransmission(OV5647_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.endTransmission(false);
  Wire.requestFrom(OV5647_ADDR, (uint8_t)1);
  return Wire.read();
}

void checkSensorStatus() 
{
    uint8_t mipi_ctrl = readReg(0x4800);
    Serial.print("MIPI Control Register (0x4800): 0x");
    Serial.println(mipi_ctrl, HEX);
    
    if (mipi_ctrl & 0x04) {
        Serial.println("Erfolg: Sensor sendet MIPI Daten!");
    } else {
        Serial.println("Fehler: Sensor ist im Standby.");
    }
}

void start_camera_mclk(int pin, int freq_hz) 
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_1_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = freq_hz, 
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = pin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 1, // 50% Duty Cycle bei 1-Bit Auflösung
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

static bool IRAM_ATTR camera_get_new_trans_cb(esp_cam_ctlr_handle_t handle,
                                               esp_cam_ctlr_trans_t *trans,
                                               void *user_data)
{
    // Buffer aus user_data übergeben
    esp_cam_ctlr_trans_t *t = (esp_cam_ctlr_trans_t *)user_data;
    trans->buffer = t->buffer;
    trans->buflen = t->buflen;
    return true;
}

static bool IRAM_ATTR camera_trans_finished_cb(
    esp_cam_ctlr_handle_t handle,
    esp_cam_ctlr_trans_t *trans,
    void *user_data)
{
    // Jeder Callback-Aufruf wird gezählt
    callbackCount++;

    if (CameraGetNewPicture == true)
    {
      uint32_t startMicros = micros();
      esp_cache_msync(buffer,width * height * 2,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
      CameraGetNewPicture = false;

      // Frame kopieren
      memcpy(buffer2, buffer, frameSize);

      // Tatsächlich kopierte Frames zählen
      copyCount++;
      CameraNewTransFinished = true;

      //esp_cache_msync(buffer,width * height * 2,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
      copyFrameToBufferMs += (micros() - startMicros);
      //copyFrameToBufferMs+=1;
    }
    return true;
}


void setup_mipi_power() 
{
    // Definition des LDO-Kanals (Kanal 3 ist Standard für MIPI D-PHY)
    esp_ldo_channel_handle_t ldo_mipi = NULL;
    
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = 3,          // LDO ID 3
        .voltage_mv = 2500,    // 2500 mV (2,5V)
    };

    // LDO initialisieren und einschalten
    esp_err_t err = esp_ldo_acquire_channel(&ldo_cfg, &ldo_mipi);
    
    if (err == ESP_OK) {
        Serial.println("MIPI D-PHY Power (2.5V) auf LDO Kanal 3 aktiviert.");
    } else {
        Serial.printf("Fehler beim Aktivieren des LDO: 0x%x\n", err);
    }
}

void writeCameraParameter()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  uint16_t chip_id = (readReg(0x300A) << 8) | readReg(0x300B);
  Serial.printf("Sensor Chip ID: 0x%04X\n", chip_id);

  delay(1000);
  writeRegList(ov5647_init_800x640);
  writeReg(0x4800, 0x24);
  writeReg(0x4837, 0x19);   
  delay(200);
}

void INIT_MIPI()
{
  setup_mipi_power();
  start_camera_mclk(13, 24000000);

  writeCameraParameter();
  checkSensorStatus();

  esp_err_t errocode;
  esp_isp_processor_cfg_t isp_config = {
        .clk_hz = 80 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = ISP_COLOR_RAW8,
        .output_data_color_type = ISP_COLOR_RGB565,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = width,
        .v_res = height,
        .bayer_order = COLOR_RAW_ELEMENT_ORDER_GBRG, 
    };
    //.bayer_order = COLOR_RAW_ELEMENT_ORDER_GRBG, 
    //GBRG
  errocode=esp_isp_new_processor(&isp_config, &isp_proc);
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_isp_new_processor: ");
    Serial.println(errocode);
  }
  errocode=esp_isp_enable(isp_proc);
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_isp_enable: ");
    Serial.println(errocode);
  }

  Serial.println("isp work now...");


  size_t buf_size = width * height*2; // RAW8 = 1 Byte pro Pixel!
  buffer = (uint8_t*)heap_caps_aligned_alloc(64, buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);
  buffer2 = (uint8_t*)heap_caps_aligned_alloc(64, buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);

  if (buffer == NULL) 
  {
      Serial.println("Fehler: DMA-fähiger Speicher konnte nicht alloziert werden!");
      return;
  }
  if (buffer2 == NULL) 
  {
      Serial.println("Fehler: DMA-fähiger Speicher konnte nicht alloziert werden Buffer 2!");
      return;
  }
  //memset(buffer, 0, buf_size);

  for(int i=0;i<width*height*2;i++)
  {
    buffer[i]=0;
    buffer2[i]=0;
  }
  Serial.println("PSRAM Buffer bereit.");

  // 2. CSI Controller konfigurieren (wie wir es zuvor gelöst haben)
  esp_cam_ctlr_csi_config_t csi_config= { 0 };
  csi_config.ctlr_id = 0;
  csi_config.h_res = width; 
  csi_config.v_res = height;
  csi_config.data_lane_num = 2;
  csi_config.lane_bit_rate_mbps = 400;
  csi_config.queue_items = 2;
  csi_config.byte_swap_en = 1; //0
  csi_config.bk_buffer_dis = 1;
  csi_config.input_data_color_type = CAM_CTLR_COLOR_RAW8;  // Format vom Sensor
  csi_config.output_data_color_type = CAM_CTLR_COLOR_RGB565; // Format im RAM
  csi_config.byte_swap_en = false;

  
  errocode=esp_cam_new_csi_ctlr(&csi_config, &handle);
  if(errocode!=0)
  {
    Serial.print("Errorcode: ");
    Serial.println(errocode);
  }
  esp_cam_ctlr_evt_cbs_t cbs = 
  {
      .on_get_new_trans = camera_get_new_trans_cb,
      .on_trans_finished = camera_trans_finished_cb,   
  };
  trans.buffer = buffer;              // DEIN reservierter Speicher
  trans.buflen = width * height * 2;  // Die Größe

  trans2.buffer = buffer2;              // DEIN reservierter Speicher
  trans2.buflen = width * height * 2;  // Die Größe
  errocode = esp_cam_ctlr_register_event_callbacks(handle, &cbs, &trans);
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_cam_ctlr_register_event_callbacks: ");
    Serial.println(errocode);
  }



  errocode = esp_cam_ctlr_enable(handle);
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_cam_ctlr_enable: ");
    Serial.println(errocode);
  }

  errocode=esp_cam_ctlr_receive(handle, &trans,1000);
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_cam_ctlr_receive: ");
    Serial.println(errocode);
  }

  errocode=esp_cam_ctlr_receive(handle, &trans2,1000);
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_cam_ctlr_receive trans2: ");
    Serial.println(errocode);
  }
  writeReg(0x0100, 0x01);

  errocode = esp_cam_ctlr_start(handle); 
  if(errocode!=0)
  {
    Serial.print("Errorcode esp_cam_ctlr_start: ");
    Serial.println(errocode);
  }

  Serial.println("Warte auf MIPI-Datenframes...");
}

void INIT_RawPictureToJPEG()
{
  esp_cache_msync(buffer2, width * height * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);

  engine_cfg.intr_priority=0;
  engine_cfg.timeout_ms=100;

  if (jpeg_new_encoder_engine(&engine_cfg, &encoder_engine) != ESP_OK) 
  {
    Serial.println("Fehler: Encoder Engine konnte nicht erstellt werden");
    return;
  }
  jpeg_encode_memory_alloc_cfg_t mem_cfg = {
    .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER
  };
  
  out_bufjpeg = (uint8_t*)jpeg_alloc_encoder_mem(width * height * 2, &mem_cfg, &allocated_size);

  enc_cfg = {
    .height = static_cast<uint32_t>(height),
    .width = static_cast<uint32_t>(width), 
    .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,//JPEG_ENCODE_IN_FORMAT_RGB888,
    .sub_sample = JPEG_DOWN_SAMPLING_YUV422, // Guter Kompromiss Qualität/Größe
    .image_quality = 80                      // Qualität 1-100
  };

  //h264
  //Verbindung verbesser zwischen P4 und C6
}

void RawPictureToJPEG()
{
  uint32_t startMicros = micros();
  esp_err_t ret = jpeg_encoder_process(encoder_engine, &enc_cfg, 
                                     buffer2, (size_t)(width * height * 2),  
                                     out_bufjpeg, allocated_size, &out_sizejpeg);

  calculateFrameToJPEGMs += (micros() - startMicros)/ 1000.0f;

}

