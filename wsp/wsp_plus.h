#ifndef WSP_PLUS_H
#define WSP_PLUS_H

#ifdef __cplusplus
extern "C" {
#endif

#define WSTATION_VERSION "v1.0-03"
#define WSP_VERSION      "v1.0-b28"

// http://code.google.com/p/weatherpoller/

#define WS_RAIN_MM 0.3
#define WS_WIND_DEG 22.5

typedef struct ws_data_s {
   time_t tstmp;     // timestamp of weather data
   double itemp;     // indoor temperature
   double ihumi;     // indoor humidity
   double otemp;     // outdoor temperature
   double ohumi;     // outdoor humidity
   double opres;     // pressure
   double owspd;     // average wind speed
   double owgus;     // gust wind speed
   size_t owdir;     // wind direction [1-15], 0 for invalid direction
   size_t orain;     // total rain counter. Multiply by WS_RAIN_MM to get total rain in mm
   size_t osens;     // 0 when outside sensor contact lost, otherwise 1
} wsp_data;

void init_wsp_settings();
void get_wsp_data(wsp_data* data);
void out_wsp_data(wsp_data* data);


#ifdef __cplusplus
}
#endif


#endif

