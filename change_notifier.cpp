#ifndef CHANGE_NOTIFIER
#define CHANGE_NOTIFIER

#include "change_notifier.h"

/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent.
 */

void change_subscriber::subscribe(change_notifier& p, unique_lock<mutex>& lck)
{
  unique_l = &lck;
  parent = &p;
  p.add_subscriber(*this);
}

void change_notifier::add_subscriber(change_subscriber& sub)
{
  subscriber_list_mtx.lock();
  subscriber_list.push_back(&sub);
  cout << "subscriber pushed to list" << endl;
  sub_iter it = --subscriber_list.end();
  subscriber_list_mtx.unlock();
  cout << "iterator created" << endl;
  (*it)->store_iterator(it);
  //cout << "list cc = " << subscriber_list.back().change_count;
}

void change_notifier::notify_all()
{
  cout << "notify_all " << subscriber_list.size() << " subscribers" << endl;
  subscriber_list_mtx.lock();
  for (sub_iter it = subscriber_list.begin(); it != subscriber_list.end(); it++)
  {
    cout << "loop iteration" << endl;
    cout << "notify_all waiting_on_cv = " << (*it)->waiting_on_cv << endl;
    cout << "notify_all creation_time = " << (*it)->creation_time << endl;
    (*it)->notify_change();
  }
  subscriber_list_mtx.unlock();
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
  cout << "notifying of change" << esubscriberssubscriberssubscriberssubscriberssubscriberssubscribersndl;
  cout << "creation_time = " << creation_time << endl;
  cout << "waiting_on_cv = " << waiting_on_cv << endl;
  if (waiting_on_cv)
  {
    cout << "is waiting" << endl;
    cv.notify_all();
  }
  else
  { 
    change_count++;
  }
  cout << "notified" << endl;
}

#endif
