
ESP_C_FILENAMES =  \
  src/api/esp_misc.c \
  src/api/esp_ping.c \
  src/api/esp_sta.c  \
  src/api/esp_ap.c   \
  src/api/esp_conn.c \
  src/esp/esp.c      \
  src/esp/esp_cmd.c  \
  src/esp/esp_recv_buf.c \
  src/esp/esp_parser.c   \
  src/esp/esp_pktbuf.c   \
  src/esp/esp_util.c     \
  src/esp/esp_thread.c

ifeq ($(OS), freertos-v10) 
	ESP_C_FILENAMES += src/system/esp_system_freertos.c 
endif

ESP_C_SOURCES = $(addprefix $(ESP_PROJ_HOME)/, $(ESP_C_FILENAMES))

ESP_C_INCLUDES = $(ESP_PROJ_HOME)/include \

