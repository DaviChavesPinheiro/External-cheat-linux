#include <iostream>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/uio.h>
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

int findHeapAddress(long pid, unsigned long* heap_s, unsigned long* heap_e) {
  // Gets maps file path
  char maps_path[64];
  sprintf(maps_path, "/proc/%ld/maps", pid);

  // Read the file
  ifstream maps(maps_path);
  string line;

  while(std::getline(maps, line)) {
    if(line.find("[heap]") != line.npos) {
      *heap_s = std::stoul(line.substr(0, 12), NULL, 16);
      *heap_e = std::stoul(line.substr(13, 12), NULL, 16);
      return 0;
    }
  }

  return -1;
}

int findStackAddress(long pid, unsigned long* stack_s, unsigned long* stack_e) {
  // Gets maps file path
  char maps_path[64];
  sprintf(maps_path, "/proc/%ld/maps", pid);

  // Read the file
  ifstream maps(maps_path);
  string line;

  while(std::getline(maps, line)) {
    if(line.find("[stack]") != line.npos) {
      *stack_s = std::stoul(line.substr(0, 12), NULL, 16);
      *stack_e = std::stoul(line.substr(13, 12), NULL, 16);
      return 0;
    }
  }

  return -1;
}

unsigned long scanBytes(long pid, unsigned long heap_s, unsigned long heap_e, char *bytes, int bytes_n) {
  struct iovec local[1];
  struct iovec remote[1];
  char buf[bytes_n];
  ssize_t nread;

  local[0].iov_base = buf;
  local[0].iov_len = bytes_n;
  remote[0].iov_base = (void*) heap_s;
  remote[0].iov_len = bytes_n;
  
  unsigned long scan_area = 0;

  while(heap_e - heap_s >= scan_area) {
    remote[0].iov_base = (void*)(heap_s + scan_area);
    nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if(nread != bytes_n) return 0;
    
    // Match
    if(memcmp(buf, bytes, bytes_n) == 0) return (unsigned long)remote[0].iov_base;

    scan_area++;
  }
  return 0;
}

int putBytes(long pid, unsigned long addr, char *bytes, int bytes_n) {
  struct iovec local[1];
  struct iovec remote[1];
  ssize_t nread;

  local[0].iov_base = bytes;
  local[0].iov_len = bytes_n;
  remote[0].iov_base = (void*) addr;
  remote[0].iov_len = bytes_n;
  
  nread = process_vm_writev(pid, local, 1, remote, 1, 0);
  return nread == bytes_n ? 1 : -1;
}

int main (int argc, char *argv[])
{
  if(argc < 2) {
    cout << "[Error]: Invalid parameters" << endl;
    return 1;
  }

  long pid = findPidByName(argv[1]);

  if(pid == -1) {
    cout << "[Error]: Process not found" << endl;
    return 1;
  }

  cout << "[Info]: Process found (" << pid << ")" << endl;

  // Find heap range address
  // unsigned long heap_s = -1, heap_e = -1;
  // if (findHeapAddress(pid, &heap_s, &heap_e) == -1) {
  //   cout << "[Error]: Heap address not found" << endl;
  //   return 1;
  // }
  // printf("[Info]: Heap: 0x%lx - 0x%lx\n", heap_s, heap_e);
  // printf("[Info]: Heap: %lu bytes\n", heap_e - heap_s);

  // Find stack range address
  unsigned long stack_s = -1, stack_e = -1;
  if (findStackAddress(pid, &stack_s, &stack_e) == -1) {
    cout << "[Error]: Stack address not found" << endl;
    return 1;
  }
  printf("[Info]: Stack: 0x%lx - 0x%lx\n", stack_s, stack_e);
  printf("[Info]: Stack: %lu bytes\n", stack_e - stack_s);

  // Find addr from array of bytes
  char bytes_r[4] = {0x44, 0x33, 0x22, 0x11};

  // unsigned long addr = scanBytes(pid, heap_s, heap_e, bytes_r, sizeof(bytes_r)); // heap
  unsigned long addr = scanBytes(pid, stack_s, stack_e, bytes_r, sizeof(bytes_r)); // stack

  if(addr == 0) {
    printf("[Error]: Address not found\n");
    return 1;
  }

  printf("[Info]: Scan: 0x%lx\n", addr);

  // Write on memory
  char bytes_w[4] = {0x78, 0x56, 0x34, 0x12};
  unsigned long res = putBytes(pid, addr, bytes_w, sizeof(bytes_w));
  if(res == -1) {
    printf("[Error]: Error writing to memory\n");
    return 1;
  }

  printf("[Info]: Writing done successfully!\n");

  return 0;
}
