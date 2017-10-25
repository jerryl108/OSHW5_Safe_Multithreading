#ifndef CHANGE_NOTIFIER
#define CHANGE_NOTIFIER

#include "change_notifier.h"

/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent.
 */

change_subscriber change_notifier::subscribe(unique_lock<mutex>& unique_l)
{
  change_subscriber sub = change_subscriber(unique_l,this);
  cout << "subscriber created" << endl;
  subscriber_list.push_back(sub);
  cout << "subscriber pushed to list" << endl;
  sub_iter it = --subscriber_list.end();
  //make sure another subscriber added by another thread hasn't just interfered:
  while ((*it).unique_l != sub.unique_l) it--;
  cout << "iterator created" << endl;
  (*it).store_iterator(it);
  //cout << "list cc = " << subscriber_list.back().change_count;
  return *it;
}

void change_notifier::notify_all()
{
  cout << "notify_all " << subscriber_list.size() << " subscribers" << endl;
  for (sub_iter it = subscriber_list.begin(); it != subscriber_list.end(); it++)
  {
    cout << "loop iteration" << endl;
    cout << "notify_all waiting_on_cv = " << (*it).waiting_on_cv << endl;
    cout << "notify_all creation_time = " << (*it).creation_time << endl;
    (*it).notify_change();
  }
  cout << "done notifying" << endl;
}

void change_subscriber::store_iterator(sub_iter i)
{
  cout << "store_it waiting_on_cv= " << waiting_on_cv << endl;
  cout << "store_it creation_time= " << creation_time << endl;
  it = i;
  cout << "after store_it waiting_on_cv= " << waiting_on_cv << endl;
  cout << "store_it creation_time= " << creation_time << endl;
}

change_notifier::~change_notifier()
{
  subscriber_list.clear();
}

void change_subscriber::wait()
{
  cout << "change_subscriber wait, cc=" << change_count << endl;
  assert((*it).unique_l == unique_l);
  if (change_count == 0)
  {
    cout << "waiting on cv" << endl;
    waiting_on_cv = true;
    cout <<"wait waiting_on_cv = " << waiting_on_cv << endl;
    cout <<"wait creation_time = " << creation_time << endl;
    cv.wait(*unique_l);
    waiting_on_cv = false;
  }
  else
  {
    cout << "change_count = " << change_count << endl;
    //no need to wait; a change has happened:
    unique_l->unlock();
    cout << "unlocked unique_l" << endl;
    change_count--;
    cout << "decremented change_count" << endl;
    unique_l->lock();
    cout << "re-locked unique_l" << endl;
  }
}

void change_subscriber::notify_change()
{
  cout << "notifying of change" << endl;
  cout << "creation_time = " << creation_time << endl;
  cout << "waiting_on_cv = " << waiting_on_cv << endl;
  if (waiting_on_cv)
  {
    cout << "is waiting" << endl;
    cv.notify_one();
  }
  else
  {
    change_count++;
  }
  cout << "notified" << endl;
}
//should be called at end of subscribing function (instead of default destructor):
void change_subscriber::close()
{
  parent->erase(it);
}

#endif
