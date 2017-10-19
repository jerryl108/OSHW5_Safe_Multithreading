#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
using namespace std;

deque<string> file_que;
deque<int> count_que;
int nMapper;
int nReducer;
string keyword;
mutex file_queMtx;
mutex count_queMtx;
int current_mapper_working=0;

//int count_keyword_lbl(string material_que);
void read_file(int id);
void push_queue(ifstream &file,deque<string> &file_que);
void find_sum();
void user_input();

void sum()
{
  //cout << "AAA" << endl;
  int temp1,temp2,sum;
  while(current_mapper_working>0)
  {
    count_queMtx.lock();
    if(count_que.size()>1)
    {
      temp1=count_que.front();
      count_que.pop_front();
      temp2=count_que.front();
      count_que.pop_front();
      count_queMtx.unlock();
      sum=temp1+temp2;
      count_queMtx.lock();
      count_que.push_back(sum);
    }
    count_queMtx.unlock();

  }
}

void read_file(int id)
{
  string file_name;
  int count=0;
  //cout << file_que.size() << endl;
  while(1)
  {
    count=0;
    file_queMtx.lock();
    if(file_que.size()>0)
    {
      file_name=file_que.front();
      file_que.pop_front();
      file_queMtx.unlock();
    }
    else
    {
      file_queMtx.unlock();
      break;
    }
    string material;
    ifstream file(file_name);
    //cout << id << ":" << file_name << endl;
    while(file>>material)
    {
      //file>>material;
      if(material==keyword)
      count++;
    }
    file.close();
    count_queMtx.lock();
    count_que.push_back(count);
    count_queMtx.unlock();
    //cout<<count << endl;
  }
  current_mapper_working--;
}

void push_queue(ifstream &file,deque<string> &file_que)
{
  string temp;
  while(getline(file,temp))
  {
    file_que.push_back(temp);
  }
}

void user_input()
{
  cout << "Enter the number of mapper: ";
  cin >> nMapper;
  cout << "Enter the number of reducer: ";
  cin >> nReducer;
  cout << "Enter the keyword: ";
  cin >> keyword;
}
/*
int count_keyword_lbl(string material_que)
{
  //material_que="aaaaaaab";
  //keyword="aa";
  int count=0;
  for(int i=0;i<material_que.size();i++)
  {
    if(material_que[i]==keyword[0])
    {
      for(int j=1;j+i<material_que.size()&&j<keyword.size();j++)
      {
        if(keyword[j]!=material_que[i+j])
        break;
        if(j==keyword.size()-1)
        {
          count++;
          i+=keyword.size()-1;
        }
      }
    }
  }
  return count;
}
*/


int main()
{
  string temp;
  //string temp1;
  ifstream file("files.dat");
  push_queue(file,file_que);
  file.close();
  //keyword = "the";
  //cout << "run" << endl;
  user_input();


  current_mapper_working=nMapper;
  vector<thread*> mappers;
  for(int i=0;i<nMapper;i++){
    thread *tmp = new thread(&read_file,i);
    mappers.push_back(tmp);
  }
  vector<thread*> reducers;
  for(int i=0;i<nReducer;i++){
    thread *tmp = new thread(&sum);
    reducers.push_back(tmp);
  }

  for(int i=0;i<nMapper;i++){
    mappers[i]->join();
  }
  for(int i=0;i<nReducer;i++){
    reducers[i]->join();
  }

  //cout << "end" << endl;
  cout << "Total count of keyword: " << count_que.front() << endl;;
  //cout<<count_keyword_lbl(temp1)<<endl;
  return 0;
}
