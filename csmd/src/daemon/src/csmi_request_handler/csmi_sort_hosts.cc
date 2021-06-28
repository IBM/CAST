#include <string.h> // strverscmp()

#include "csmi_sort_hosts.h"

int
csmi_hostname_compare(const void *hosta, const void *hostb) {
  const char *p1, *p2;

  p1 = *(char **)hosta; p2 = *(char **)hostb;
  // handle degenrate cases that should never happen:
  if (p1 && !p2) { return -1; }
  else if (p2 && !p1) { return 1; }
  else if (!p1 && !p2) { return 0; }

  return strverscmp(p1, p2);
}
