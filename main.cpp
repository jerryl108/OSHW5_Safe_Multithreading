#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include "change_notifier.h"

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

void count_string_ocurrences();
void sum_counts(int reducer_index);
void get_user_input();
void read_config_file();
void fill_file_queue();

int main()
{
  num_mappers = -1;
  num_reducers = -1;
  string temp;
  vector<thread*> mappers;
  vector<thread*> reducers;

  //fill file queue:
  try
  {
    fill_file_queue();
  }
  catch (exception e)
  {
    cout << "cannot read input filenames from files.dat" << endl;
    return -1;
  }

  read_config_file();
  get_user_input();
  //num_mappers = file_queue.length;

  //cout << "creating mapper thread" << endl;

  //create mapper threads:
  for (int i = 0; i < num_mappers; i++)
  {
    thread *mapper = new thread(&count_string_ocurrences);
    mappers.push_back(mapper);
  }

  num_reducers_running = num_reducers;

  //cout << "done, creating reducer threads" << endl;
  //create reducer threads:
  for (int i = 0; i < num_reducers; i++)
  {
    thread *reducer = new thread(&sum_counts,i);
    reducers.push_back(reducer);
  };

  //cout << "done, waiting for mapper termination" << endl;
  //wait for the termination of all mapper threads:
  for (int i = 0; i < num_mappers; i++)
  {
    mappers[i]->join();
    delete mappers[i];
  }
  //cout << "done, waiting for reducer termination" << endl;

  //There are no more mapper threads:
  mappers_running = false;

  //make damned sure that the reducer threads realize that:
  while (num_reducers_running > 0)
  {
    count_queue_modified.notify_all();
    this_thread::yield();
  }
  //cout << "notified all reducers" << endl;
  /**
  * note: This loop only runs beyond 1 iteration in a few edge cases
  * when the change_subscriber condition_variable(s) refuse(s) to notify
  * from this point (if those cases even still exist).
  */

  //wait for the termination of all reducer threads:
  for (int i = 0 ; i < num_reducers ; i++)
  {
    reducers[i]->join();
    delete reducers[i];
  }

  cout << "There are " << count_queue.front() << " total instances of the string \"" << search_str << "\"." << endl;

  return 0;
}

void sum_counts(int reducer_index)
{
  int count1, count2, sum;
  unique_lock<mutex> lck(count_queue_mutex);
  change_subscriber queue_changed;
  queue_changed.subscribe(count_queue_modified,lck);

  while (mappers_running)
  {
    //passive wait for change in count queue:
    queue_changed.wait();
    //if 2 or more counts exist in the queue:
    if(count_queue.size() > 1)
    {
      count1 = count_queue.front();
      count_queue.pop_front();

      count2 = count_queue.front();
      count_queue.pop_front();
      //cout << reducer_index << ": adding " << count1 << " and " << count2 << endl;
      lck.unlock();

      //add the first two counts:
      sum = count1 + count2;

      lck.lock();
      count_queue.push_back(sum);
      count_queue_modified.notify_one();
      //cout << reducer_index << ": notified reducers" << endl;
    }
  }
  num_reducers_running--;
  //cout << reducer_index << ": reducer terminating, num_reducers_running = " << num_reducers_running << endl;
  //cout << reducer_index << ": done terminating" << endl;
}

void count_string_ocurrences()
{
  string file_name;
  //streambuf* read_buffer;
  int count;
  int search_str_index;

  //loop through files in file queue:
  while(true)
  {
    count = 0;
    search_str_index = 0;
    //cout << "checking file queue" << endl;

    //get file from file queue:
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

    //cout << endl << "file " << file_name << endl;

    //try to open file:
    ifstream file(file_name);

    if (file) {}
      //cout << "successfully opened file" << endl;
    else {
      cout << "cannot open file " << file_name << endl;
      continue;
    }


    char c;

    //read file in one character at a time:
    while (!file.eof())
    {
      file.get(c);
      //cout << "read in char '" << c << "'" << endl;
      if (c == search_str[search_str_index])
      {
        //the next character of the search string has been found
        search_str_index++;
      }
      else search_str_index = 0;
      //if entire search string found:
      if (search_str_index == search_str.length())
      {
        count++;
        search_str_index = 0;
      }
    }

    //cout << "finished searching " << file_name << endl;

    //cout << "count is " << count << endl;

    file.close();

    //cout << "adding sum to count queue" << endl;

    //add count to reducer queue:
    count_queue_mutex.lock();
    count_queue.push_back(count);
    count_queue_mutex.unlock();
    //notify reducer threads of update:
    count_queue_modified.notify_all();
    //cout << "notified reducers" << endl;
  }
  //cout << "mapper thread done" << endl;
}

