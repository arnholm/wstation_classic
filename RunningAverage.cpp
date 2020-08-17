#include "RunningAverage.h"

RunningAverage::RunningAverage(int plen)
: m_plen(plen)
, m_sum(0.0)
{}

RunningAverage::~RunningAverage()
{}


void RunningAverage::push_back(const ValuePair& value)
{
   m_queue.push(value);
   time_t end_time = value.first;
   m_sum += value.second;

   while((end_time-m_queue.front().first) > m_plen) {
      m_sum -= m_queue.front().second;
      m_queue.pop();
   }
}


double RunningAverage::value() const
{
   double average = 0.0;
   size_t qsize = m_queue.size();
   if(qsize > 0) {
      average = m_sum/double(qsize);
      double qlen = double(m_queue.back().first) - double(m_queue.front().first);
      if(qlen/double(m_plen) < 0.9) {
         average = 0.0/0.0;
      }
   }
   return average;
}

time_t RunningAverage::time_value() const
{
   time_t av_time = 0;
   size_t qsize = m_queue.size();
   if(qsize > 0) {
      int dsec = difftime(m_queue.back().first,m_queue.front().first);
      av_time = m_queue.front().first + dsec/2;
   }
   return av_time;
}
