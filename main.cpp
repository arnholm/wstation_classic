
#include <wx/defs.h>
#include <wx/platinfo.h>  // platform info
#include <wx/app.h>

#include <wx/string.h>   // wxString
#include <wx/filefn.h>   // File Functions
#include <wx/filename.h> // wxFileName
#include <wx/file.h>     // wxFile
#include <wx/cmdline.h>  // command line parser

#include <boost/lexical_cast.hpp>
#include "wsp/wsp_plus.h"

#include "ck_sqlite3/sqlTransaction.h"
#include "ck_sqlite3/sqlQuery.h"

#include "sqlWeatherStation.h"
#include "sqlTextExporter.h"
#include "wsMeasurement.h"  // the old schema

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
using namespace std;
typedef map<wxString,wxString> CmdLineMap;    // CmdLineMap

/*
   switch 	  This is a boolean option which can be given or not, but which doesn't have any value.
              We use the word switch to distinguish such boolean options from more generic options
              like those described below. For example, -v might be a switch meaning "enable verbose mode".

   option 	  Option for us here is something which comes with a value unlike a switch.
              For example, -o:filename might be an option which allows to specify the name of the output file.

   parameter  This is a required program argument.

   More info at: http://docs.wxwidgets.org/2.8/wx_wxcmdlineparser.html#wxcmdlineparser

*/

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
  //   kind            shortName          longName           description                                                                parameterType          flag(s)
  { wxCMD_LINE_PARAM,  wxT_2("database"),   wxT_2("database"),   wxT_2("   <database filename>"),                                             wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY  },
  { wxCMD_LINE_SWITCH, wxT_2("i"),          wxT_2("init_db") ,   wxT_2("   Initialise new database"),                                         wxCMD_LINE_VAL_NONE,   wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_SWITCH, wxT_2("l"),          wxT_2("latest") ,    wxT_2("   Show latest data on screen (1 entry)"),                            wxCMD_LINE_VAL_NONE,   wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_SWITCH, wxT_2("s"),          wxT_2("store") ,     wxT_2("   Store latest data to database (1 entry)"),                         wxCMD_LINE_VAL_NONE,   wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_OPTION, wxT_2("xd"),         wxT_2("xdays") ,     wxT_2("   Export <num> days to standard output"),                            wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_OPTION, wxT_2("xe"),         wxT_2("xelev"),      wxT_2("      Export: Elevation above sea level, <str>=[m]"),                 wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_OPTION, wxT_2("xr"),         wxT_2("xrdat"),      wxT_2("      Export: Rain level datum, <str>=[mm]"),                         wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_OPTION, wxT_2("xh"),         wxT_2("xhtml"),      wxT_2("      Export: latest numeric values to html file, <str>=[filename]"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_SWITCH, wxT_2("xu"),         wxT_2("xutc") ,      wxT_2("      Export: use UTC time instead of local time"),                   wxCMD_LINE_VAL_NONE,   wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_OPTION, wxT_2("co"),         wxT_2("conv"),       wxT_2("   Convert old data <str>=WStest1"),                                  wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL    },
  { wxCMD_LINE_NONE,   wxT_2(""),           wxT_2(""),           wxT_2(""),                                                                   wxCMD_LINE_VAL_NONE,   wxCMD_LINE_PARAM_OPTIONAL    }
};

void ParserToMap(wxCmdLineParser& parser, CmdLineMap& cmdMap)
{
   size_t pcount = sizeof(cmdLineDesc)/sizeof(wxCmdLineEntryDesc) - 1;
   for(size_t i=0; i<pcount; i++) {
      wxString pname = cmdLineDesc[i].longName;
      if(cmdLineDesc[i].kind == wxCMD_LINE_PARAM) {
         cmdMap.insert(make_pair(pname,parser.GetParam(0)));
      }
      else {
         // switch or option, mush check if present
         if(parser.Found(pname)) {
            wxString pvalue;
            if(cmdLineDesc[i].type == wxCMD_LINE_VAL_STRING) {
               parser.Found(pname,&pvalue);
            }
            else if(cmdLineDesc[i].type == wxCMD_LINE_VAL_NUMBER) {
               long lvalue=0;
               parser.Found(pname,&lvalue);
               pvalue.Printf(wxT("%i"),lvalue);
            }
            cmdMap.insert(make_pair(pname,pvalue));
         }
      }
   }
}

void convert_WStest1(sqlDatabase* db)
{
   time_t t_begin = sqlTextExporter::time_instance(-365*50);
   time_t t_end   = sqlTextExporter::time_instance(1);
   list<sqlRef> ids;
   wsMeasurement::get_ids(db,t_begin,t_end,ids);
   cout << "Found " << ids.size() << " measurements of old type 'WStest1' to convert. " << endl;

   // make sure new schema exists
   sqlWeatherStation::create_table(db);

   if(ids.size() > 0) {
      cout << "Starting conversion ..." << endl;

      sqlTransaction trans(db);

      for(list<sqlRef>::iterator id=ids.begin(); id!=ids.end(); id++) {

         wsMeasurement old(*id);

         wsMeasurement::WSdata& odata = old.data();
         wsp_data data;

         data.tstmp = odata.tstmp;     // timestamp of weather data
         data.itemp = odata.itemp;     // indoor temperature
         data.ihumi = odata.ihumi;     // indoor humidity
         data.otemp = odata.otemp;     // outdoor temperature
         data.ohumi = odata.ohumi;     // outdoor humidity
         data.opres = odata.opres;     // pressure
         data.owspd = odata.owspd;     // average wind speed
         data.owgus = odata.owgus;     // gust wind speed
         data.owdir = odata.owdir;     // wind direction [1-15], 0 for invalid direction
         data.orain = 0;               // total rain counter. Multiply by WS_RAIN_MM to get total rain in mm
         data.osens = 1;               // 0 when outside sensor contact lost, otherwise 1

         sqlWeatherStation ws(db);
         ws.write(sqlWeatherStation::Data(data));
      }


      time_t now;
      time(&now);

      const size_t buflen = 32;
      char buffer[buflen];
      strftime(buffer,buflen,"%Y%m%d",gmtime(&now));

      ostringstream out;
      out << "ALTER TABLE 'WSTest1' RENAME TO 'WSTest1_bck" << buffer << "'";
      string query_string = out.str();

      cout << "Conversion completed, renaming old data table: " << query_string << endl;

      sqlQuery query(db,query_string);
      if(!query.ok()) {
         cout << query.message() << endl;
      }
   }

}

