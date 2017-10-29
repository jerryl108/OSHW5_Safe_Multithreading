#ifndef CHANGE_NOTIFIER_H
#define CHANGE_NOTIFIER_H

#define sub_iter list<change_subscriber*>::iterator
#include <list>
#include <condition_variable>
#include <mutex>

#include <iostream>
#include <ctime>



/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent. In addition, any number of
 * change notifcations are supported with (sort of) smart assignment.
 */



using namespace std;

//change_notifiers must be declared before adding change_subscribers
//using the default (no arguments) constructor
class change_notifier;

//change_subscribers are declared to suspend a thread until it is
//notified of a change by a change_notifier:
class change_subscriber
{
private:
  condition_variable cv;
  unique_lock<mutex>* unique_l;
  sub_iter it;
  change_notifier* parent;
  int num_globally_notified_changes = 0;
  bool waiting_on_cv = false;
public:
  const clock_t creation_time; //unique identifier

  //initial constructor:
  change_subscriber() : creation_time(clock()) {}

  change_subscriber(const change_subscriber& rhs) = delete;
  change_subscriber& operator=(const change_subscriber& rhs) = delete;

  //must call this function to use:
  void subscribe(change_notifier& p, unique_lock<mutex>& lck);

  //passive wait for changes:
  void wait();

  //don't worry about these, for internal use:
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
  //notify all subscribers of a change:
  void notify_all();
  //notify one subscriber of a change:
  void notify_one();
  //notify n changes to available subscribers:
  void notify(int);

  //ignore these, for internal use:
  void erase(sub_iter it);
  void add_subscriber(change_subscriber& sub);
  friend void change_subscriber::wait();
  ~change_notifier();
};

#endif
