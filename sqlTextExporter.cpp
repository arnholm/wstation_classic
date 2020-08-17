#include "sqlTextExporter.h"
#include <iomanip>

#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/time_clock.hpp>

#include "RunningAverage.h"

sqlTextExporter::sqlTextExporter()
{}

sqlTextExporter::~sqlTextExporter()
{}

int sqlTextExporter::utc_offset_seconds()
{
    using namespace boost::posix_time;
    using namespace boost::date_time;

    // boost::date_time::c_local_adjustor uses the C-API to adjust a
    // moment given in utc to the same moment in the local time zone.
    typedef boost::date_time::c_local_adjustor<ptime> local_adj;
    const ptime utc_now = second_clock<ptime>::universal_time();
    const ptime loc_now = local_adj::utc_to_local(utc_now);

    // compute the difference and return the total seconds
    boost::posix_time::time_duration diff = (loc_now - utc_now);
    return diff.total_seconds();
}

time_t sqlTextExporter::time_instance(int day_offset)
{
   time_t now;
   time(&now);

   return now + day_offset*24*60*60;
}

double sqlTextExporter::rain_intensity(time_t now, int seconds_before, RainMap& rain)
{
   time_t time_begin = now - seconds_before;
   double orain = rain[now];
   double irain = 0.0;

   RainMap::const_iterator itlb = rain.lower_bound(time_begin);
   if(itlb != rain.end()) {
      // time found
      time_t time_before  = itlb->first;
      double orain_before = itlb->second;

      // actual length of period
      double delta_sec  = now - time_before;
      if(delta_sec > 0) {
         double delta_rain = orain - orain_before;
         double factor     = double(seconds_before)/delta_sec;
         irain = delta_rain*factor;
      }
   }

   return irain;
}


bool sqlTextExporter::export_list(list<sqlRef>& ids, int utc_offset, double elevation, double orain_datum, ostream& out, double& irain_last)
{
   RainMap rain;
   irain_last = 0.0;

   const size_t plen24h = 60*60*24;

   RunningAverage otemp24h(plen24h);
   RunningAverage itemp24h(plen24h);

   out << "# wstation Gnuplot export file. UTC offset used [sec] = " << utc_offset << ". Elevation [m] = " << elevation << endl;
   out << '#'
       << setw(14) << "timestamp  " << " "
       << setw(15) << "itemp[C]"    << " "
       << setw(15) << "ihumi[%]"    << " "
       << setw(15) << "otemp[C]"    << " "
       << setw(15) << "ohumi[%]"    << " "
       << setw(15) << "opres[mbar]" << " "
       << setw(15) << "owspd[m/s]"  << " "
       << setw(15) << "owgus[m/s]"  << " "
       << setw(15) << "owdir[deg]"  << " "
       << setw(15) << "orain[mm]"   << " "
       << setw(15) << "irain1[mm/h]" << " "
       << setw(15) << "irain24[mm/24h]" << " "
       << setw(15) << "timestamp24h" << " "
       << setw(15) << "itemp24h[C]" << " "
       << setw(15) << "otemp24[C]"  << " "
       << setw(15) << "osens[1/0]"  << " "
       << "Time" << endl;

   for(list<sqlRef>::iterator i=ids.begin(); i != ids.end(); i++) {
      sqlRef& id = *i;

      sqlWeatherStation ws(id);

      time_t timestamp = ws.time_utc()+utc_offset;

      // use gmtime always, since we adjust the timestamp manually for local time
      const size_t buflen = 32;
      char buffer[buflen];
      strftime(buffer,buflen,"%Y%m%d %H:%M:%S",gmtime(&timestamp));

      double orain = ws.orain() - orain_datum;
      if(orain < 0.0)orain = 0.0;
      rain[timestamp] = orain;

      // compute rain intensity over one hour and 24 hours
      double irain1  = rain_intensity(timestamp,  1*3600,rain);
      double irain24 = rain_intensity(timestamp, 24*3600,rain);

      // add to calculation of running averages
      otemp24h.push_back(make_pair(timestamp,ws.otemp()));
      itemp24h.push_back(make_pair(timestamp,ws.itemp()));

      out << setw(15) << timestamp << " "
          << setw(15) << ws.itemp() << " "
          << setw(15) << ws.ihumi() << " "
          << setw(15) << ws.otemp() << " "
          << setw(15) << ws.ohumi() << " "
          << setw(15) << ws.opres(elevation) << " "
          << setw(15) << ws.owspd() << " "
          << setw(15) << ws.owgus() << " "
          << setw(15) << ws.owdir_deg() << " "
          << setw(15) << ws.orain() << " "
          << setw(15) << irain1     << " "
          << setw(15) << irain24    << " "
          << setw(15) << otemp24h.time_value()  << " "
          << setw(15) << itemp24h.value()  << " "
          << setw(15) << otemp24h.value()  << " "
          << setw(15) << ws.osens() << " "
          << buffer << endl;

      irain_last = irain1;

   }

   return true;
}


