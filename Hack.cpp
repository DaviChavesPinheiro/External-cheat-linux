#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <fstream>

using namespace std;

bool is_number(const string& s)
{
  string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

long findPidByName(string proc_name) {
  chdir("/proc");

  DIR* dir = opendir("/proc");

  struct dirent* dir_entry;
  while (dir_entry = readdir(dir)) {
    // Se o nome da pasta não é um pid válido
    if(!is_number(dir_entry->d_name)) continue;
    
    chdir(dir_entry->d_name);
    
    string name;
    ifstream status("status");
    getline(status, name);
    
    // Se o processo com nome correto foi achado
    if(name.find(proc_name) != name.npos) return atoi(dir_entry->d_name);
  
    chdir("..");
  }

  return -1;
}

int main (int argc, char *argv[])
{
  if(argc < 2) {
    cout << "[Error]: Parâmetros inválidos!" << endl;
    return 1;
  }

  long pid = findPidByName(argv[1]);

  if(pid == -1) {
    cout << "[Error]: Process not found" << endl;
    return 1;
  }

  cout << "[Info]: Processo encontrado (" << pid << ")" << endl;
  return 0;
}
