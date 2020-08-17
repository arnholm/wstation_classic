#include "wsMeasurement.h"
#include "ck_sqlite3/sqlDatabase.h"
#include "ck_sqlite3/sqlTable.h"
#include <iomanip>
#include <sstream>

wsMeasurement::wsMeasurement(sqlDatabase* db)
: sqlObject(db)
, m_off_press(0.0)
, m_off_time(3600)
{
   declare_row();
}

wsMeasurement::wsMeasurement(sqlDatabase* db, WSdata& data)
: sqlObject(db)
, m_data(data)
, m_off_press(0.0)
, m_off_time(3600)
{
    declare_row();
}

wsMeasurement::wsMeasurement(const sqlRef& id)
: sqlObject(id)
, m_off_press(0.0)
, m_off_time(3600)
{
    read(id);
}

wsMeasurement::~wsMeasurement()
{}


string wsMeasurement::table_name()
{
   return "WStest1";
}

void wsMeasurement::declare_row()
{
   m_row.clear();
   m_row.declare(sqlRow::column("time_t",sqlRow::column::INTEGER,sqlRow::column::PK_1));
   m_row.declare(sqlRow::column("ihumi",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("itemp",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("ohumi",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("otemp",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("opres",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("owspd",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("owgus",sqlRow::column::DOUBLE));
   m_row.declare(sqlRow::column("owdir",sqlRow::column::INTEGER));
}

// create the table in the database
sqlTable* wsMeasurement::table_create()
{
   sqlTable* table = db()->table_create(table_name(),m_row);
   sqlRef id(table,-1);
   set_id(id);
   return table;
}


  // write data to db
bool wsMeasurement::write()
{
   m_row.insert(sqlRow::column("time_t",(int)m_data.tstmp));
   m_row.insert(sqlRow::column("ihumi",m_data.ihumi));
   m_row.insert(sqlRow::column("itemp",m_data.itemp));
   m_row.insert(sqlRow::column("ohumi",m_data.ohumi));
   m_row.insert(sqlRow::column("otemp",m_data.otemp));
   m_row.insert(sqlRow::column("opres",m_data.opres));
   m_row.insert(sqlRow::column("owspd",m_data.owspd));
   m_row.insert(sqlRow::column("owgus",m_data.owgus));
   m_row.insert(sqlRow::column("owdir",(int)m_data.owdir));
   if(sqlTable* tab =  db()->find_table(table_name())) {
      sqlRef id = tab->insert(m_row);
      set_id(id);
      return (id.id()>0);
   }
   return false;
}



// read data from db
bool wsMeasurement::read(const sqlRef& id)
{
   if(sqlTable* tab =  db()->find_table(table_name())) {
      if(m_row.size() == 0)declare_row();
      if(tab->get(id,m_row)) {

         m_data.tstmp = m_row["time_t"].value_int();
         m_data.ihumi = m_row["ihumi"].value_double();
         m_data.itemp = m_row["itemp"].value_double();

         m_data.ohumi = m_row["ohumi"].value_double();
         m_data.otemp = m_row["otemp"].value_double();
         m_data.opres = m_row["opres"].value_double();

         m_data.owspd = m_row["owspd"].value_double();
         m_data.owgus = m_row["owgus"].value_double();
         m_data.owdir = m_row["owdir"].value_int();
         return true;
      }
   }
   return false;
}

void wsMeasurement::export_data(ostream& out)
{
   /*
   for(sqlRow::iterator ic = m_row.column_begin(); ic != m_row.column_end(); ic++) {
      out << setw(10) << (ic->second).value() << " ";
   }
   out << endl;
   */

   const size_t buflen = 32;
   char buffer[buflen];
   strftime(buffer,buflen,"%Y%m%d %H:%M:%S",localtime(&m_data.tstmp));

   out << setw(10) << buffer;
   out << setw(10) << m_data.ihumi;
   out << setw(10) << m_data.itemp;

   out << setw(10) << m_data.ohumi;
   out << setw(10) << m_data.otemp;
   out << setw(10) << m_data.opres+m_off_press;

   out << setw(10) << m_data.owspd;
   out << setw(10) << m_data.owgus;
   out << setw(15) << m_data.owdir;
   out << setw(15) << m_data.tstmp+m_off_time;
   out << endl;
}

void wsMeasurement::export_header(ostream& out)
{
   out << "             time     ihumi     itemp     ohumi     otemp     opres     owspd     owgus     owdir         time_t" << endl;
}

void wsMeasurement::get_ids(sqlDatabase* db, time_t t_after, time_t t_before, list<sqlRef>& ids)
{
   ostringstream qout;
   if(t_after > 0 || t_before > 0) {
      qout << "WHERE ";
      if(t_after > 0)qout << "ROWID > " << t_after;
      if(t_after > 0 && t_before > 0)qout << " AND ";
      if(t_before > 0)qout << "ROWID < " << t_before;
   }

   // run the query
   if(sqlTable* table = db->find_table(table_name())) {
      table->select_ids(qout.str(),ids);
   }
}

void wsMeasurement::set_time_offset(int off_time)
{
   m_off_time = off_time;
}

void wsMeasurement::set_pressure_offset(double off_press)
{
   m_off_press = off_press;
}

void wsMeasurement::export_numeric_html(ostream& out)
{

   const size_t buflen = 32;
   char buffer[buflen];
   strftime(buffer,buflen,"%d-%b-%Y",localtime(&m_data.tstmp));
   string date_value = buffer;
   strftime(buffer,buflen,"%H:%M",localtime(&m_data.tstmp));
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
   out << "<td><td>Temperature:</td><td>" << m_data.otemp << " &#176C</td>" << endl;
   out << "<td><td>Pressure:</td> <td>"<< m_data.opres+m_off_press <<" hPa</td>" << endl;
   out << "</tr>" << endl;

   out << "<tr>" << endl;
   out << "<td>Time:</td>   <td>"<< time_value.c_str() << "</td>" << endl;
   out << "<td><td>Wind:</td>     <td>" << m_data.owspd << " m/s</td>" << endl;
   out << "<td><td>Wind dir:</td>  <td>" << m_data.owdir << "</td>" << endl;
   out << "</tr>" << endl;

   out << "<tr>" << endl;
   out << "<td>Humidity:</td><td>" << m_data.ohumi << "%</td>" << endl;
   out << "<td><td>Inside Temp:</td> <td>" << m_data.itemp << " &#176C</td>" << endl;
   out << "<td><td>Inside Humidity:</td> <td>"<< m_data.ihumi << " %</td>" << endl;
   out << "</tr>" << endl;

   out << "</body>" << endl;
   out << "<html>" << endl;

}
