#ifndef CHANGE_NOTIFIER_H
#define CHANGE_NOTIFIER_H

#define sub_iter list<change_subscriber>::iterator
#include <list>
#include <condition_variable>
#include <mutex>

#include <iostream>
#include <cassert>
#include <ctime>



/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent.
 */


using namespace std;

class change_subscriber;

class change_notifier
{
private:
  list<change_subscriber> subscriber_list;
  void store_iterator(sub_iter i);
public:
  change_subscriber subscribe(unique_lock<mutex>& unique_l);
  void notify_all();
  void erase(sub_iter it)
  {
    subscriber_list.erase(it);
  }
  ~change_notifier();
};

class change_subscriber
{
private:
  condition_variable cv;
  sub_iter it;
  change_notifier* parent;
  int change_count;
public:
  unique_lock<mutex>* const unique_l;
  bool waiting_on_cv;
  clock_t creation_time;

  //note: please ONLY use the change_notifier::subscribe function
  //and not this constructor; this class is not meant to be used alone:
  change_subscriber() = delete;
  change_subscriber(const change_subscriber& rhs) :
    change_count(rhs.change_count), unique_l(rhs.unique_l),
    waiting_on_cv(rhs.waiting_on_cv), parent(rhs.parent), it(rhs.it), creation_time(clock())
  {
    cout << "copy constructor" << endl;
  }
  change_subscriber& operator=(const change_subscriber& rhs) = delete;
  change_subscriber(unique_lock<mutex>& ul, change_notifier* p) :
    unique_l(&ul), parent(p), change_count(0), waiting_on_cv(false), creation_time(clock()) {}
  //should only be called when erased from parent's subscriber_listlo by
  //the close() function:
  void wait();
  void notify_change();
  //Please call this at the end of the subscribing function instead of
  //letting the default destructor be called:
  void close();
  void store_iterator(sub_iter i);
};

#endif
