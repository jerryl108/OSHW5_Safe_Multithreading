#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include "change_notifier.h"
#include <cassert>

using namespace std;

deque<string> file_queue;
deque<int> count_queue;
string search_str;
mutex file_queue_mutex;
mutex count_queue_mutex;
change_notifier count_queue_modified;
bool mappers_running = true;
int num_reducers_running;
int num_reducers;
int num_mappers;

void count_strings();
void sum_counts(int reducer_index);
void get_user_input();

int main()
{
  num_mappers = -1;
  num_reducers = -1;
  string temp;
  vector<thread*> mappers;
  vector<thread*> reducers;

  //fill file queue:
  ifstream index_file("files.dat");
  string filename;
  while(getline(index_file, filename))
  {
    file_queue.push_back("data/"+filename);
  }
  index_file.close();

  get_user_input();
  //num_mappers = file_queue.length;

  cout << "creating mapper thread" << endl;

  //create mapper threads:
  for (int i = 0; i < num_mappers; i++)
  {
    thread *mapper = new thread(&count_strings);
    mappers.push_back(mapper);
  }

  num_reducers_running = num_reducers;

  cout << "done, creating reducer threads" << endl;
  //create reducer threads:
  for (int i = 0; i < num_reducers; i++)
  {
    thread *reducer = new thread(&sum_counts,i);
    reducers.push_back(reducer);
  };

  cout << "done, waiting for mapper termination" << endl;
  //wait for the termination of all mapper threads:
  for (int i = 0; i < num_mappers; i++)
  {
    mappers[i]->join();
    delete mappers[i];
  }
  cout << "done, waiting for reducer termination" << endl;

  //There are no more mapper threads:
  mappers_running = false;

  //make damned sure that the reducer threads realize that:
  while (num_reducers_running > 0)
  {
    count_queue_modified.notify_all();
    this_thread::yield();
  }
  cout << "notified all reducers" << endl;
  /**
  * note: This loop only runs beyond 1 iteration in a few edge cases
  * when the change_subscriber condition_variable(s) refuse(s) to notify
  * in this specific instance.
  */

  //wait for the termination of all reducer threads:
  for (int i = 0 ; i < num_reducers ; i++)
  {
    cout << "reducer " << i << " join" << endl;
    reducers[i]->join();
    delete reducers[i];
  }

  cout << "There are " << count_queue.front() << " total instances of the string \"" << search_str << "\"." << endl;

  return 0;
}

void sum_counts(int reducer_index)
{
  //cout << reducer_index << ": AAA" << endl;condition

  int count1, count2, sum;
  unique_lock<mutex> lck(count_queue_mutex);
  change_subscriber queue_changed;
  queue_changed.subscribe(count_queue_modified,lck);
  //cout << "after_subscribing, cc= " << queue_changed.change_count << endl;

  while (mappers_running)
  {
    cout << "mappers_running = " << mappers_running << endl;
    cout << "waiting" << endl;
    queue_changed.wait();
    cout << "wait finished" << endl;
    if(count_queue.size() > 1)
    {
      count1 = count_queue.front();
      count_queue.pop_front();

      count2 = count_queue.front();
      count_queue.pop_front();
      cout << reducer_index << ": adding " << count1 << " and " << count2 << endl;
      lck.unlock();

      sum = count1 + count2;

      lck.lock();
      count_queue.push_back(sum);
      count_queue_modified.notify_one();
      cout << reducer_index << ": notified reducers" << endl;
    }
  }
  num_reducers_running--;
  cout << reducer_index << ": reducer terminating, num_reducers_running = " << num_reducers_running << endl;
  cout << reducer_index << ": done terminating" << endl;
}

void count_strings()
{
  string file_name;
  streambuf* read_buffer;
  int count;
  int search_str_index;

  while(true)
  {
    count = 0;
    search_str_index = 0;
    cout << "checking file queue" << endl;
    file_queue_mutex.lock();
    if (file_queue.size() > 0)
    {
      file_name=file_queue.front();
      file_queue.pop_front();
    }
    else
    {
      file_queue_mutex.unlock();
      break; //no more files
    };

    file_queue_mutex.unlock();

    cout << endl << "file " << file_name << endl;

    ifstream file(file_name);

    if (file) {}
      //cout << "successfully opened file" << endl;
    else {
      cout << "unable to open" << endl;
      return;
    }


    char c;

    //read file in one character at a time:
    while (!file.eof())
    {
      file.get(c);
      //cout << "read in char '" << c << "'" << endl;
      if (c == search_str[search_str_index])
      {
        search_str_index++;
      }
      else search_str_index = 0;
      if (search_str_index == search_str.length())
      {
        count++;
        search_str_index = 0;
      }
    }

    cout << "finished searching " << file_name << endl;

    cout << "count is " << count << endl;

    file.close();

    cout << "adding sum to count queue" << endl;

    //add count to reducer queue:
    count_queue_mutex.lock();
    cout << "mutex locked" << endl;
    count_queue.push_back(count);
    count_queue_mutex.unlock();
    //notify reducer threads of update:
    count_queue_modified.notify_all();
    cout << "notified reducers" << endl;
    cout << "mutex unlocked" << endl;
  }
  cout << "mapper thread done" << endl;
}
void get_user_input()
{
  cout << "Enter the keyword/string to search for: " << endl;
  cin >> search_str;
  cout << "Enter the number of mapper threads to use: ";
  cin >> num_mappers;
  while (num_mappers <= 0)
  {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Please enter a non-zero number of mappers: ";
    cin >> num_mappers;
  }
  cout << "Enter the number of reducer threads to use: ";
  cin >> num_reducers;
  while (num_reducers <= 0)
  {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Please enter a non-zero number of reducers: ";
    cin >> num_reducers;
  }
}
