#ifndef RUNNINGAVERAGE_H
#define RUNNINGAVERAGE_H

#include <ctime>
#include <queue>
#include <map>
using namespace std;

class RunningAverage {
public:
   typedef pair<time_t,double> ValuePair;
   typedef queue<ValuePair>    ValueQueue;

   RunningAverage(int plen);
   virtual ~RunningAverage();

   // just like a list, push it to the "newest" position in the queue
   void push_back(const ValuePair& value);

   // return the current average value
   double value() const;

   // return the current time for which the value is valid
   time_t time_value() const;

private:
   int        m_plen;   // max length of time period
   ValueQueue m_queue;
   double     m_sum;    // sum of values in queue
};

#endif // RUNNINGAVERAGE_H
