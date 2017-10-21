#ifndef CHANGE_NOTIFIER
#define CHANGE_NOTIFIER

#include "change_notifier.h"

/**
 * The change_notifier class is basically a condition_variable that
 * allows threads to receive notifications even if they are not waiting
 * when those notifications are sent.
 */

notification_subscriber change_notifier::subscribe(unique_lock<mutex>& unique_l)
{
  subscriber_list.push_back(notification_subscriber(unique_l,*this));
  subscriber_list.back().store_iterator(--subscriber_list.end());
}

void change_notifier::notify_all()
{
  for (sub_iter it = subscriber_list.begin(); it != subscriber_list.end(); it++)
  {
    (*it).notify_change();
  }
}

void notification_subscriber::store_iterator(sub_iter i)
{
  it = &i;
}

change_notifier::~change_notifier()
{
  subscriber_list.clear();
}

void notification_subscriber::wait()
{
  if (change_count == 0)
  {
    cv.wait(*unique_l);
  }
  else
  {
    //no need to wait; a change has happened:
    change_count--;
  }
}

void notification_subscriber::notify_change()
{
  if (waiting_on_cv)
  {
    cv.notify_one();
  }
  else
  {
    change_count++;
  }
}
//should be called at end of subscribing function (instead of destructor):
void notification_subscriber::close()
{
  parent->erase(*it);
}

#endif