void sqlTextExporter::export_numeric_html(list<sqlRef>& ids, int utc_offset, double elevation, double irain, ostream& out)
{
   sqlWeatherStation ws(ids.back());

   time_t timestamp = ws.time_utc()+utc_offset;

   // use gmtime always, since we adjust the timestamp manually for local time
   const size_t buflen = 32;
   char buffer[buflen];
   strftime(buffer,buflen,"%d-%b-%Y",gmtime(&timestamp));
   string date_value = buffer;
   strftime(buffer,buflen,"%H:%M",gmtime(&timestamp));
   string time_value = buffer;


   out << "<html>" << endl;
   out << "<head>" << endl;

   out << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">" << endl;
   out << "<style>body {font-family:Verdana;}</style>" << endl;
   out << "<body>" << endl;

   out << "<b>Latest values:</b>" << endl;
   out << "<table border=0 bgcolor=\"#eeeeee\" width=\"800\">" << endl;

   out << "<tr>" << endl;
   out << "<td>Date:</td>   <td>" << date_value.c_str() << "</td>" << endl;
   out << "<td><td>Temperature:</td><td>" << ws.otemp() << " &#176C</td>" << endl;
   out << "<td><td>Pressure:</td> <td>"<< ws.opres(elevation) <<" hPa</td>" << endl;
   out << "</tr>" << endl;

   out << "<tr>" << endl;
   out << "<td>Time:</td>   <td>"<< time_value.c_str() << "</td>" << endl;
   out << "<td><td>Humidity:</td><td>" << ws.ohumi() << "%</td>" << endl;
   out << "<td><td>Wind-avg.:</td>  <td>" << ws.owspd() << " m/s</td>" << endl;
   out << "</tr>" << endl;

   out << "<tr>" << endl;
   out << "<td>Inside Temp:</td> <td>" << ws.itemp() << " &#176C</td>" << endl;
   out << "<td><td>Rain-acc:</td>  <td>" << ws.orain() << " mm</td>" << endl;
   out << "<td><td>Wind-gust:</td>  <td>" << ws.owgus() << " m/s</td>" << endl;
   out << "</tr>" << endl;

   out << "<tr>" << endl;
   out << "<td>Inside Humidity:</td> <td>"<< ws.ihumi() << " %</td>" << endl;
   out << "<td><td>Rain/hour:</td>  <td>" << irain << " mm/h</td>" << endl;
   out << "<td><td>Wind dir:</td>  <td>" << ws.owdir_deg() << "</td>" << endl;
   out << "</tr>" << endl;

   out << "<tr> <th colspan=\"8\"> <hr> </th> </tr>" << endl;

   out << "<tr>" << endl;
   out << "<td>UTC offset:</td> <td>"<< utc_offset/60 << " min.</td>" << endl;
   out << "<td><td>Elevation</td>  <td>" << elevation << " m</td>" << endl;
   out << "<td><td> </td>  <td>" << WSTATION_VERSION << "</td>" << endl;
   out << "</tr>" << endl;

   out << "</table>" << endl;
   out << "</body>" << endl;
   out << "<html>" << endl;

}