inline std::string cnv(const wxString& wx_string)
{
#if wxCHECK_VERSION(3, 0, 0)
   return wx_string.ToStdString();
#else
   return std::string(wx_string.fn_str());
#endif
}

int main(int argc, char **argv)
{
   // initialise wxWidgets library
   wxInitializer initializer(argc,argv);

   // parse command line
   wxCmdLineParser parser(cmdLineDesc);
   parser.SetSwitchChars(wxT("-"));
   parser.SetCmdLine(argc,argv);
   if(parser.Parse() != 0) {
      // command line parameter error
      return 1;
   }

   // convert parameters to map
   CmdLineMap cmdMap;
   ParserToMap(parser,cmdMap);

   bool init_db   = cmdMap.find(wxT("init_db"))  != cmdMap.end();
   bool latest    = cmdMap.find(wxT("latest"))   != cmdMap.end();
   bool store     = cmdMap.find(wxT("store"))  != cmdMap.end();
   bool xdays     = cmdMap.find(wxT("xdays"))    != cmdMap.end();
   bool xelev     = cmdMap.find(wxT("xelev"))    != cmdMap.end();
   bool xhtml     = cmdMap.find(wxT("xhtml"))    != cmdMap.end();
   bool xutc      = cmdMap.find(wxT("xutc"))     != cmdMap.end();
   bool conv      = cmdMap.find(wxT("conv"))     != cmdMap.end();
   bool xrdat     = cmdMap.find(wxT("xrdat"))    != cmdMap.end();

   // get the database name
   wxFileName  database_filename(cmdMap[wxT("database")]);
   string db_name = cnv(database_filename.GetFullPath());

   // orain datum, for computing rain intensities
   double orain_datum = 0.0;
   if(xrdat) {
      string xrdat_val = cnv(cmdMap[wxT("xrdat")]);
      orain_datum     = boost::lexical_cast<double>(xrdat_val);
   }

   // default sea level elevation
   double elevation = 0.0;

   // compute local time offset in seconds, relative to UTC
   // positive value means ahead of UTC
   // This value is only used in exported tables & reports
   int utc_offset = (xutc)? 0 : sqlTextExporter::utc_offset_seconds();


   // prepare for using the wsp_plus interface to read weather station data
   init_wsp_settings();


   if(sqlDatabase* db = sqlDatabase::open_create(db_name)) {


      if(init_db) {
         // initialise the database
         sqlWeatherStation::create_table(db);
      }

      if(conv) {
         // read and cpnvert any old data
         string table_name = cnv(cmdMap[wxT("conv")]);
         if(table_name == "WStest1") {
             convert_WStest1(db);
         }
         else {
            cout << "Conversion of table data " << table_name << " is not supported." << endl;
         }
      }

      // ================ data storeion

      if(store || latest) {
         // read data from the weather station
         wsp_data data;
         get_wsp_data(&data);

         if(latest && !xdays) {
            // echo to screen, but not if export has been requested
            cout << endl << "wstation " << WSTATION_VERSION << ", based on wsp " << WSP_VERSION << endl;
            out_wsp_data(&data);

            cout << endl << "UTC offset : " << utc_offset << " [sec] (positive value means ahead of UTC) " << endl;
         }

         if(store) {
            // write to database
            sqlWeatherStation ws(db);
            ws.write(sqlWeatherStation::Data(data));
         }
      }

      // ================ export from database

      if(xelev) {
         // millibar pressure adjustment
         string xelev_val = cnv(cmdMap[wxT("xelev")]);
         elevation     = boost::lexical_cast<double>(xelev_val);
      }

      if(xdays) {
         // report latess N days to output

         string xdays_val =  cnv(cmdMap[wxT("xdays")]);
         int day_offset   = boost::lexical_cast<int>(xdays_val);

         time_t t_begin = sqlTextExporter::time_instance(-day_offset);
         time_t t_end   = sqlTextExporter::time_instance(0);

         list<sqlRef> ids;
         sqlWeatherStation::get_ids(db,t_begin,t_end,ids);

         // export data and compute rain intensities (i.e. mm/h)
         // latest rain intensity is returned as irain.
         double irain = 0;
         sqlTextExporter::export_list(ids,utc_offset,elevation,orain_datum,cout,irain);

         if(xhtml) {
            // report latest db entry in HTML format
            string xhtml_filename =  cnv(cmdMap[wxT("xhtml")]);
            ofstream out(xhtml_filename.c_str());
            sqlTextExporter::export_numeric_html(ids,utc_offset,elevation,irain,out);
         }

      }

   }

   return 0;
}

