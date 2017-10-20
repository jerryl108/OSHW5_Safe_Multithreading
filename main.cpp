#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
using namespace std;

deque<string> file_queue;
deque<int> count_queue;
int num_mappers;
int num_reducers;
string search_str;
mutex file_queue_mutex;
mutex count_queue_mutex;
bool mappers_running = true;

void count_strings();
void sum_counts();
void fill_file_queue(ifstream &index_file, deque<string> &file_queue);

int main()
{
  string temp;
  vector<thread*> mappers;
  vector<thread*> reducers;

  ifstream index_file("files.dat");
  fill_file_queue(index_file, file_queue);
  index_file.close();

  cout << "Enter the keyword/string to search for: " << endl;
  cin >> search_str;
  cout << "Enter the number of mapper threads to use: ";
  cin >> num_mappers;
  cout << "Enter the number of reducer threads to use: ";
  cin >> num_reducers;

  //num_mappers = file_queue.length;

  cout << "creating mapper thread" << endl;

  //create mapper threads:
  for (int i = 0; i < num_mappers; i++)
  {
    thread *mapper = new thread(&count_strings);
    mappers.push_back(mapper);
  }

  cout << "done, creating reducer threads" << endl;
  //create reducer threads:
  for (int i = 0; i < num_reducers; i++)
  {
    thread *reducer = new thread(&sum_counts);
    reducers.push_back(reducer);
  };

  cout << "done, waiting for mapper termination" << endl;
  //wait for the termination of all mapper threads:
  for (int i = 0; i < num_mappers; i++)
  {
    mappers[i]->join();
  }

  mappers_running = false;

  cout << "done, waiting for reducer termination" << endl;

  //wait for the termination of all mapper threads:
  for (int i = 0 ; i < num_reducers ; i++)
  {
    reducers[i]->join();
  }

  cout << "There are " << count_queue.front() << " total instances of the string \"" << search_str << "\"." << endl;

  return 0;
}

void sum_counts()
{
  //cout << "AAA" << endl;
  int count1, count2, sum;
  while (mappers_running)
  {
    count_queue_mutex.lock();
    if(count_queue.size() > 1)
    {
      count1 = count_queue.front();
      count_queue.pop_front();

      count2 = count_queue.front();
      count_queue.pop_front();

      count_queue_mutex.unlock();

      sum = count1 + count2;

      count_queue_mutex.lock();
      count_queue.push_back(sum);
    }
    count_queue_mutex.unlock();
  }
}

void count_strings()
{
  string file_name;
  streambuf* read_buffer;
  int count;
  int search_str_index = 0;

  while(true)
  {
    cout << "checking file queue" << endl;
    file_queue_mutex.lock();
    if (file_queue.size() > 0)
    {
      file_name=file_queue.front();
      file_queue.pop_front();
    }
    else break; //no more files
    file_queue_mutex.unlock();

    cout << endl << "file " << file_name << endl;

    ifstream file(file_name);

    if (file)
      cout << "successfully opened file" << endl;
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

    file.close();

    cout << "adding file to count queue" << endl;

    //add count to reducer queue:
    count_queue_mutex.lock();
    cout << "mutex locked" << endl;
    count_queue.push_back(count);
    count_queue_mutex.unlock();
    cout << "mutex unlocked" << endl;
  }
}

void fill_file_queue(ifstream &file, deque<string> &file_queue)
{
  string filename;
  while(getline(file, filename))
  {
    file_queue.push_back("data/"+filename);
  }
}
