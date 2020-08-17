#ifndef SQLTEXTEXPORTER_H
#define SQLTEXTEXPORTER_H

#include "sqlWeatherStation.h"
#include <iostream>
#include <map>
using namespace std;

class sqlTextExporter {
public:
   typedef map<time_t,double> RainMap;

   sqlTextExporter();
   virtual ~sqlTextExporter();

   static int utc_offset_seconds();

   static time_t time_instance(int day_offset);

   // utc_offset is difference from UTC in seconds. A positive value means we are ahead of UTC.
   // We use gnuplot to read time values in "%s" format, i.e. time_t values, and time_t are UTC values.
   // In order to get values plotted against local time, we add the utc_offset. Obviously zero offset becomes UTC.
   //
   // elevation is meters above sea level. Used for adjusting pressure to sea level
   //
   // orain_datum is the last rain sensor level (in [mm]) before registration of rain levels in the database,
   // assuming this much rain was actually recorded by the sensor *before* database registrations began.
   // This value is then subtracted from orain in irain calculations, thus avoiding artificial large jumps at the
   // start of the rain intensity series. This value does not affect orain series itself, so the datum value can be found
   // by producing a test export for the time around nstallation. The datum is then the first non-zero value in the orain column.
   //
   // irain is the latest mm/h rain value computed in the given range
   static bool export_list(list<sqlRef>& ids, int utc_offset, double elevation, double orain_datum, ostream& out, double& irain);

   static void export_numeric_html(list<sqlRef>& ids, int utc_offset, double xmbar_offset, double irain, ostream& out);
private:

   static double rain_intensity(time_t now, int seconds_before, RainMap& rain);
};

#endif // SQLTEXTEXPORTER_H
