#ifndef CHANGE_NOTIFIER
#define CHANGE_NOTIFIER

#include "change_notifier.h"

/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent. In addition, any number of
 * change notifcations are supported with (sort of) smart assignment.
 */


void change_notifier::add_subscriber(change_subscriber& sub)
{
  subscriber_list_mtx.lock();
  subscriber_list.push_back(&sub);
  cout << "subscriber pushed to list" << endl;
  sub_iter it = --subscriber_list.end();
  subscriber_list_mtx.unlock();
  cout << "iterator created" << endl;
  (*it)->store_iterator(it);
  //cout << "list cc = " << subscriber_list.back().num_globally_notified_changes;
}

void change_notifier::notify_all()
{
  //This function handles changes that all subscribers should be notified of:
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

void change_notifier::notify_one()
{
  //This function notifies of changes that only one subscriber
  // needs to be be notified of:

  unhandled_changes_mtx.lock();
  subscriber_list_mtx.lock();

  //if all subscribers are waiting, unfreeze one of their condition_variables:
  if (num_waiting == subscriber_list.size())
  {
    //handle change:
    (*subscriber_list.begin())->unfreeze();
  }
  else
  {
    //otherwise, the change will be notified whenever wait() is called:
    num_unhandled_changes++;
  }
  subscriber_list_mtx.unlock();
  unhandled_changes_mtx.unlock();
}


void change_notifier::notify(int num_changes)
{
  //This function notifies of changes that only one subscriber
  // needs to be be notified of:

  unhandled_changes_mtx.lock();
  subscriber_list_mtx.lock();

  //if too many subscribers are waiting, unfreeze condition_variables:
  int num_not_waiting = subscriber_list.size()-num_waiting;
  if (num_changes >= num_not_waiting)
  {
    //handle changes:
    for (sub_iter it = subscriber_list.begin(); it != subscriber_list.end(); it++)
    {
      if ((*it)->unfreeze() == false) num_unhandled_changes++;
      num_changes--;
      if (num_changes == 0) break;
    }
  }
  else
  {
    //otherwise, the change will be notified whenever wait() is called:
    num_unhandled_changes+= num_changes;
  }
  subscriber_list_mtx.unlock();
  unhandled_changes_mtx.unlock();
}


void change_notifier::erase(sub_iter it)
{
  subscriber_list_mtx.lock();
  cout << "erasing subscriber from list" << endl;
  subscriber_list.erase(it);
  subscriber_list_mtx.unlock();
}

change_notifier::~change_notifier()
{
  subscriber_list.clear();
}

void change_subscriber::subscribe(change_notifier& p, unique_lock<mutex>& lck)
{
  unique_l = &lck;
  parent = &p;
  p.add_subscriber(*this);
}

void change_subscriber::store_iterator(sub_iter i)
{
  cout << "store_it waiting_on_cv= " << waiting_on_cv << endl;
  cout << "store_it creation_time= " << creation_time << endl;
  it = i;
  cout << "after store_it waiting_on_cv= " << waiting_on_cv << endl;
  cout << "store_it creation_time= " << creation_time << endl;
}

bool change_subscriber::unfreeze()
{
  //This function notifies of singly-notified changes if waiting_on_cv:
  //(otherwise, they will automatically be notified of when wait() is called)
  if (waiting_on_cv)
  {
    cv.notify_one();
    return true;
  }
  else
    return false;
}

void change_subscriber::notify_change()
{
  //This function makes sure a globally notified change is handled:
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
    num_globally_notified_changes++;
  }
  cout << "notified" << endl;
}


void change_subscriber::wait()
{
  cout << "change_subscriber wait, cc=" << num_globally_notified_changes << endl;
  if (num_globally_notified_changes == 0)
  {
    parent->unhandled_changes_mtx.lock();
    if (parent->num_unhandled_changes >= 0)
    {
      unique_l->unlock();
      parent->num_unhandled_changes--;
      parent->unhandled_changes_mtx.unlock();
      unique_l->lock();
    }
    else
    {
      parent->num_waiting++;
      parent->unhandled_changes_mtx.unlock();
      cout << "waiting on cv" << endl;
      waiting_on_cv = true;
      cout <<"wait waiting_on_cv = " << waiting_on_cv << endl;
      cout <<"wait creation_time = " << creation_time << endl;
      cv.wait(*unique_l);
      parent->num_waiting--;
      waiting_on_cv = false;
    }
  }
  else
  {
    cout << "num_globally_notified_changes = " << num_globally_notified_changes << endl;
    //no need to wait; a change has happened:
    unique_l->unlock();
    cout << "unlocked unique_l" << endl;
    num_globally_notified_changes--;
    cout << "decremented num_globally_notified_changes" << endl;
    unique_l->lock();
    cout << "re-locked unique_l" << endl;
  }
}

change_subscriber::~change_subscriber()
{
  parent->erase(it);
}

#endif
