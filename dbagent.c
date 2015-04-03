#include "agt.h"

int main() {
  agent_init();
  agent_start();
  before_exit();
  exit(0);
}