void fill_file_queue()
{
  try
  {
    ifstream index_file("files.dat");
    string filename;
    //read lines from files.dat
    while(getline(index_file, filename))
    {
      //add uncommented lines to queue
      if (filename.length() > 0 && filename[0] != '#')
        file_queue.push_back("data/"+filename);
    }
    index_file.close();
  }
  catch (exception e)
  {
    throw e;
  }
}

void read_config_file()
{
  ifstream config_file("config.txt");
  if (!config_file) return;
  string option_name;
  //read in configuration parameters:
  while(!config_file.eof())
  {
    config_file >> option_name;
    //ignore comments:
    if (option_name.length() > 0 && option_name[0] == '#')
    {
      config_file.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    else if (option_name == "search_string:")
    {
      config_file >> search_str;
      //cout << "search_str " << search_str << endl;
      if (!config_file.good())
        break;
      //parse search string within single quotes (that can be escaped with a \)
      if (search_str.length() > 0)
      {
        if (search_str[0] != '\'')
          break;
        else if (search_str[search_str.length()-1] != '\'')
        {
          //the search string has whitespace!
          //parse it one character at a time:
          char c;
          bool escaped = false;
          bool invalid = false;

          while (config_file.good())
          {
            //get character
            config_file.get(c);

            if (c == '\'')
            {
              if (escaped)
              {
                escaped = false;
                continue; //ignore ' character
              }
              break; //end of search string
            }
            else if (c == '\n')
            {
              search_str = "";
              invalid = true;
              break; //stop parsing search string
            }
            else if (c == '\\')
            {
              //next ' character will be escaped
              escaped = true;
              continue; //ignore escape character
            }
            //otherwise, add character to search_str
            search_str += c;
            escaped = false;
          }
          if (!config_file.good() || invalid)
          {
            //there was a problem with the search string:
            search_str = "";
            break;
          }
          else
          {
            //remove leading single quote
            search_str = search_str.substr(1,search_str.length()-1);
          }
        }
        else
        {
          //no whitespace, search string completely contained in quotes
          //remove single qoutes:
          search_str = search_str.substr(1,search_str.length()-2);
        }
      }
      //done parsing search string
    }
    else if (option_name == "num_mappers:")
    {
      config_file >> num_mappers;
      if (!config_file.good())
        break;
    }
    else if (option_name == "num_reducers:")
    {
      config_file >> num_reducers;
      if (!config_file.good())
        break;
    }
  }
  config_file.close();
}

void get_user_input()
{
  //if search_str not already read in from config file:
  if (search_str.length() == 0)
  {
    cout << "Enter the keyword/string to search for: " << endl;
    cin >> search_str;
  }
  //if num_mappers not already read in from config file:
  if (num_mappers <= 0)
  {
    cout << "Enter the number of mapper threads to use: ";
    cin >> num_mappers;
    //error checking loop:
    while (num_mappers <= 0)
    {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Please enter a non-zero number of mappers: ";
      cin >> num_mappers;
    }
  }
  //if num_reducers not already read in from config file:
  if (num_reducers <= 0)
  {
    cout << "Enter the number of reducer threads to use: ";
    cin >> num_reducers;
    //error checkig loop:
    while (num_reducers <= 0)
    {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Please enter a non-zero number of reducers: ";
      cin >> num_reducers;
    }
  }
}
