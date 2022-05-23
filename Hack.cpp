#include <iostream>
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

unsigned long scanInt(long pid, unsigned long heap_s, unsigned long heap_e, int number) {
  struct iovec local[1];
  struct iovec remote[1];
  char buf[4];
  ssize_t nread;

  local[0].iov_base = buf;
  local[0].iov_len = 4;
  remote[0].iov_base = (void*) heap_s;
  remote[0].iov_len = 4;
  
  unsigned long scan_area = 0;

  while(heap_e - heap_s >= scan_area) {
    remote[0].iov_base = (void*)(heap_s + scan_area);
    nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if(nread != 4) return 0;
    
    // Achamos o int
    if(*((int*)buf) == number) return (unsigned long)remote[0].iov_base;

    scan_area++;
  }
  return 0;
}

int putInt(long pid, unsigned long addr, int number) {
  struct iovec local[1];
  struct iovec remote[1];
  ssize_t nread;

  local[0].iov_base = &number;
  local[0].iov_len = 4;
  remote[0].iov_base = (void*) addr;
  remote[0].iov_len = 4;
  
  nread = process_vm_writev(pid, local, 1, remote, 1, 0);
  return nread == 4 ? 1 : -1;
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

  // Find heap start address
  unsigned long heap_s = -1, heap_e = -1;
  if (findHeapAddress(pid, &heap_s, &heap_e) == -1) {
    cout << "[Error]: Heap address not found" << endl;
    return 1;
  }
  printf("[Info]: Heap: 0x%lx - 0x%lx\n", heap_s, heap_e);
  printf("[Info]: Heap: %lu bytes\n", heap_e - heap_s);

  // Find addr from number
  unsigned long addr = scanInt(pid, heap_s, heap_e, 0x11223344);
  if(addr == 0) {
    printf("([Error]: Endereco não encontrado)\n");
    return 1;
  }

  printf("[Info]: Scan: 0x%lx\n", addr);
  
  // Write on memory
  unsigned long res = putInt(pid, addr, 0x12345678);
  if(res == -1) {
    printf("([Error]: Erro ao tentar escrever na memória\n");
    return 1;
  }

  printf("[Info]: Escrita realizada com sucesso!\n");

  return 0;
}
