#ifndef SQLWEATHERSTATION_H
#define SQLWEATHERSTATION_H

#include "cpde_sqlite3/sqlObject.h"
#include "wsp/wsp_plus.h"
#include <string>
using namespace std;


class sqlWeatherStation : public sqlObject {
public:

   struct Data {
      Data() {}
      Data(const wsp_data& _wsp) : wsp(_wsp) {}
      wsp_data wsp;
   };

   // simplified table management
   static sqlTable* create_table(sqlDatabase* db);


   // this constructor is used when creating new objects
   sqlWeatherStation(sqlDatabase* id);

   // this constructor is used when restoring existing objects
   sqlWeatherStation(const sqlRef& id);
   virtual ~sqlWeatherStation();

   // Write to database
   bool write(const Data& data, bool assign_object=false);

   // provide access to the raw data
   Data& data();

   // the following are interpreted values of Data
   time_t time_utc() const;
   double itemp() const;
   double ihumi() const;

   double otemp() const;
   double ohumi() const;
   double opres(double elevation=0.0) const;  // pressure adjusted to sea level
   double owspd() const;
   double owgus() const;
   size_t owdir() const;
   double owdir_deg() const;
   double orain() const;
   size_t osens() const;

   // get list of id for a given time range
   static void get_ids(sqlDatabase* db, time_t t_begin, time_t t_end, list<sqlRef>& ids);

public:
   static string table_name();              // table name for this type
   static void declare_row(sqlRow& row);    // row definition for this type

protected:
   // pure virtual functions must be defined
   virtual bool write();                  // write data to db
   virtual bool read(const sqlRef& id);   // read data from db

   static bool to_row(const Data& data, sqlRow& row);   // from Data to sqlRow
   static bool to_data(sqlRow& row, Data& data);  // from sqlRow to Data

private:
   Data m_data;
};

#endif // SQLWEATHERSTATION_H
