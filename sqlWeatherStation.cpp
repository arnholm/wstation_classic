#include "sqlWeatherStation.h"
#include "ck_sqlite3/sqlTable.h"
#include "cf_utils/cf_utils.h"
#include <sstream>
#include <typeinfo>
#include <cmath>

string sqlWeatherStation::table_name()
{
   string raw_name = typeid(sqlWeatherStation).name();
   string name = class_name(raw_name);
   return name;
}

sqlTable* sqlWeatherStation::create_table(sqlDatabase* db)
{
   if(sqlTable* table = db->find_table(table_name())) {
      // table already exist
      return table;
   }

   // object to hold the column definitions
   sqlRow row;
   sqlWeatherStation::declare_row(row);

   // create the table
   return db->table_create(sqlWeatherStation::table_name(),row);
}

void sqlWeatherStation::declare_row(sqlRow& row)
{
   row.clear();
   row.declare(sqlRow::column("time_t" ,sqlRow::column::INTEGER,sqlRow::column::PK_1));
   // indoor sensors
   row.declare(sqlRow::column("itemp"  ,sqlRow::column::DOUBLE));   // indoor temperature
   row.declare(sqlRow::column("ihumi"  ,sqlRow::column::DOUBLE));   // indoor humidity
   // outdoor sensors
   row.declare(sqlRow::column("otemp"  ,sqlRow::column::DOUBLE));   // outdoor temperature
   row.declare(sqlRow::column("ohumi"  ,sqlRow::column::DOUBLE));   // outdoor humidity
   row.declare(sqlRow::column("opres"  ,sqlRow::column::DOUBLE));   // pressure
   row.declare(sqlRow::column("owspd"  ,sqlRow::column::DOUBLE));   // Average wind speed
   row.declare(sqlRow::column("owgus"  ,sqlRow::column::DOUBLE));   // Gust wind speed
   row.declare(sqlRow::column("owdir"  ,sqlRow::column::INTEGER));  // Wind direction
   row.declare(sqlRow::column("orain"  ,sqlRow::column::INTEGER));  // Raw, total rain counter, Rain [mm] = orain * 0.3;
   row.declare(sqlRow::column("osens"  ,sqlRow::column::INTEGER));  // Outdoor sensor contact
}

bool sqlWeatherStation::to_row(const Data& data, sqlRow& row)
{
   row.insert(sqlRow::column("time_t",(int)data.wsp.tstmp));

   row.insert(sqlRow::column("itemp" ,data.wsp.itemp));
   row.insert(sqlRow::column("ihumi" ,data.wsp.ihumi));

   row.insert(sqlRow::column("otemp" ,data.wsp.otemp));
   row.insert(sqlRow::column("ohumi" ,data.wsp.ohumi));
   row.insert(sqlRow::column("opres" ,data.wsp.opres));
   row.insert(sqlRow::column("owspd" ,data.wsp.owspd));
   row.insert(sqlRow::column("owgus" ,data.wsp.owgus));
   row.insert(sqlRow::column("owdir" ,(int)data.wsp.owdir));
   row.insert(sqlRow::column("orain" ,(int)data.wsp.orain));
   row.insert(sqlRow::column("osens" ,(int)data.wsp.osens));
   return true;
}

bool sqlWeatherStation::to_data(sqlRow& row, Data& data)
{
   data.wsp.tstmp = row["time_t"].value_int();
   data.wsp.itemp = row["itemp"].value_double();
   data.wsp.ihumi = row["ihumi"].value_double();

   data.wsp.otemp = row["otemp"].value_double();
   data.wsp.ohumi = row["ohumi"].value_double();
   data.wsp.opres = row["opres"].value_double();
   data.wsp.owspd = row["owspd"].value_double();
   data.wsp.owgus = row["owgus"].value_double();
   data.wsp.owdir = row["owdir"].value_int();
   data.wsp.orain = row["orain"].value_int();
   data.wsp.osens = row["osens"].value_int();
   return true;
}


sqlWeatherStation::sqlWeatherStation(sqlDatabase* db)
: sqlObject(db)
{}

sqlWeatherStation::sqlWeatherStation(const sqlRef& id)
: sqlObject(id)
{
   read(id);
}

sqlWeatherStation::~sqlWeatherStation()
{}

sqlWeatherStation::Data& sqlWeatherStation::data()
{
   return m_data;
}

bool sqlWeatherStation::write(const Data& data, bool assign_object)
{
   if(sqlTable* table = db()->find_table(table_name())) {
      sqlRow row;
      declare_row(row);
      to_row(data,row);
      sqlRef id = table->insert(row);
      set_id(id);
      if(assign_object)m_data = data;
      return (id.id()>0);
   }
   return false;
}

bool sqlWeatherStation::write()
{
   return write(m_data,false);
}


bool sqlWeatherStation::read(const sqlRef& id)        // read data from db
{
  if(sqlTable* table = db()->find_table(table_name())) {
     sqlRow row;
     declare_row(row);
     // Object Id has already been set on this object and passed to this function
     // Just read the values
     if(table->get(id,row)) {
        to_data(row,m_data);
        set_id(id);
        return true;
     }
  }
  return false;
}


time_t sqlWeatherStation::time_utc() const
{
   return m_data.wsp.tstmp;
}

double sqlWeatherStation::itemp() const
{
   return m_data.wsp.itemp;
}

double sqlWeatherStation::ihumi() const
{
   return m_data.wsp.ihumi;
}


double sqlWeatherStation::otemp() const
{
   return m_data.wsp.otemp;
}

double sqlWeatherStation::ohumi() const
{
   return m_data.wsp.ohumi;
}

double sqlWeatherStation::opres(double elevation) const
{
   // http://en.wikipedia.org/wiki/Barometric_formula
   double otemp_K = 273.15 + m_data.wsp.otemp;
   return m_data.wsp.opres / (exp((-9.80665*0.0289664*elevation)/(8.31432*otemp_K)));
}

double sqlWeatherStation::owspd() const
{
   return m_data.wsp.owspd;
}

double sqlWeatherStation::owgus() const
{
   return m_data.wsp.owgus;
}

size_t sqlWeatherStation::owdir() const
{
   return m_data.wsp.owdir;
}

double sqlWeatherStation::owdir_deg() const
{
   return m_data.wsp.owdir*WS_WIND_DEG;
}

double sqlWeatherStation::orain() const
{
   return m_data.wsp.orain*WS_RAIN_MM;
}

size_t sqlWeatherStation::osens() const
{
   return m_data.wsp.osens;
}


void sqlWeatherStation::get_ids(sqlDatabase* db, time_t t_begin, time_t t_end, list<sqlRef>& ids)
{
   ostringstream qout;
   if(t_begin > 0 || t_end > 0) {
      qout << "WHERE ";
      if(t_begin > 0)qout << "ROWID >= " << t_begin;
      if(t_begin > 0 && t_end > 0)qout << " AND ";
      if(t_end   > 0)qout << "ROWID < " << t_end;
   }

   // run the query
   if(sqlTable* table = db->find_table(table_name())) {
      table->select_ids(qout.str(),ids);
   }
}
