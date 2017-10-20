#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include <condition_variable>
using namespace std;

deque<string> file_queue;
deque<int> count_queue;
string search_str;
mutex file_queue_mutex;
mutex count_queue_mutex;
condition_variable count_queue_modified;
//array of bools to determine if each reducer waits on count_queue_modified:
bool* should_cv_wait;
bool mappers_running = true;
int num_reducers_running;
int num_reducers;
int num_mappers;

void count_strings();
void sum_counts(int reducer_index);
void reducers_skip_one_wait();
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
  should_cv_wait = new bool[num_reducers];
  for (int i = 0; i < num_reducers; i++)
  {
    should_cv_wait[i] = true;
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
  //make damned sure that the producer threads realize that:
  reducers_skip_one_wait();
  count_queue_modified.notify_all();
  cout << "notified all reducers" << endl;


  //wait for the termination of all mapper threads:
  for (int i = 0 ; i < num_reducers ; i++)
  {
    reducers[i]->join();
    delete reducers[i];
  }

  delete [] should_cv_wait;

  cout << "There are " << count_queue.front() << " total instances of the string \"" << search_str << "\"." << endl;

  return 0;
}

void sum_counts(int reducer_index)
{
  //cout << reducer_index << ": AAA" << endl;condition
  unique_lock<mutex> count_queue_lock(count_queue_mutex);

  int count1, count2, sum;
  while (mappers_running)
  {
    if (should_cv_wait[reducer_index])
    {
      cout << reducer_index << ": reducer waiting on notification" << endl;
      count_queue_modified.wait(count_queue_lock);
      cout << reducer_index << ": reducer received notification" << endl;
    }
    else should_cv_wait[reducer_index] = true;

    if(count_queue.size() > 1)
    {
      count1 = count_queue.front();
      count_queue.pop_front();

      count2 = count_queue.front();
      count_queue.pop_front();
      cout << reducer_index << ": adding " << count1 << " and " << count2 << endl;
      count_queue_lock.unlock();

      sum = count1 + count2;

      count_queue_lock.lock();
      count_queue.push_back(sum);
      reducers_skip_one_wait();
      count_queue_modified.notify_all();
      cout << reducer_index << ": notified reducers" << endl;
    }
  }
  num_reducers_running--;
  cout << reducer_index << ": reducer terminating, num_reducers_running = " << num_reducers_running << endl;
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
    reducers_skip_one_wait();
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
void reducers_skip_one_wait()
{
  for (int i = 0; i < num_reducers; i++)
  {
    should_cv_wait[i] = false;
  }
}
