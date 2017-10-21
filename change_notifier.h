#ifndef CHANGE_NOTIFIER_H
#define CHANGE_NOTIFIER_H

#define sub_iter list<notification_subscriber>::iterator
#include "change_notifier.h"
#include <list>
#include <condition_variable>
#include <mutex>

/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent.
 */


using namespace std;

class notification_subscriber;

class change_notifier
{
private:
  list<notification_subscriber> subscriber_list;
  void store_iterator(sub_iter i);
public:
  notification_subscriber subscribe(unique_lock<mutex>& unique_l);
  void notify_all();
  void erase(sub_iter it)
  {
    subscriber_list.erase(it);
  }
  ~change_notifier();
};

class notification_subscriber
{
private:
  condition_variable cv;
  int change_count = 0;
  unique_lock<mutex>* unique_l;
  bool waiting_on_cv = false;
  sub_iter* it;
  change_notifier* parent;
public:
  //note: please ONLY use the change_notifier::subscribe function
  //and not this constructor; this class is not meant to be used alone:
  notification_subscriber() = delete;
  notification_subscriber(const notification_subscriber& rhs)
  {
    change_count = rhs.change_count;
    unique_l = move(rhs.unique_l);
    waiting_on_cv = rhs.waiting_on_cv;
    it = rhs.it;
    parent = rhs.parent;
  }
  notification_subscriber& operator=(const notification_subscriber& rhs)
  {
    change_count = rhs.change_count;
    unique_l = move(rhs.unique_l);
    waiting_on_cv = rhs.waiting_on_cv;
    it = rhs.it;
    parent = rhs.parent;
  }
  notification_subscriber(unique_lock<mutex>& ul, change_notifier& p) :
    unique_l(&ul), parent(&p) {}
  //should only be called when erased from parent's subscriber_list by
  //the close() function:
  void wait();
  void notify_change();
  //Please call this at the end of the subscribing function instead of
  //letting the default destructor be called:
  void close();
  void store_iterator(sub_iter i);
};

#endif
