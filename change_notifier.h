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

class change_notifier;

class notification_subscriber
{
private:
  condition_variable cv;
  int change_count = 0;
  unique_lock<mutex>& unique_l;
  bool waiting_on_cv = false;
  sub_iter it;
  change_notifier& parent;
public:
  void wait();
  void notify_change();
  //note: please ONLY use the change_notifier::subscribe function
  //and not this constructor; this class is not meant to be used alone:
  notification_subscriber() = delete;
  notification_subscriber(notification_subscriber&) = delete;
  notification_subscriber(unique_lock<mutex>& ul, change_notifier& p) :
    unique_l(ul), parent(p) {}
  void close();
  void store_iterator(sub_iter i);
};

class change_notifier
{
private:
  list<notification_subscriber> subscriber_list;
  void store_iterator(sub_iter i);
public:
  notification_subscriber subscribe(unique_lock<mutex>& unique_l);
  void notify_all();
  friend void notification_subscriber::close();
  ~change_notifier();
};

#endif
