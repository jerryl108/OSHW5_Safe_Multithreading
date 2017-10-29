#ifndef CHANGE_NOTIFIER_H
#define CHANGE_NOTIFIER_H

#define sub_iter list<change_subscriber*>::iterator
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

class change_notifier;

class change_subscriber
{
private:
  condition_variable cv;
  unique_lock<mutex>* unique_l;
  sub_iter it;
  change_notifier* parent;
  int num_globally_notified_changes = 0;
public:
  bool waiting_on_cv = false;
  clock_t creation_time; //unique identifier

  //initial constructor:
  change_subscriber() : creation_time(clock()) {}

  change_subscriber(const change_subscriber& rhs) = delete;

  change_subscriber& operator=(const change_subscriber& rhs) = delete;

  void subscribe(change_notifier& p, unique_lock<mutex>& lck);
  void wait();
  void notify_change();
  bool unfreeze();
  void store_iterator(sub_iter i);
  ~change_subscriber();
};

class change_notifier
{
private:
  mutex unhandled_changes_mtx;
  int num_unhandled_changes;
  int num_waiting;
  mutex subscriber_list_mtx;
  list<change_subscriber*> subscriber_list;
  void store_iterator(sub_iter i);
public:
  change_subscriber subscribe(unique_lock<mutex>& unique_l);
  void notify_all();
  void notify_one();
  void notify(int num_changes);
  void erase(sub_iter it);
  void add_subscriber(change_subscriber& sub);
  friend void change_subscriber::wait();
  ~change_notifier();
};

#endif
