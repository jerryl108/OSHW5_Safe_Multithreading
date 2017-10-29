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
  //add subscriber to change_notifier subscriber_list:
  subscriber_list_mtx.lock();
  subscriber_list.push_back(&sub);
  //cout << "subscriber pushed to list" << endl;
  sub_iter it = --subscriber_list.end();
  subscriber_list_mtx.unlock();
  //cout << "iterator created" << endl;
  (*it)->store_iterator(it);
}

void change_notifier::notify_all()
{
  //This function handles changes that all subscribers should be notified of:
  //cout << "notify_all " << subscriber_list.size() << " subscribers" << endl;
  subscriber_list_mtx.lock();
  //notify all subscribers:
  for (sub_iter it = subscriber_list.begin(); it != subscriber_list.end(); it++)
  {
    //cout << "notify_all creation_time = " << (*it)->creation_time << endl;
    (*it)->notify_change();
  }
  subscriber_list_mtx.unlock();
  //cout << "done notifying" << endl;
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
    //otherwise, the change will be notified of whenever wait() is called:
    num_unhandled_changes++;
  }
  subscriber_list_mtx.unlock();
  unhandled_changes_mtx.unlock();
}


void change_notifier::notify(int changes_to_notify)
{
  //This function notifies of multiple changes for which
  // only one subscriber (each) needs to be be notified:

  unhandled_changes_mtx.lock();
  subscriber_list_mtx.lock();

  //if too many subscribers are waiting, unfreeze condition_variables:
  int subscribers_not_waiting = subscriber_list.size()-num_waiting;
  if (changes_to_notify >= subscribers_not_waiting)
  {
    //distribute changes:
    for (sub_iter it = subscriber_list.begin(); it != subscriber_list.end(); it++)
    {
      //subscribers that are not waiting will see changes when wait() is called
      if ((*it)->unfreeze() == false) num_unhandled_changes++;

      changes_to_notify--;
      if (changes_to_notify == 0) break;
    }
  }
  else
  {
    //otherwise, all changes will be notified of whenever wait() is called:
    num_unhandled_changes+= changes_to_notify;
  }
  subscriber_list_mtx.unlock();
  unhandled_changes_mtx.unlock();
}


void change_notifier::erase(sub_iter it)
{
  //called by change_subscriber destructor to erase itself from subscriber_list
  subscriber_list_mtx.lock();
  //cout << "erasing subscriber from list" << endl;
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
  //add self to subscriber list:
  p.add_subscriber(*this);
}

void change_subscriber::store_iterator(sub_iter i)
{
  //cout << "store_it creation_time= " << creation_time << endl;
  it = i;
}

bool change_subscriber::unfreeze()
{
  //This function unfreezes a subscriber's condition_variable if it is waiting:
  //(otherwise, any change will be notified of when wait() is called)
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
  //This subscriber will now be notified once for a globally-notified change:

  //cout << "notifying of change" << endl;
  //cout << "creation_time = " << creation_time << endl;
  //cout << "waiting_on_cv = " << waiting_on_cv << endl;
  if (waiting_on_cv)
  {
    //cout << "is waiting" << endl;
    cv.notify_one();
  }
  else
  {
    num_globally_notified_changes++;
  }
  //cout << "notified" << endl;
}


void change_subscriber::wait()
{
  if (num_globally_notified_changes == 0)
  {
    //no global change notifications, check for single change notifications:
    parent->unhandled_changes_mtx.lock();
    if (parent->num_unhandled_changes >= 0)
    {
      //there is a change; don't wait, immedialely notify calling thread:

      unique_l->unlock();
      //decrement change counter:
      parent->num_unhandled_changes--;
      parent->unhandled_changes_mtx.unlock();
      unique_l->lock();
    }
    else
    {
      //no changes, wait to be notified:
      parent->num_waiting++;
      parent->unhandled_changes_mtx.unlock();
      //cout << "waiting on cv" << endl;
      waiting_on_cv = true;
      cv.wait(*unique_l);
      parent->num_waiting--;
      waiting_on_cv = false;
    }
  }
  else
  {
    //cout << "num_globally_notified_changes = " << num_globally_notified_changes << endl;

    //There is a globally-notified change to notify of first:

    //don't wait; the calling thread is immedialely notified of a change:

    unique_l->unlock();
    //cout << "unlocked unique_l" << endl;
    //decrement change counter:
    num_globally_notified_changes--;
    //cout << "decremented num_globally_notified_changes" << endl;
    unique_l->lock();
    //cout << "re-locked unique_l" << endl;
  }
}

change_subscriber::~change_subscriber()
{
  parent->erase(it);
}

#endif
