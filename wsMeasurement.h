#ifndef WSMEASUREMENT_H
#define WSMEASUREMENT_H

#include "cpde_sqlite3/sqlObject.h"
#include "cpde_sqlite3/sqlRow.h"
#include <iostream>
using namespace std;

class wsMeasurement : public sqlObject {
public:

   struct WSdata {
      size_t  index;    // Weather station history index, 1 for current entry
      time_t  tstmp;    // estimated local time of weather data entry recording
      size_t  mprev;    // minutes since previous save

      double  ihumi;    // indoor humidity [%]
      double  itemp;    // indoor temperature

      double  ohumi;    // outdoor humidity [%]
      double  otemp;    // outdoor temperature
      double  opres;    // outdoor Pressure

      double  owspd;    // outdoor wind speed
      double  owgus;    // outdoor wind gust
      size_t  owdir;    // outdoor wind direction [1-15] : (1=N, 2=NNE, 3=NE .... 15=NNW)
   };

   // this constructor is used when creating new objects
   wsMeasurement(sqlDatabase* db);
   wsMeasurement(sqlDatabase* db, WSdata& data);

   // this constructor is used when restoring existing objects
   wsMeasurement(const sqlRef& id);

   static void get_ids(sqlDatabase* db, time_t t_after, time_t before, list<sqlRef>& ids);

   virtual ~wsMeasurement();

   WSdata& data() { return m_data; }

private:
   static string table_name();     // the table name this object uses

   bool write();                  // write data to db
   bool read(const sqlRef& id);   // read data from db


   // create the table in the database
   sqlTable* table_create();

   // export WSdata header and data to output stream
   void export_header(ostream& out);
   void export_data(ostream& out);

   void set_time_offset(int off_time);
   void set_pressure_offset(double off_press);

   void export_numeric_html(ostream& out);

private:
   void declare_row();

private:
   WSdata  m_data;
   sqlRow  m_row;
   double  m_off_press; // export adjustment of pressure (mbar)
   int     m_off_time;  // export adjustment of time (sec)
};

#endif // WSMEASUREMENT_H
